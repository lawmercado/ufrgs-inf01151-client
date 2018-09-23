#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include "sync.h"
#include "utils.h"

#define EVENT_SIZE ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN ( 1024 * ( EVENT_SIZE + 16 ) )

pthread_t __watcher_thread;
pthread_mutex_t __eventHandlingMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t __eventsDoneProcessing = PTHREAD_COND_INITIALIZER;
int __isEventProcessing = FALSE;
int __stopEventHandling = FALSE;

int __inotify_instance;
int __inotify_dir_watcher;

int __watcher_set(char *dir_path)
{
    __inotify_instance = inotify_init1(IN_NONBLOCK);

    // Checking for error
    if( __inotify_instance < 0 )
    {
        perror("inotify_init");

        return -1;
    }

    struct stat st = {0};

    // If the directory does not exist, create it
    if (stat(dir_path, &st) == -1)
    {
        mkdir(dir_path, 0700);
    }

    // Adding the specified directory into watch list
    __inotify_dir_watcher = inotify_add_watch( __inotify_instance, dir_path, IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MODIFY );
    
    return 0;
}

/**
 * Handle an inotify event
 *
 * @param struct inotify_event* event The event
 */
void __watcher_handle_event(struct inotify_event *event)
{
    pthread_mutex_lock( &__eventHandlingMutex );
    __isEventProcessing = TRUE;

    if( event->len )
    {
        if( event->mask & IN_CREATE )
        {
            if( !(event->mask & IN_ISDIR) )
            {
                utils_debug_fmt("sync", "'%s' created", event->name);
            }
        }
        else if( event->mask & IN_DELETE )
        {
            if( !(event->mask & IN_ISDIR) )
            {
                utils_debug_fmt("sync", "'%s' deleted", event->name);
            }
        }
        else if( event->mask & IN_MOVED_TO )
        {
            if( !(event->mask & IN_ISDIR) )
            {
                utils_debug_fmt("sync", "'%s' moved into", event->name);
            }
        }
        else if( event->mask & IN_MODIFY )
        {
            if( !(event->mask & IN_ISDIR) )
            {
                utils_debug_fmt("sync", "'%s' modified", event->name);
            }
        }
    }

    __isEventProcessing = FALSE;
    pthread_cond_signal( &__eventsDoneProcessing );
    pthread_mutex_unlock( &__eventHandlingMutex );
}

void __watcher_unset()
{
    // Removes the directory from the watch list
    inotify_rm_watch( __inotify_instance, __inotify_dir_watcher );

    // Close the inotify instance
    close( __inotify_instance );
}

/**
 * Sets up the watcher to the directory informed
 *
 * @param void* arg The directory path
 */
void *__watcher(void *dir_path)
{
    char buffer[EVENT_BUF_LEN];
    int length, idxEvent = 0;

    while( !__stopEventHandling )
    {
        // Reads an event change happens on the directory
        length = read( __inotify_instance, buffer, EVENT_BUF_LEN );

        if( length > 0 )
        {
            idxEvent = 0;

            // Read list of change events happened. Reads the change event one by one and process it accordingly
            while( idxEvent < length )
            {
                struct inotify_event *event = ( struct inotify_event * ) &buffer[ idxEvent ];

                __watcher_handle_event(event);

                idxEvent += EVENT_SIZE + event->len;
            }
        }

        // To avoid excessive monitoring
        sleep(1);
    }

    __watcher_unset();

    pthread_exit(NULL);
}

/**
 * Initializes the syncronisation in the specified directory to be syncronized
 *
 * @param char* dir_path The directory to be syncronized
 * @return 0 if no errors, -1 otherwise
 */
int sync_init(char *dir_path)
{
    if( __watcher_set( (char *) dir_path ) == 0 )
    {
        if( pthread_create( &__watcher_thread, NULL, __watcher, dir_path ) == 0 )
        {
            utils_debug("sync", "Syncronization process started");

            return 0;
        }
        else
        {
            perror("pthread_create");
        }
    }

    return -1;
}

/**
 * Stop the syncronisation process
 *
 */
void sync_stop()
{
    while( __isEventProcessing )
    {
        pthread_cond_wait( &__eventsDoneProcessing, &__eventHandlingMutex );
    }

    __stopEventHandling = TRUE;

    pthread_mutex_unlock( &__eventHandlingMutex );

    pthread_join(__watcher_thread, NULL);

    utils_debug("sync", "Syncronization process ended");
}

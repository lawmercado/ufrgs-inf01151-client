#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/inotify.h>
#include "sync.h"
#include "file.h"
#include "log.h"

#define EVENT_SIZE (sizeof (struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

pthread_t __watcher_thread;
pthread_mutex_t __event_handling_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t __events_done_processing = PTHREAD_COND_INITIALIZER;
int __is_event_processing = 0;
int __stop_event_handling = 0;

int __inotify_instance;
int __inotify_dir_watcher;

char* __watched_dir_path;

/**
 * Handle an inotify event
 *
 * @param struct inotify_event* event The event
 */
void __watcher_handle_event(struct inotify_event *event)
{
    pthread_mutex_lock(&__event_handling_mutex);
    __is_event_processing = 1;

    if(event->len)
    {
        if(event->mask & IN_MODIFY)
        {
            if(!(event->mask & IN_ISDIR))
            {
                log_debug("sync", "'%s' modified", event->name);
            }
        }
        else if(event->mask & IN_MOVED_TO)
        {
            if(!(event->mask & IN_ISDIR))
            {
                log_debug("sync", "'%s' moved into", event->name);
            }
        }
        else if(event->mask & IN_DELETE)
        {
            if(!(event->mask & IN_ISDIR))
            {
                log_debug("sync", "'%s' deleted", event->name);
            }
        }
        else if(event->mask & IN_MOVED_FROM)
        {
            if(!(event->mask & IN_ISDIR))
            {
                log_debug("sync", "'%s' moved from", event->name);
            }
        }
    }

    __is_event_processing = 0;
    pthread_cond_signal(&__events_done_processing);
    pthread_mutex_unlock(&__event_handling_mutex);
}

/**
 * Unsets the current watcher
 *
 */
void __watcher_unset()
{
    // Removes the directory from the watch list
    inotify_rm_watch(__inotify_instance, __inotify_dir_watcher);

    // Close the inotify instance
    close(__inotify_instance);
}

/**
 * Sets up the watcher to the specified irectory
 *
 */
void *__watcher()
{
    char buffer[EVENT_BUF_LEN];
    int length, idx_event = 0;

    while(!__stop_event_handling)
    {
        // Reads an event change happens on the directory
        length = read(__inotify_instance, buffer, EVENT_BUF_LEN);

        if(length > 0)
        {
            idx_event = 0;

            // Read list of change events happened. Reads the change event one by one and process it accordingly
            while(idx_event < length)
            {
                struct inotify_event *event = (struct inotify_event *) &buffer[ idx_event ];

                __watcher_handle_event(event);

                idx_event += EVENT_SIZE + event->len;
            }
        }

        // To avoid excessive monitoring
        // TODO: test if this line really works in every computer
        sleep(1);
    }

    pthread_exit(NULL);
}

int __initialize_dir(char *dir_path)
{
    if(file_exists(dir_path))
    {
        file_clear_dir(dir_path);
    }
    else
    {
        file_create_dir(dir_path);
    }

    return 0;
}

/**
 * Initializes the synchronization in the specified directory to be synchronized
 *
 * @param char* dir_path The directory to be synchronized
 * @return 0 if no errors, -1 otherwise
 */
int sync_init(char *dir_path)
{
    __initialize_dir(dir_path);

    __watched_dir_path = dir_path;

    __inotify_instance = inotify_init1(IN_NONBLOCK);

    // Checking for error
    if(__inotify_instance < 0)
    {
        log_error("sync", "Could not create an inotify instance");

        return -1;
    }

    // Adding the specified directory into watch list
    __inotify_dir_watcher = inotify_add_watch(__inotify_instance, dir_path, IN_MODIFY | IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM);

    // Checking for error
    if(__inotify_dir_watcher < 0)
    {
        log_error("sync", "Could not watch '%s': directory does not exists", dir_path);

        return -1;
    }

    if(pthread_create(&__watcher_thread, NULL, __watcher, NULL) != 0)
    {
        log_error("sync", "Could not create the thread");

        return -1;
    }

    log_debug("sync", "Synchronization process started");

    return 0;
}

/**
 * Stop the synchronization process
 *
 */
void sync_stop()
{
    pthread_mutex_lock(&__event_handling_mutex);

    while(__is_event_processing)
    {
        pthread_cond_wait(&__events_done_processing, &__event_handling_mutex);
    }

    __stop_event_handling = 1;

    pthread_mutex_unlock(&__event_handling_mutex);

    pthread_join(__watcher_thread, NULL);

    __watcher_unset();

    log_debug("sync", "Synchronization process ended");
}

/**
 * Updates the file in the synchronized directory
 *
 * @param char* name The name of the file
 * @param char* buffer The content of the file
 * @param int length The buffer size of the file
 * @return 0 if no errors, -1 otherwise
 */
int sync_update_file(char name[MAX_FILENAME_LENGTH], char *buffer, int length)
{
    sync_stop();

    char path[MAX_PATH_LENGTH];
    file_path(__watched_dir_path, name, path, MAX_PATH_LENGTH);

    if(file_write_buffer(path, buffer, length) != 0)
    {
        log_error("sync", "Could not update the file '%s'", name);

        return -1;
    }

    if(sync_init(__watched_dir_path) != 0)
    {
        log_error("sync", "Could not initialize the synchronization process");

        return -1;
    }

    return 0;
}

/**
 * List the content of the watched directory
 *
 * @return 0 if no errors, -1 otherwise
 */
int sync_list_files()
{
    DIR *watched_dir;
    struct dirent *entry;
    char path[MAX_PATH_LENGTH];
    MACTimestamp entryMAC;

    watched_dir = opendir(__watched_dir_path);

    if(watched_dir)
    {
        while((entry = readdir(watched_dir)) != NULL)
        {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            file_path(__watched_dir_path, entry->d_name, path, MAX_PATH_LENGTH);

            if(file_mac(path, &entryMAC) == 0)
            {
                printf("M: %s | A: %s | C: %s | '%s'\n", entryMAC.m, entryMAC.a, entryMAC.c, entry->d_name);
            }
        }

        closedir(watched_dir);

        return 0;
    }

    return -1;
}

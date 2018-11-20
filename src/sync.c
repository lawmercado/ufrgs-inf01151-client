#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <poll.h>
#include <sys/inotify.h>
#include "sync.h"
#include "file.h"
#include "log.h"
#include "comm.h"

#define EVENT_SIZE (sizeof (struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

pthread_t __watcher_thread;
pthread_mutex_t __event_handling_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t __events_done_processing = PTHREAD_COND_INITIALIZER;
int __is_event_processing = 0;
int __stop_event_handling = 0;

int __inotify_instance;
int __inotify_dir_watcher;

char* __watched_dir_path = NULL;

/**
 * Handle an inotify event
 *
 * @param struct inotify_event* event The event
 */
void __watcher_handle_event(struct inotify_event *event)
{
    pthread_mutex_lock(&__event_handling_mutex);

    char path[FILE_PATH_LENGTH];

    if(event->len)
    {
        // Ignoring hidden files
        if(event->name[0] != '.')
        {
            if(event->mask & IN_CLOSE_WRITE)
            {
                if(!(event->mask & IN_ISDIR))
                {
                    log_debug("sync", "'%s' closed", event->name);

                    file_path(__watched_dir_path, event->name, path, FILE_PATH_LENGTH);

                    comm_upload(path);
                }
            }
            else if(event->mask & IN_MOVED_TO)
            {
                if(!(event->mask & IN_ISDIR))
                {
                    log_debug("sync", "'%s' moved into", event->name);

                    file_path(__watched_dir_path, event->name, path, FILE_PATH_LENGTH);
                    comm_upload(path);
                }
            }
            else if(event->mask & IN_DELETE)
            {
                if(!(event->mask & IN_ISDIR))
                {
                    log_debug("sync", "'%s' deleted", event->name);
                    comm_delete(event->name);
                }
            }
            else if(event->mask & IN_MOVED_FROM)
            {
                if(!(event->mask & IN_ISDIR))
                {
                    log_debug("sync", "'%s' moved from", event->name);
                    comm_delete(event->name);
                }
            }
        }
    }

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
    struct pollfd fd;
    fd.fd = __inotify_instance;
    fd.events = POLLIN;

    while(!__stop_event_handling)
    {
        int poll_status = poll(&fd, 1, SYNC_READ_TIMEOUT);

        if(poll_status == 0)
        {
            log_debug("sync", "Nothing to read");
        }
        else if(poll_status == -1)
        {
            log_error("sync", "Error while trying to read");
        }
        else
        {
            // Reads an event change happens on the directory
            length = read(__inotify_instance, buffer, EVENT_BUF_LEN);

            if(length > 0)
            {
                idx_event = 0;

                // Read list of change events happened. Reads the change event one by one and process it accordingly
                while(idx_event < length)
                {
                    struct inotify_event *event = (struct inotify_event *) &buffer[idx_event];

                    __watcher_handle_event(event);

                    idx_event += EVENT_SIZE + event->len;
                }
            }
        }
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

int sync_setup(char *dir_path)
{
    __watched_dir_path = dir_path;

    return __initialize_dir(dir_path);
}

int sync_watcher_init()
{
    __inotify_instance = inotify_init();
    __stop_event_handling = 0;

    // Checking for error
    if(__inotify_instance < 0)
    {
        log_error("sync", "Could not create an inotify instance");

        return -1;
    }

    // Adding the specified directory into watch list
    __inotify_dir_watcher = inotify_add_watch(__inotify_instance, __watched_dir_path, IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM | IN_CLOSE_WRITE);

    // Checking for error
    if(__inotify_dir_watcher < 0)
    {
        log_error("sync", "Could not watch '%s': directory does not exists", __watched_dir_path);

        return -1;
    }

    if(pthread_create(&__watcher_thread, NULL, __watcher, NULL) != 0)
    {
        log_error("sync", "Could not create the thread");

        return -1;
    }

    log_debug("sync", "Synchronization process started on '%s'", __watched_dir_path);

    return 0;
}

/**
 * Stop the synchronization process
 *
 */
void sync_watcher_stop()
{
    __stop_event_handling = 1;

    pthread_join(__watcher_thread, NULL);

    __watcher_unset();

    log_debug("sync", "Synchronization process ended");
}

int sync_delete_file(char name[FILE_NAME_LENGTH])
{
    sync_watcher_stop();

    char path[FILE_PATH_LENGTH];
    file_path(__watched_dir_path, name, path, FILE_PATH_LENGTH);

    if(file_delete(path) != 0)
    {
        log_error("sync", "Could not delete the file: does it exists?");

        return -1;
    }

    if(sync_watcher_init() != 0)
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
    char path[FILE_PATH_LENGTH];
    struct file_mactimestamp entryMAC;

    watched_dir = opendir(__watched_dir_path);

    if(watched_dir)
    {
        while((entry = readdir(watched_dir)) != NULL)
        {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            file_path(__watched_dir_path, entry->d_name, path, FILE_PATH_LENGTH);

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

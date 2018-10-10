#ifndef __SYNC_H__
#define __SYNC_H__

int sync_init();

int sync_setup(char *dir_path);

int sync_stop();

/**
 * Initializes the synchronization in the specified directory to be synchronized
 *
 * @param char* dir_path The directory to be synchronized
 * @return 0 if no errors, -1 otherwise
 */
int sync_watcher_init(char *dir_path);

/**
 * Stop the synchronization process
 *
 */
void sync_watcher_stop();

/**
 * Updates the file in the synchronized directory
 *
 * @param char* name The name of the file
 * @param char* buffer The content of the file
 * @param int length The buffer size of the file
 * @return 0 if no errors, -1 otherwise
 */
int sync_update_file(char *name, char *buffer, int length);

int sync_delete_file(char *name);

/**
 * List the content of the watched directory
 *
 * @return 0 if no errors, -1 otherwise
 */
int sync_list_files();

#endif

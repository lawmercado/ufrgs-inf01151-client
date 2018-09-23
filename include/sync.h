#ifndef __SYNC___
#define __SYNC___

#define RET_ERROR -1
#define RET_SUCCESS 0
#define TRUE 1
#define FALSE 0

/**
 * Initializes the syncronisation in the specified directory to be syncronized
 *
 * @param char* dir_path The directory to be syncronized
 * @return RET_SUCCESS if no errors, RET_ERROR otherwise
 */
int sync_init( char *dir_path );

/**
 * Stop the syncronisation process
 *
 */
void sync_stop();

/**
 * Updates the file in the syncronized dir
 *
 * @param char* name The name of the file
 * @param char* content The content of the file
 */
int sync_update_file( char *name, char *content );

#endif

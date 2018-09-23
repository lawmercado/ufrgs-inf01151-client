#ifndef __FILE___
#define __FILE___

#define MAX_FILENAME_LENGTH 255
#define MAX_PATH_LENGTH 4096

/**
 * Write a file in the specified path
 *
 * @param char* path The path of the file
 * @param char* buffer The content of the file
 * @param int length The buffer size of the file
 * @return 0 if no errors, -1 otherwise
 */
int file_write_buffer(char path[MAX_PATH_LENGTH], char *buffer, int length);

#endif

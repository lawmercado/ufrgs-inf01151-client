#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include "file.h"
#include "log.h"

/**
 * Write a file in the specified path
 *
 * @param char* path The path of the file
 * @param char* buffer The content of the file
 * @param int length The buffer size of the file
 * @return 0 if no errors, -1 otherwise
 */
int file_write_buffer(char path[MAX_PATH_LENGTH], char *buffer, int length)
{
    FILE *file = NULL;
    int bytes_writen = 0, has_errors = 0;

    log_debug("file", "Writing in '%s'", path);

    file = fopen(path, "wb");

    if(file == NULL)
    {
        log_debug("file", "Could not open '%s' for writing", path);

        return -1;
    }

    bytes_writen = fwrite(buffer, sizeof(char), length, file);
    has_errors = bytes_writen != length;

    fclose(file);

    !has_errors ? log_debug("file", "Write sucessfull") : log_error("file", "Write unsucessfull for '%s'", path);

    return !has_errors ? 0 : -1;
}

/**
 * Fills a MACTimestamp struct with the corresponding times for the file
 *
 * @param char* path The path of the file
 * @param MACTimestamp* mac The struct to initialize
 * @return 0 if no errors, -1 otherwise
 */
int file_mac(char path[MAX_PATH_LENGTH], MACTimestamp *mac)
{
    struct stat attrib;

    if(stat(path, &attrib) == 0)
    {
        strftime(mac->m, MAX_TIMESTAMP_LENGTH, "%Y %b %d %H:%M", localtime(&attrib.st_mtime));
        strftime(mac->a, MAX_TIMESTAMP_LENGTH, "%Y %b %d %H:%M", localtime(&attrib.st_atime));
        strftime(mac->c, MAX_TIMESTAMP_LENGTH, "%Y %b %d %H:%M", localtime(&attrib.st_ctime));

        return 0;
    }

    return -1;
}

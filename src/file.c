#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
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
    struct stat st;

    if(stat(path, &st) == 0)
    {
        strftime(mac->m, MAX_TIMESTAMP_LENGTH, "%Y %b %d %H:%M", localtime(&st.st_mtime));
        strftime(mac->a, MAX_TIMESTAMP_LENGTH, "%Y %b %d %H:%M", localtime(&st.st_atime));
        strftime(mac->c, MAX_TIMESTAMP_LENGTH, "%Y %b %d %H:%M", localtime(&st.st_ctime));

        return 0;
    }

    return -1;
}

/**
 * Gets the file size for the specified file
 *
 * @param char* path The path of the file
 * @return the file size if no errors, -1 otherwise
 */
int file_size(char path[MAX_PATH_LENGTH])
{
    struct stat st;

    if(stat(path, &st) == 0)
    {
        return st.st_size;
    }

    return -1;
}

int file_exists(char path[MAX_PATH_LENGTH])
{
    struct stat st;

    return stat(path, &st) == 0;
}

int file_create_dir(char path[MAX_PATH_LENGTH])
{
    return mkdir(path, 0700);
}

int file_get_name_from_path(char *path, char *filename)
{
    int i = 0;
    for(i = strlen(path); i != 0; i--)
    {
        if (path[i] == '/')
        {
            strncpy(filename, path+i+1, ((strlen(path)-i)+1));
            printf("%s\n", filename);
            return 0;
        }
    }
    
    return -1;
}

int file_read_bytes(FILE *file, char *buffer, int length)
{
    return fread(buffer, sizeof(char), length, file);
}

int file_write_bytes(FILE *file, char *buffer, int length)
{
    return fwrite(buffer, sizeof(char), length, file);
}

int file_delete(char path[MAX_PATH_LENGTH])
{
    return remove(path);
}

int file_clear_dir(char *path)
{
    DIR *dir;
    struct dirent *entry;
    char entry_path[MAX_PATH_LENGTH];

    dir = opendir(path);

    if(dir)
    {
        while((entry = readdir(dir)) != NULL)
        {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            file_path(path, entry->d_name, entry_path, MAX_PATH_LENGTH);

            if(unlink(entry_path) != 0)
            {
                return -1;
            }

        }

        closedir(dir);

        return 0;
    }

    return -1;
}

int file_path(char* dir, char* file, char *dest, int length)
{
    bzero((void *)dest, length);

    strcat(dest, dir);
    strcat(dest, "/");
    strcat(dest, file);

    return 0;
}

int file_print(char *path)
{

    FILE *file = NULL;

    file = fopen(path, "rb");
    FILE_TEMP file_temp;

    log_debug("DEBUGGG", "entrou no file print!!!");

    while(fread(&file_temp, sizeof(file_temp), 1, file) == 1)
    {
        if(strcmp(file_temp.file_name,"Diretorio vazio.") == 0)
        {  
            printf("\n\t%s\n\n", "!!! EMPTY DIR !!!");            
        } else 
        {
            printf("M: %s | A: %s | C: %s | '%s'\n", file_temp.file_mac.m, file_temp.file_mac.a, file_temp.file_mac.c, file_temp.file_name);
        }
    }

    return 0;


}
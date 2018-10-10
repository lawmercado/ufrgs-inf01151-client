#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client.h"
#include "file.h"
#include "comm.h"
#include "sync.h"
#include "log.h"

int __exit()
{
    sync_watcher_stop();
    sync_stop();
    comm_stop();

    return 0;
}

int __handle_input(char* input)
{
    char command[MAX_COMMAND_LENGTH];
    char file[MAX_FILENAME_LENGTH];

    bzero((void *) command, MAX_COMMAND_LENGTH);
    bzero((void *) file, MAX_FILENAME_LENGTH);

    sscanf(input, "%s %[^\n\t]s", command, file);

    if(strcmp(command, "upload") == 0)
    {
        if( strlen(file) > 0 )
        {
            upload(file);
        }
        else
        {
            log_error("client", "upload usage: upload <file_path>");
        }
    }
    else if(strcmp(command, "download") == 0)
    {
        if( strlen(file) > 0 )
        {
            download(file);
        }
        else
        {
            log_error("client", "download usage: download <file_name>");
        }
    }
    else if(strcmp(command, "delete") == 0)
    {
        if( strlen(file) > 0 )
        {
            delete(file);
        }
        else
        {
            log_error("client", "delete usage: delete <file_name>");
        }
    }
    else if(strcmp(command, "list_server") == 0)
    {
        list_server();
    }
    else if(strcmp(command, "list_client") == 0)
    {
        list_client();
    }
    else if(strcmp(command, "get_sync_dir") == 0)
    {
        get_sync_dir();
    }
    else if(strcmp(command, "exit") == 0)
    {
        return -1;
    }
    else
    {
        log_error("client", "Unknown command");
    }

    return 0;
}

int main(int argc, char** argv)
{
    char input[MAX_INPUT_LENGTH];

    if(argc != 4)
    {
    	log_error("client", "Usage: ./client <username> <host> <port>");

    	exit(1);
  	}

    fprintf(stderr, "Initializing...\n");

    if(comm_init(argv[1], argv[2], atoi(argv[3])) != 0)
    {
        exit(1);
    }

    sync_setup("sync_dir");

    if(sync_watcher_init("sync_dir") != 0)
    {
        exit(1);
    }

    get_sync_dir();

    if(sync_init() != 0)
    {
        exit(1);
    }

    do
    {
        fprintf(stderr, "%s@%s# ", argv[1], argv[2]);
        fgets(input, sizeof(input), stdin);

    } while(__handle_input(input) == 0);
    
    __exit();

    return 0;
}

void upload(char *file)
{
    if(file_exists(file))
    {
        fprintf(stderr, "Uploading... ");
        if(comm_upload(file) == 0)
        {
            fprintf(stderr, "Done!\n");
        }
        else
        {
            fprintf(stderr, "Error\n");
        }
    }
    else
    {
        fprintf(stderr, "File does not exist\n");
    }
}

void download(char *file)
{
    fprintf(stderr, "Downloading... ");

    if(comm_download(file, ".") == 0)
    {
        fprintf(stderr, "Done!\n");
    }
}

void delete(char *file)
{
    if(comm_delete(file) == 0)
    {
        if(sync_delete_file(file) == 0)
        {
            fprintf(stderr, "File deleted!\n");
        }
    }
}

void list_server()
{
    comm_list_server();
}

void list_client()
{
    sync_list_files();
}

void get_sync_dir()
{
    fprintf(stderr, "Syncing your directory... ");
    sync_watcher_stop();
    comm_get_sync_dir();
    sync_watcher_init("sync_dir");
    fprintf(stderr, "Done!\n");
}

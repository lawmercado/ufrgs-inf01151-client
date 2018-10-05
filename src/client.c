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

    sscanf(input, "%s %s", command, file);

    //log_debug("client", "Command read '%s', file '%s'", command, file);

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
        // TODO: here we should kill all threads (communication and synchronization related threads) and logout
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

    if(comm_init(argv[1], argv[2], atoi(argv[3])) != 0)
    {
        exit(1);
    }



    if(sync_init("sync_dir") != 0)
    {
        exit(1);
    }
    else
    {
        if(watch_sync_init("sync_dir") != 0)
        {
            exit(1);
        }

    }
    
    //get_sync_dir();

    do
    {
        printf("\nInsert a command: ");
        fgets(input, sizeof(input), stdin);

    } while(__handle_input(input) == 0);

    __exit();

    return 0;
}

void upload(char *file)
{
    comm_upload(file);
}

void download(char *file)
{
    comm_download(file);
}

void delete(char *file)
{

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
    comm_get_sync_dir();
}

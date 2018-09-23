#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "client.h"
#include "comm.h"
#include "sync.h"
#include "log.h"

int main(int argc, char** argv)
{
    char input[MAX_INPUT_LENGTH];
    char command[MAX_COMMAND_LENGTH];
    char file[MAX_FILENAME_LENGTH];

    if( argc != 3 )
    {
    	log_error("client", "Usage: ./client <host> <port>");

    	exit(1);
  	}

    // TODO: Enter this loop only after sucessfull login

    while(1)
    {
        fgets(input, sizeof(input), stdin);

        bzero(command, sizeof(command));
        bzero(file, sizeof(file));

        sscanf(input, "%s %s", command, file);

        log_debug("client", "Command read '%s', file '%s'", command, file);

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

            break;
        }
        else
        {
            log_error("client", "Unknown command");
        }
    }

    return 0;
}

void upload(char *file)
{

}

void download(char *file)
{

}

void delete(char *file)
{

}

void list_server()
{

}

void list_client()
{

}

void get_sync_dir()
{

}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "client.h"
#include "file.h"
#include "comm.h"
#include "sync.h"
#include "log.h"
#include "utils.h"

struct comm_entity __server_entity;
struct comm_entity __frontend_entity;
pthread_t __frontend_thread;

int __exit()
{
    sync_watcher_stop();
    comm_logout();

    return 0;
}

int __handle_input(char* input)
{
    char command[MAX_COMMAND_LENGTH];
    char file[FILE_NAME_LENGTH];

    bzero((void *) command, MAX_COMMAND_LENGTH);
    bzero((void *) file, FILE_NAME_LENGTH);

    sscanf(input, "%s %[^\n\t]s", command, file);

    if(strcmp(command, "upload") == 0)
    {
        if( strlen(file) > 0 )
        {
            client_upload(file);
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
            client_download(file);
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
            client_delete(file);
        }
        else
        {
            log_error("client", "delete usage: delete <file_name>");
        }
    }
    else if(strcmp(command, "list_server") == 0)
    {
        client_list_server();
    }
    else if(strcmp(command, "list_client") == 0)
    {
        client_list_client();
    }
    else if(strcmp(command, "get_sync_dir") == 0)
    {
        client_get_sync_dir();
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

    if(client_setup(argv[1], argv[2], atoi(argv[3])) != 0)
    {
        exit(1);
    }

    client_get_sync_dir();

    do
    {
        fprintf(stderr, "%s@%s# ", argv[1], argv[2]);
        fgets(input, sizeof(input), stdin);

    } while(__handle_input(input) == 0);
    
    __exit();

    return 0;
}

void client_upload(char *file)
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

void client_download(char *file)
{
    fprintf(stderr, "Downloading... ");

    if(comm_download(file, ".") == 0)
    {
        fprintf(stderr, "Done!\n");
    }
}

void client_delete(char *file)
{
    if(sync_delete_file(file) == 0)
    {
        if(comm_delete(file) == 0)
        {   
            fprintf(stderr, "File deleted!\n");
        }
    }
}

void client_list_server()
{
    comm_list_server();
}

void client_list_client()
{
    sync_list_files();
}

void client_get_sync_dir()
{
    fprintf(stderr, "Syncing your directory... ");
    comm_get_sync_dir();
    fprintf(stderr, "Done!\n");
}

int __handle_command(char *command)
{
    char operation[COMM_COMMAND_LENGTH], parameter[COMM_PARAMETER_LENGTH], parameter2[COMM_PARAMETER_LENGTH];

    sscanf(command, "%s %s %s", operation, parameter, parameter2);

    log_info("comm", "Command read '%s %s %s'", operation, parameter, parameter2);

    if(strcmp(operation, "synchronize") == 0)
    {
        return comm_response_synchronize(parameter);
    }
    else if(strcmp(operation, "delete") == 0)
    {
        return sync_delete_file(parameter);
    }
    else if(strcmp(operation, "recon") == 0)
    {
        int new_port = atoi(parameter2);

        utils_init_sockaddr_to_host(&(__server_entity.sockaddr), new_port, parameter);

        return comm_init(__server_entity, __frontend_entity);
    }

    return 0;
}

void *__receive()
{
    while(1)
    {
        char command[COMM_COMMAND_LENGTH] = "";

        bzero(command, COMM_COMMAND_LENGTH);

        if(comm_receive_command(&__frontend_entity, command) == 0)
        {
            __handle_command(command);
        }
    }
}

int __setup_receiver()
{
    utils_init_sockaddr(&(__frontend_entity.sockaddr), 0, INADDR_ANY);
    socklen_t length = sizeof(__frontend_entity.sockaddr);

    __frontend_entity.socket_instance = utils_create_binded_socket(&(__frontend_entity.sockaddr));
    __frontend_entity.idx_buffer = -1;

    if(__frontend_entity.socket_instance == -1)
    {
        return -1;
    }

    getsockname(__frontend_entity.socket_instance, (struct sockaddr *)&(__frontend_entity.sockaddr), &length);

    log_info("comm", "Client controller is socket %d %d", __frontend_entity.socket_instance, utils_get_port((struct sockaddr *)&(__frontend_entity.sockaddr)));

    if(pthread_create(&__frontend_thread, NULL, __receive, NULL) != 0)
    {
        log_error("comm", "Could not create the receiver thread");
    }

    return 0;
}

int __setup_sender(char* username, char *host, int port)
{
    int tmp_port = port;
    int new_port;

    if((__server_entity.socket_instance = utils_create_socket()) == -1)
    {
		return -1;
	}

    if(utils_init_sockaddr_to_host(&(__server_entity.sockaddr), tmp_port, host) == -1)
    {
        return -1;
    }

    new_port = comm_login(&(__server_entity), username, utils_get_port((struct sockaddr *)&(__frontend_entity.sockaddr)));

    if(new_port != -1)
    {
        utils_init_sockaddr_to_host(&(__server_entity.sockaddr), new_port, host);

        return 0;
    }

    close(__server_entity.socket_instance);

    return -1;
}

int client_setup(char* username, char *host, int port)
{
    if(__setup_receiver() == -1)
    {
        return -1;
    }

    if(__setup_sender(username, host, port) == -1)
    {
        return -1;
    }

    comm_init(__server_entity, __frontend_entity);

    if(sync_setup("sync_dir") == -1)
    {
        return -1;
    }
    
    if(sync_watcher_init() == -1)
    {
        return -1;
    }

    return 0;
}
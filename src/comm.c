#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <math.h>
#include <dirent.h>
#include <pthread.h>
#include "comm.h"
#include "sync.h"
#include "log.h"
#include "file.h"

pthread_mutex_t __command_handling_mutex = PTHREAD_MUTEX_INITIALIZER;

int __socket_instance;
struct sockaddr_in __server_sockaddr;

int __send_packet(struct sockaddr_in *server_sockaddr, struct comm_packet *packet);
int __receive_packet(struct sockaddr_in *server_sockaddr, struct comm_packet *packet);
int __send_ack(struct sockaddr_in *server_sockaddr);
int __receive_ack(struct sockaddr_in *server_sockaddr);
int __send_data(struct sockaddr_in *server_sockaddr, struct comm_packet *packet);
int __receive_data(struct sockaddr_in *server_sockaddr, struct comm_packet *packet);
int __send_command(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH]);
int __receive_command(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH]);
int __send_file(struct sockaddr_in *server_sockaddr, char path[FILE_PATH_LENGTH]);
int __receive_file(struct sockaddr_in *server_sockaddr, char path[FILE_PATH_LENGTH]);

int __command_run(COMM_COMMAND command, struct comm_command_args *args)
{
    pthread_mutex_lock(&__command_handling_mutex);

    int result = command(args);

    pthread_mutex_unlock(&__command_handling_mutex);

    return result;
}

int __command_download(struct comm_command_args *args)
{
    char download_command[COMM_PPAYLOAD_LENGTH];
    char path[FILE_PATH_LENGTH];
    char *file = args->fileName;
    char *dest = args->receivePath;

    sprintf(download_command, "download %s", file);

    file_path(dest, file, path, FILE_PATH_LENGTH);

    if(__send_command(&__server_sockaddr, download_command) == 0)
    {
        if(__receive_file(&__server_sockaddr, path) == 0)
        {
            log_debug("comm", "'%s' downloaded", file);

            return 0;
        }
    }

    return -1;
}

int comm_download(char *file, char *dest)
{
    COMM_COMMAND command = __command_download;
    struct comm_command_args args;
    strncpy(args.fileName, file, FILE_NAME_LENGTH);
    strncpy(args.receivePath, dest, FILE_PATH_LENGTH);
    bzero(args.sendPath, FILE_PATH_LENGTH);

    return __command_run(command, &args);
}

int __command_upload(struct comm_command_args *args)
{
    char upload_command[COMM_PPAYLOAD_LENGTH];
    char filename[FILE_NAME_LENGTH];
    char *path = args->sendPath;

    if(!file_exists(path))
    {
        return -1;
    }

    if(file_get_name_from_path(path, filename) == -1)
    {
        strcpy(filename, path);
    }

    sprintf(upload_command, "upload %s", filename);

    if(__send_command(&__server_sockaddr, upload_command) == 0)
    {
        if(__send_file(&__server_sockaddr, path) == 0)
        {
            log_debug("comm", "'%s' uploaded into '%s'", path, filename);

            return 0;
        }
    }

    return -1;
}

int comm_upload(char *path)
{
    COMM_COMMAND command = __command_upload;
    struct comm_command_args args;
    bzero(args.fileName, FILE_NAME_LENGTH);
    bzero(args.receivePath, FILE_PATH_LENGTH);
    strncpy(args.sendPath, path, FILE_PATH_LENGTH);

    return __command_run(command, &args);
}

int __command_delete(struct comm_command_args *args)
{
    char delete_command[COMM_PPAYLOAD_LENGTH];
    char *file = args->fileName;

    bzero(delete_command, COMM_PPAYLOAD_LENGTH);
    sprintf(delete_command, "delete %s", file);

    if(__send_command(&__server_sockaddr, delete_command) == 0)
    {
        log_debug("comm", "'%s' removed", file);

        return 0;
    }

    return -1;
}

int comm_delete(char *file)
{
    COMM_COMMAND command = __command_delete;
    struct comm_command_args args;
    strncpy(args.fileName, file, FILE_NAME_LENGTH);
    bzero(args.receivePath, FILE_PATH_LENGTH);
    bzero(args.sendPath, FILE_PATH_LENGTH);

    return __command_run(command, &args);
}

int __command_check_sync(struct comm_command_args *args)
{
    char sync_command[COMM_PPAYLOAD_LENGTH];
    bzero(sync_command, COMM_PPAYLOAD_LENGTH);
    strcpy(sync_command, "synchronize");

    char command[COMM_PPAYLOAD_LENGTH];
    char operation[COMM_COMMAND_LENGTH];
    char file[FILE_NAME_LENGTH];
    bzero(command, COMM_PPAYLOAD_LENGTH);
    bzero(operation, COMM_COMMAND_LENGTH);
    bzero(file, FILE_NAME_LENGTH);
    struct comm_packet packet;

    if(__send_command(&__server_sockaddr, sync_command) != 0)
    {
        log_error("comm", "Could not send the synchronize command!");
    }
    else
    {
        if(__receive_data(&__server_sockaddr, &packet) == 0)
        {
            if(strlen(packet.payload) > 6)
            {
                strcpy(command, packet.payload);
                
                sscanf(command, "%s %[^\n\t]s", operation, file);
                if(strcmp(operation, "download") == 0)
                {
                    struct comm_command_args downloadArgs;
                    strncpy(downloadArgs.fileName, file, FILE_NAME_LENGTH);
                    strncpy(downloadArgs.receivePath, "./sync_dir", FILE_PATH_LENGTH);
                    bzero(downloadArgs.sendPath, FILE_PATH_LENGTH);

                    log_info("comm", "Found file to download");

                    sync_watcher_stop();
                    __command_download(&downloadArgs);
                    sync_watcher_init("sync_dir");
                }
                else if(strcmp(operation, "delete") == 0)
                {
                    log_info("comm", "Found file to delete");

                    sync_delete_file(file);
                }

                return 0;
            }

            log_debug("comm", "No file to sync!");
        }
    }

    return -1;
}

int comm_check_sync()
{
    COMM_COMMAND command = __command_check_sync;

    return __command_run(command, NULL);
}

int __command_download_all_dir(char *temp_file)
{
    FILE *fp;
    char str[FILENAME_MAX];

    /* opening file for reading */
    fp = fopen(temp_file , "r");

    if(fp == NULL)
    {
        perror("fopen");
        return(-1);
    }

    while(fgets(str, FILENAME_MAX, fp)!=NULL )
    {
        if(str[strlen(str) - 1] == '\n')
        {
            str[strlen(str) - 1] = '\0';
        }

        log_debug("comm", "Starting '%s' download...", str);

        if(strcmp(str, "DiretorioVazio") == 0)
        {
            file_delete(str);
            return 0;
        }

        struct comm_command_args downloadArgs;
        strncpy(downloadArgs.fileName, str, FILE_NAME_LENGTH);
        strncpy(downloadArgs.receivePath, "./sync_dir", FILE_PATH_LENGTH);
        bzero(downloadArgs.sendPath, FILE_PATH_LENGTH);

        sync_watcher_stop();
        __command_download(&downloadArgs);
        sync_watcher_init("./sync_dir");
    }

    fclose(fp);

    return 0;
}

int __command_get_sync_dir(struct comm_command_args *args)
{
    char get_sync_dir_command[COMM_PPAYLOAD_LENGTH] = "get_sync_dir";

    if(__send_command(&__server_sockaddr, get_sync_dir_command) != 0)
    {
        log_error("comm", "Send command get_sync_dir error!");
    }
    else
    {
        char temp_file[FILE_NAME_LENGTH];
        bzero(temp_file, FILE_NAME_LENGTH);
        strcat(temp_file, "temp.txt");

        if(__receive_file(&__server_sockaddr, temp_file) == 0)
        {
            log_debug("comm", "'%s' downloaded", temp_file);

            __command_download_all_dir(temp_file);

            file_delete(temp_file);

            return 0;
        }
    }

    return -1;
}

int comm_get_sync_dir()
{
    COMM_COMMAND command = __command_get_sync_dir;

    return __command_run(command, NULL);
}

int __command_list_server(struct comm_command_args *args)
{
    char list_server_command[COMM_PPAYLOAD_LENGTH] = "list_server";

    if(__send_command(&__server_sockaddr, list_server_command) == 0)
    {
        char temp_file[FILE_NAME_LENGTH] = "temp.txt";

        if(__receive_file(&__server_sockaddr, temp_file) == 0)
        {
            log_debug("comm", "'%s' downloaded", temp_file);

            FILE *file = NULL;

            file = fopen(temp_file, "rb");
            struct file_status file_temp;

            while(fread(&file_temp, sizeof(file_temp), 1, file) == 1)
            {
                if(strcmp(file_temp.file_name, "DiretorioVazio") != 0)
                {
                    printf("M: %s | A: %s | C: %s | '%s'\n", file_temp.file_mac.m, file_temp.file_mac.a, file_temp.file_mac.c, file_temp.file_name);
                }
            }

            file_delete(temp_file);

            return 0;
        }
    }

    return -1;
}

int comm_list_server()
{
    COMM_COMMAND command = __command_list_server;

    return __command_run(command, NULL);
}

void __server_init_sockaddr(struct sockaddr_in *server_sockaddr, struct hostent *server, int port)
{
    server_sockaddr->sin_family = AF_INET;
	server_sockaddr->sin_port = htons(port);
	server_sockaddr->sin_addr = *((struct in_addr *) server->h_addr);
	bzero((void *) &(server_sockaddr->sin_zero), sizeof(server_sockaddr->sin_zero));
}

int __login(struct sockaddr_in *tmp_sockaddr, char* username)
{
    char login_command[COMM_PPAYLOAD_LENGTH];
    struct comm_packet packet;
    int port;

    sprintf(login_command, "login %s", username);

    if(__send_command(tmp_sockaddr, login_command) == 0)
    {
        if(__receive_data(tmp_sockaddr, &packet) == 0)
        {
            port = atoi(packet.payload);

            log_debug("comm", "Client port %d", port);

            return port;
        }
    }

    return -1;
}

int comm_init(char* username, char *host, int port)
{
    struct sockaddr_in tmp_sockaddr;
	struct hostent *hostent;
    int tmp_port = port;
    int new_port;

    if((__socket_instance = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
		log_error("comm", "Could not create socket instance");

        return -1;
	}

	if((hostent = gethostbyname(host)) == NULL)
    {
        log_error("comm", "Could not find the '%s' host", host);

        close(__socket_instance);
        return -1;
    }

	__server_init_sockaddr(&tmp_sockaddr, hostent, tmp_port);

    new_port = __login(&tmp_sockaddr, username);

    if(new_port != -1)
    {
        __server_init_sockaddr(&__server_sockaddr, hostent, new_port);

        return 0;
    }

    close(__socket_instance);

    return -1;
}

int __command_logout(struct comm_command_args *args)
{
    char logout_command[COMM_PPAYLOAD_LENGTH];

    bzero(logout_command, COMM_PPAYLOAD_LENGTH);
    sprintf(logout_command, "logout");

    if(__send_command(&__server_sockaddr, logout_command) == 0)
    {
        close(__socket_instance);

        return 0;
    }

    return -1;
}

int comm_stop()
{
    COMM_COMMAND command = __command_logout;

    return __command_run(command, NULL);
}

int __send_packet(struct sockaddr_in *server_sockaddr, struct comm_packet *packet)
{
    int status;

    status = sendto(__socket_instance, (void *)packet, sizeof(struct comm_packet), 0, (struct sockaddr *)server_sockaddr, sizeof(struct sockaddr));

    if(status < 0)
    {
        return -1;
    }

    return 0;
}

int __receive_packet(struct sockaddr_in *server_sockaddr, struct comm_packet *packet)
{
    int status;
    struct sockaddr_in from;
    socklen_t from_length = sizeof(struct sockaddr_in);
    struct pollfd fd;
    int res;

    fd.fd = __socket_instance;
    fd.events = POLLIN;

    res = poll(&fd, 1, COMM_TIMEOUT);
    if(res == 0)
    {
        log_error("comm", "Connection timed out");

        return COMM_ERROR_TIMEOUT;
    }
    else if(res == -1)
    {
        log_error("comm", "Polling error");

        return -1;
    }
    else
    {
        status = recvfrom(__socket_instance, (void *)packet, sizeof(*packet), 0, (struct sockaddr *)&from, &from_length);
        
        if(status < 0)
        {
            return -1;
        }

        return 0;
    }
}

int __send_ack(struct sockaddr_in *server_sockaddr)
{
    log_debug("comm", "Sending ack!");

    struct comm_packet packet;

    packet.type = COMM_PTYPE_ACK;
    bzero(packet.payload, COMM_PPAYLOAD_LENGTH);
    packet.seqn = 0;
    packet.length = 0;
    packet.total_size = 1;

    if(__send_packet(server_sockaddr, &packet) != 0)
    {
        log_error("comm", "Ack could not be sent!");

        return -1;
    }

    return 0;
}

int __receive_ack(struct sockaddr_in *server_sockaddr)
{
    log_debug("comm", "Receiving ack!");

    struct comm_packet packet;
    int status = __receive_packet(server_sockaddr, &packet);

    if(status == 0)
    {
        if(packet.type != COMM_PTYPE_ACK)
        {
            log_error("comm", "The received packet is not an ack.");

            return -1;
        }
    }

    return status;
}

int __send_data(struct sockaddr_in *server_sockaddr, struct comm_packet *packet)
{
    log_debug("comm", "Sending data!");

    packet->type = COMM_PTYPE_DATA;

    if(__send_packet(server_sockaddr, packet) != 0)
    {
        log_error("comm", "Data could not be sent. Trying again!");

        return -1;
    }
    
    int status = __receive_ack(server_sockaddr);

    while(status == COMM_ERROR_TIMEOUT)
    {
        log_error("comm", "Data was not received by the destinatary. Trying again!");
        __send_packet(server_sockaddr, packet);
        status = __receive_ack(server_sockaddr);
    }

    return status;
}

int __receive_data(struct sockaddr_in *server_sockaddr, struct comm_packet *packet)
{
    log_debug("comm", "Receiving data!");

    int status = __receive_packet(server_sockaddr, packet);
    int isValidPacket = packet->type == COMM_PTYPE_DATA;

    while(status == COMM_ERROR_TIMEOUT || !isValidPacket)
    {
        log_error("comm", "Not received valid packet. Trying again!");

        status = __receive_packet(server_sockaddr, packet);
        
        if(status == 0)
        {
            if(packet->type == COMM_PTYPE_DATA)
            {
                isValidPacket = 1;
            }
            else
            {
                log_error("comm", "The received packet is not data. Trying again!");

                isValidPacket = 0;
            }
        }
        else if(status == -1)
        {
            return -1;
        }
    }
    
    return __send_ack(server_sockaddr);
}

int __send_command(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH])
{
    log_debug("comm", "Sending command!");

    struct comm_packet packet;

    packet.type = COMM_PTYPE_CMD;
    packet.length = strlen(buffer);
    packet.seqn = 0;
    packet.total_size = 1;
    bzero(packet.payload, COMM_PPAYLOAD_LENGTH);
    strncpy(packet.payload, buffer, strlen(buffer));

    if(__send_packet(server_sockaddr, &packet) != 0)
    {
        log_error("comm", "Command could not be sent. Trying again!");

        return -1;
    }
    
    int status = __receive_ack(server_sockaddr);

    while(status == COMM_ERROR_TIMEOUT)
    {
        log_error("comm", "Command was not received by the destinatary. Trying again!");
        __send_packet(server_sockaddr, &packet);
        status = __receive_ack(server_sockaddr);
    }

    return status;
}

int __receive_command(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH])
{
    log_debug("comm", "Receiving command!");

    struct comm_packet packet;

    int status = __receive_packet(server_sockaddr, &packet);
    int isValidPacket = packet.type == COMM_PTYPE_CMD;

    while(status == COMM_ERROR_TIMEOUT || !isValidPacket)
    {
        log_error("comm", "Not received valid packet. Trying again!");

        status = __receive_packet(server_sockaddr, &packet);

        if(status == 0)
        {
            if(packet.type == COMM_PTYPE_CMD)
            {
                isValidPacket = 1;
            }
            else
            {
                log_error("comm", "The received packet is not command. Trying again!");

                isValidPacket = 0;
            }
        }
        else if(status == -1)
        {
            return -1;
        }
    }

    bzero(buffer, COMM_PPAYLOAD_LENGTH);
    strncpy(buffer, packet.payload, packet.length);

    return __send_ack(server_sockaddr);
}

int __send_file(struct sockaddr_in *sockaddr, char path[FILE_PATH_LENGTH])
{
    FILE *file = NULL;
    int i;

    file = fopen(path, "rb");

    if(file == NULL)
    {
        log_error("comm", "Could not open the file in '%s'", path);

        return -1;
    }

    int num_packets = (int) ceil(file_size(path) / (float) COMM_PPAYLOAD_LENGTH);

    struct comm_packet packet;

    for(i = 0; i < num_packets; i++)
    {
        int length = file_read_bytes(file, packet.payload, COMM_PPAYLOAD_LENGTH);

        packet.length = length;
        packet.total_size = num_packets;
        packet.seqn = i;

        __send_data(sockaddr, &packet);
    }

    fclose(file);
    return 0;
}

int __receive_file(struct sockaddr_in *sockaddr, char path[FILE_PATH_LENGTH])
{
    FILE *file = NULL;
    int i;
    struct comm_packet packet;

    file = fopen(path, "wb");

    if(file == NULL)
    {
        log_error("comm", "Could not open the file in '%s'", path);

        return -1;
    }

    if(__receive_data(sockaddr, &packet) == 0)
    {
        file_write_bytes(file, packet.payload, packet.length);

        for(i = 1; i < packet.total_size; i++)
        {
            __receive_data(sockaddr, &packet);
            file_write_bytes(file, packet.payload, packet.length);
        }

        fclose(file);

        return 0;
    }

    fclose(file);

    return -1;
}

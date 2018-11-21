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

struct comm_entity __server_entity;
struct comm_entity __frontend_entity;

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

    if(comm_send_command(&(__server_entity), download_command) == 0)
    {
        if(comm_receive_file(&(__server_entity), path) == 0)
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

    if(comm_send_command(&(__server_entity), upload_command) == 0)
    {
        if(comm_send_file(&(__server_entity), path) == 0)
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

    if(comm_send_command(&(__server_entity), delete_command) == 0)
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
        sync_watcher_init();
    }

    fclose(fp);

    return 0;
}

int __command_get_sync_dir(struct comm_command_args *args)
{
    char get_sync_dir_command[COMM_PPAYLOAD_LENGTH] = "get_sync_dir";

    if(comm_send_command(&(__server_entity), get_sync_dir_command) != 0)
    {
        log_error("comm", "Send command get_sync_dir error!");
    }
    else
    {
        char temp_file[FILE_NAME_LENGTH];
        bzero(temp_file, FILE_NAME_LENGTH);
        strcat(temp_file, "temp.txt");

        if(comm_receive_file(&(__server_entity), temp_file) == 0)
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

    if(comm_send_command(&(__server_entity), list_server_command) == 0)
    {
        char temp_file[FILE_NAME_LENGTH] = "temp.txt";

        if(comm_receive_file(&(__server_entity), temp_file) == 0)
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

int __command_logout(struct comm_command_args *args)
{
    char logout_command[COMM_PPAYLOAD_LENGTH];

    bzero(logout_command, COMM_PPAYLOAD_LENGTH);
    sprintf(logout_command, "logout");

    if(comm_send_command(&(__server_entity), logout_command) == 0)
    {
        close(__server_entity.socket_instance);

        return 0;
    }

    return -1;
}

int comm_logout()
{
    COMM_COMMAND command = __command_logout;

    return __command_run(command, NULL);
}

int comm_login(struct comm_entity *entity, char* username, int port)
{
    char login_command[COMM_PPAYLOAD_LENGTH];
    struct comm_packet packet;
    int server_port;

    sprintf(login_command, "login %s %d", username, port);

    if(comm_send_command(entity, login_command) == 0)
    {
        if(comm_receive_data(entity, &packet) == 0)
        {
            server_port = atoi(packet.payload);

            log_debug("comm", "Client port %d", server_port);

            return server_port;
        }
    }

    return -1;
}

int __command_response_synchronize(struct comm_command_args *args)
{
    char path[FILE_PATH_LENGTH];
    char *file = args->fileName;
    char *dest = args->receivePath;

    file_path(dest, file, path, FILE_PATH_LENGTH);
    
    sync_watcher_stop();

    if(comm_receive_file(&(__frontend_entity), path) == 0)
    {
        log_debug("comm", "'%s' synchronized", file);

        sync_watcher_init();

        return 0;
    }

    sync_watcher_init();

    return -1;
}

int comm_response_synchronize(char *file)
{
    COMM_COMMAND command = __command_response_synchronize;
    struct comm_command_args args;
    strncpy(args.fileName, file, FILE_NAME_LENGTH);
    bzero(args.sendPath, FILE_PATH_LENGTH);
    strncpy(args.receivePath, "sync_dir", FILE_PATH_LENGTH);

    return __command_run(command, &args);
}

int comm_init(struct comm_entity server_entity, struct comm_entity receiver_entity)
{
    pthread_mutex_lock(&__command_handling_mutex);
    __server_entity = server_entity;
    __frontend_entity = receiver_entity;
    pthread_mutex_unlock(&__command_handling_mutex);

    return 0;
}

int __send_packet(struct comm_entity *to, struct comm_packet *packet)
{
    int status = sendto(to->socket_instance, (void *)packet, sizeof(struct comm_packet), 0, (struct sockaddr *)&(to->sockaddr), sizeof(struct sockaddr));

    if(status < 0)
    {
        log_error("comm", "Socket: %d, No packet could be sent", to->socket_instance);

        return -1;
    }

    return 0;
}

int __receive_packet(struct comm_entity *to, struct comm_packet *packet)
{
    struct pollfd fd;
    fd.fd = to->socket_instance;
    fd.events = POLLIN;

    int poll_status = poll(&fd, 1, COMM_TIMEOUT);

    if(poll_status == 0)
    {
        log_debug("comm", "Socket: %d, Connection timed out", to->socket_instance);

        return COMM_ERROR_TIMEOUT;
    }
    else if(poll_status == -1)
    {
        log_error("comm", "Socket: %d, Polling error");

        return -1;
    }
    else
    {
        socklen_t from_length = sizeof(to->sockaddr);
        int status = recvfrom(to->socket_instance, (void *)packet, sizeof(*packet), 0, (struct sockaddr *)&(to->sockaddr), &from_length);

        if(status < 0)
        {
            log_error("comm", "Socket: %d, No packet was received");

            return -1;
        }

        return 0;
    }
}

int __send_ack(struct comm_entity *to)
{
    log_debug("comm", "Socket: %d, Sending ack!", to->socket_instance);

    struct comm_packet packet;

    packet.type = COMM_PTYPE_ACK;
    bzero(packet.payload, COMM_PPAYLOAD_LENGTH);
    packet.seqn = 0;
    packet.length = 0;
    packet.total_size = 1;

    if(__send_packet(to, &packet) != 0)
    {
        log_error("comm", "Socket: %d, Ack could not be sent!", to->socket_instance);

        return -1;
    }

    return 0;
}

int __receive_ack(struct comm_entity *from)
{
    log_debug("comm", "Socket: %d, Receiving ack!", from->socket_instance);

    struct comm_packet packet;
    int status = __receive_packet(from, &packet);

    if(status == 0)
    {
        if(packet.type != COMM_PTYPE_ACK)
        {
            log_error("comm", "Socket: %d, The received packet is not an ack.", from->socket_instance);

            return -1;
        }
    }

    return status;
}

int __is_packet_already_in_buffer(struct comm_entity *entity, struct comm_packet *packet)
{
    struct comm_packet *last_packet = &(entity->buffer[entity->idx_buffer]);

    return
    (
        (last_packet->type == packet->type) &&
        (last_packet->seqn == packet->seqn) &&
        (last_packet->length == packet->length) &&
        (last_packet->total_size == packet->total_size) &&
        (strcmp(last_packet->payload, packet->payload) == 0)
    );
}

int __save_packet_in_buffer(struct comm_entity *entity, struct comm_packet *packet)
{
    if(entity->idx_buffer == (COMM_RECEIVE_BUFFER_LENGTH - 1))
    {
        log_error("comm", "Socket %d, buffer is full!", entity->socket_instance);

        return -1;
    }
    
    entity->idx_buffer = entity->idx_buffer + 1;
    entity->buffer[entity->idx_buffer] = *packet;

    return 0;
 }

int __get_packet_in_buffer(struct comm_entity *entity, struct comm_packet *packet)
{
    if(entity->idx_buffer == -1)
    {
        log_error("comm", "Socket %d, buffer is empty!", entity->socket_instance);

        return -1;
    }
    
    *packet = entity->buffer[entity->idx_buffer];
    entity->idx_buffer = entity->idx_buffer - 1;

    return 0;
}

int __reliable_send_packet(struct comm_entity *to, struct comm_packet *packet)
{
    int status = -1;
    int ack_status = -1;

    do
    {
        status = __send_packet(to, packet);

        if(status != 0)
        {
            return -1;
        }

        ack_status = __receive_ack(to);

    } while(ack_status == COMM_ERROR_TIMEOUT);

    return ack_status;
}

int __reliable_receive_packet(struct comm_entity *from)
{
    struct comm_packet packet;
    int status = -1;
    int timeout_count = 0;

    do
    {
        status = __receive_packet(from, &packet);

        if(status == COMM_ERROR_TIMEOUT)
        {
            timeout_count++;

            if(timeout_count == COMM_MAX_TIMEOUTS)
            {
                return -1;
            }
        }
    } while(status == COMM_ERROR_TIMEOUT);

    if(!__is_packet_already_in_buffer(from, &packet))
    {
        if(__save_packet_in_buffer(from, &packet) == 0)
        {
            if(__send_ack(from) == 0)
            {
                return 0;
            }
        }
    }
    else
    {
        if(__send_ack(from) == 0)
        {
            return 0;
        }
    }

    return -1;
}

int comm_send_data(struct comm_entity *to, struct comm_packet *packet)
{
    log_debug("comm", "Socket: %d, Sending data!", to->socket_instance);

    packet->type = COMM_PTYPE_DATA;

    if(__reliable_send_packet(to, packet) != 0)
    {
        log_error("comm", "Socket: %d, Data could not be sent.", to->socket_instance);

        return -1;
    }

    return 0;
}

int comm_receive_data(struct comm_entity *from, struct comm_packet *packet)
{
    log_debug("comm", "Socket: %d, Receiving data!", from->socket_instance);

    if(__reliable_receive_packet(from) != 0)
    {
        log_error("comm", "Socket: %d, No data could be received.", from->socket_instance);

        return -1;
    }

    if(__get_packet_in_buffer(from, packet) != 0)
    {
        return -1;
    }

    if(packet->type != COMM_PTYPE_DATA)
    {
        log_error("comm", "Socket: %d, The received packet is not data.", from->socket_instance);

        return -1;
    }

    return 0;
}

int comm_send_command(struct comm_entity *to, char buffer[COMM_PPAYLOAD_LENGTH])
{
    log_debug("comm", "Socket: %d, Sending command!", to->socket_instance);

    struct comm_packet packet;

    packet.type = COMM_PTYPE_CMD;
    packet.length = strlen(buffer);
    packet.seqn = 0;
    packet.total_size = 1;
    bzero(packet.payload, COMM_PPAYLOAD_LENGTH);
    strncpy(packet.payload, buffer, strlen(buffer));

    if(__reliable_send_packet(to, &packet) != 0)
    {
        log_error("comm", "Socket: %d, Command could not be sent.", to->socket_instance);

        return -1;
    }

    return 0;
}

int comm_receive_command(struct comm_entity *from, char buffer[COMM_PPAYLOAD_LENGTH])
{
    log_debug("comm", "Socket: %d, Receiving command!", from->socket_instance);

    struct comm_packet packet;

    if(__reliable_receive_packet(from) != 0)
    {
        log_debug("comm", "Socket: %d, No command could be received.", from->socket_instance);

        return -1;
    }

    if(__get_packet_in_buffer(from, &packet) != 0)
    {
        return -1;
    }

    if(packet.type != COMM_PTYPE_CMD)
    {
        log_error("comm", "Socket: %d, The received packet is not command.", from->socket_instance);

        return -1;
    }

    bzero(buffer, COMM_PPAYLOAD_LENGTH);
    strncpy(buffer, packet.payload, packet.length);

    return 0;
}

int comm_send_file(struct comm_entity *to, char path[FILE_PATH_LENGTH])
{
    FILE *file = NULL;
    int i;

    file = fopen(path, "rb");

    if(file == NULL)
    {
        log_error("comm", "Socket: %d, Could not open the file in '%s'", to->socket_instance, path);

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

        comm_send_data(to, &packet);
    }

    fclose(file);
    return 0;
}

int comm_receive_file(struct comm_entity *from, char path[FILE_PATH_LENGTH])
{
    FILE *file = NULL;
    int i;
    struct comm_packet packet;

    file = fopen(path, "wb");

    if(file == NULL)
    {
        log_error("comm", "Socket: %d, Could not open the file in '%s'", from->socket_instance, path);

        return -1;
    }

    if(comm_receive_data(from, &packet) == 0)
    {
        file_write_bytes(file, packet.payload, packet.length);

        for(i = 1; i < packet.total_size; i++)
        {
            comm_receive_data(from, &packet);
            file_write_bytes(file, packet.payload, packet.length);
        }

        fclose(file);

        return 0;
    }

    fclose(file);

    return -1;
}
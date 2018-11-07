#ifndef __COMM_H__
#define __COMM_H__

#include <netdb.h>
#include "file.h"

#define COMM_TIMEOUT 20000
#define COMM_ERROR_TIMEOUT -2
#define COMM_RECEIVE_BUFFER_LENGTH 10

#define COMM_PPAYLOAD_LENGTH 512
#define COMM_PTYPE_DATA 0
#define COMM_PTYPE_CMD 1
#define COMM_PTYPE_ACK 2

#define COMM_COMMAND_LENGTH 64

struct comm_packet {
    uint16_t type; // Packet type (COMM_PTYPE_*)
    uint16_t seqn; // Sequence number
    uint32_t total_size; // Number of fragments
    uint16_t length; // Payload length
    char payload[COMM_PPAYLOAD_LENGTH];
};

struct comm_entity {
    int socket_instance;
    struct sockaddr_in *sockaddr;
    struct comm_packet buffer[COMM_RECEIVE_BUFFER_LENGTH];
    int idx_buffer;
};

struct comm_command_args {
    char fileName[FILE_NAME_LENGTH];
    char receivePath[FILE_PATH_LENGTH];
    char sendPath[FILE_PATH_LENGTH];
};

typedef int (*COMM_COMMAND)(struct comm_command_args*);

int comm_init(char* username, char *host, int port);

int comm_download(char *file, char *dest);

int comm_upload(char *path);

int comm_delete(char *file);

int comm_list_server();

int comm_get_sync_dir();

int comm_stop();

int comm_check_sync();

#endif

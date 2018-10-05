#ifndef __COMM_H__
#define __COMM_H__

#include <netdb.h>

#define COMM_TIMEOUT 20000
#define COMM_PPAYLOAD_LENGTH 256
#define COMM_PTYPE_DATA 0
#define COMM_PTYPE_CMD 1
#define COMM_PTYPE_ACK 2
#define COMM_TIMEOUT_ERROR 2

struct comm_packet {
    uint16_t type; // Packet type (COMM_PTYPE_*)
    uint16_t seqn; // Sequence number
    uint32_t total_size; // Number of fragments
    uint16_t length; // Payload length
    char payload[COMM_PPAYLOAD_LENGTH];
};

int comm_init(char* username, char *host, int port);

int comm_download(char *file, char *dest);

int comm_upload(char *path);

int comm_list_server();

int comm_get_sync_dir();

int comm_stop();

#endif

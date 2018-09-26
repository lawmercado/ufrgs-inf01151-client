#ifndef __COMM_H__
#define __COMM_H__

#include <netdb.h>

#define COMM_TIMEOUT 5000
#define COMM_PPAYLOAD_LENGTH 256
#define COMM_PTYPE_DATA 0
#define COMM_PTYPE_CMD 1
#define COMM_PTYPE_ACK 2

struct comm_packet {
    uint16_t type; // Packet type (COMM_PTYPE_*)
    uint16_t seqn; // Sequence number
    uint32_t total_size; // Number of fragments
    uint16_t length; // Payload length
    char payload[COMM_PPAYLOAD_LENGTH];
};

int comm_init(char* username, char *host, int port);

#endif

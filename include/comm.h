#ifndef __COMM_H__

#define __COMM_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 256
#define MAXIDSIZE 25

typedef struct frame {
    int id_msg;
    int ack;
    char buffer[BUFFER_SIZE];
    char user[MAXIDSIZE];
} Frame;

void send_file();

void receive_file();

void delete_file();

void list_server();

int send_cmd();

int receive_cmd();

int send_state();

int receive_state();

int send_packet();

int receive_packet();

int login(char *host, char *port);

int logout(int sockfd);

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

int comm_init(char* username, char *host, char* port);

#endif

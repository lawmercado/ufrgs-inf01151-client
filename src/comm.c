#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "comm.h"
#include "log.h"

int __socket_instance;
struct sockaddr_in __server_sockaddr;

int __login(char* username, struct sockaddr_in *tmp_server_address);
int __upload(char* path);
int __download(char* path);

void __server_init_sockaddr(struct sockaddr_in *server_sockaddr, struct hostent *server, int port);

int __send_packet(struct sockaddr_in *server_sockaddr, struct comm_packet *packet);
int __receive_packet(struct sockaddr_in *server_sockaddr, struct comm_packet *packet);
int __send_ack(struct sockaddr_in *server_sockaddr);
int __receive_ack(struct sockaddr_in *server_sockaddr);
int __send_data(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH]);
int __receive_data(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH]);
int __send_command(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH]);
int __receive_command(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH]);

int comm_init(char* username, char *host, int port)
{
    struct sockaddr_in tmp_server_address;
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

	__server_init_sockaddr(&tmp_server_address, hostent, tmp_port);

    new_port = __login(username, &tmp_server_address);

    if(new_port != -1)
    {
        __server_init_sockaddr(&__server_sockaddr, hostent, new_port);

        return 0;
    }

    close(__socket_instance);
    return -1;
}

void __server_init_sockaddr(struct sockaddr_in *server_sockaddr, struct hostent *server, int port)
{
    server_sockaddr->sin_family = AF_INET;
	server_sockaddr->sin_port = htons(port);
	server_sockaddr->sin_addr = *((struct in_addr *) server->h_addr);
	bzero((void *) &(server_sockaddr->sin_zero), sizeof(server_sockaddr->sin_zero));
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
    /*
    TODO: compare this with the server address to check origin
    char clienthost[100];
    char clientport[100];
    int result = getnameinfo(&from, length, clienthost, sizeof(clienthost), clientport, sizeof (clientport), NI_NUMERICHOST | NI_NUMERICSERV);

    if(result == 0)
    {
        if (from.sa_family == AF_INET)
            printf("Received from %s %s\n", clienthost, clientport);
    }*/

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

        return -1;
    }
    else if(res == -1)
    {
        log_error("comm", "Polling error");

        return -1;
    }
    else
    {
        // Receives an ack from the server
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
    log_debug("comm", "Sending ack");

    struct comm_packet packet;

    packet.type = COMM_PTYPE_ACK;
    bzero(packet.payload, COMM_PPAYLOAD_LENGTH);

    if(__send_packet(server_sockaddr, &packet) != 0)
    {
        log_error("comm", "Ack could not be sent");

        return -1;
    }

    return 0;
}

int __receive_ack(struct sockaddr_in *server_sockaddr)
{
    log_debug("comm", "Receiving ack");

    struct comm_packet packet;

    if(__receive_packet(server_sockaddr, &packet) != 0)
    {
        log_error("comm", "Could not receive an ack");

        return -1;
    }

    if(packet.type != COMM_PTYPE_ACK)
    {
        log_error("comm", "The received packet is not an ack");

        return -1;
    }

    return 0;
}

int __send_data(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH])
{
    log_debug("comm", "Sending data");

    struct comm_packet packet;

    packet.type = COMM_PTYPE_DATA;
    packet.length = strlen(buffer);
    bzero(packet.payload, COMM_PPAYLOAD_LENGTH);
    strncpy(packet.payload, buffer, strlen(buffer));

    if(__send_packet(server_sockaddr, &packet) != 0)
    {
        log_error("comm", "Data could not be sent");

        return -1;
    }

    return __receive_ack(server_sockaddr);
}

int __receive_data(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH])
{
    log_debug("comm", "Receiving data");

    struct comm_packet packet;

    if(__receive_packet(server_sockaddr, &packet) != 0)
    {
        log_error("comm", "Could not receive the data");

        return -1;
    }

    if( packet.type != COMM_PTYPE_DATA )
    {
        log_error("comm", "The received packet is not a data");

        return -1;
    }

    bzero(buffer, COMM_PPAYLOAD_LENGTH);
    strncpy(buffer, packet.payload, packet.length);

    return __send_ack(server_sockaddr);
}

int __send_command(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH])
{
    log_debug("comm", "Sending command");

    struct comm_packet packet;

    packet.type = COMM_PTYPE_CMD;
    packet.length = strlen(buffer);
    bzero(packet.payload, COMM_PPAYLOAD_LENGTH);
    strncpy(packet.payload, buffer, strlen(buffer));

    if(__send_packet(server_sockaddr, &packet) != 0)
    {
        log_error("comm", "Command '%s' could not be sent", buffer);

        return -1;
    }

    return __receive_ack(server_sockaddr);
}

int __receive_command(struct sockaddr_in *server_sockaddr, char buffer[COMM_PPAYLOAD_LENGTH])
{
    log_debug("comm", "Receiving command");

    struct comm_packet packet;

    if(__receive_packet(server_sockaddr, &packet) != 0)
    {
        log_error("comm", "Could not receive the command");

        return -1;
    }

    if( packet.type != COMM_PTYPE_CMD )
    {
        log_error("comm", "The received packet is not a command");

        return -1;
    }

    bzero(buffer, COMM_PPAYLOAD_LENGTH);
    strncpy(buffer, packet.payload, packet.length);

    return __send_ack(server_sockaddr);
}

int __login(char* username, struct sockaddr_in *tmp_server_address)
{
    char login_command[COMM_PPAYLOAD_LENGTH];
    char buffer[COMM_PPAYLOAD_LENGTH];
    int port;

    sprintf(login_command, "login %s", username);

    if(__send_command(tmp_server_address, login_command) == 0)
    {
        if(__receive_data(tmp_server_address, buffer) == 0)
        {
            port = atoi(buffer);

            log_debug("comm", "Client port %d", port);

            return port;
        }
    }

    return -1;
}

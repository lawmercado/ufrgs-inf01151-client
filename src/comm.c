#include "comm.h"

int logout(int sockfd){

	close(sockfd);
	return 0;
}

void __create_socket(struct sockaddr_in *serv_addr, int port, struct hostent *server){

	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(port);
	serv_addr->sin_addr = *((struct in_addr *) server->h_addr);

	//TODO VERIFICAR ESSE BZERO
	bzero((char *) &serv_addr, sizeof(serv_addr));

}

int __send_packet(TextMessage *newMsg, int sockfd, struct sockaddr_in *serv){

	/*
	ssize_t sendto (
		int s,				=> descriptor for socket
		const void * msg,	=> pointer to the message that we want to send
		size_t len, 		=> length of message
		int flags			=>
		const struct sockaddr *to > pointer to a sockaddr object that specifies the addess of the target
		socklen_t tolen 	=> object that specifies the size of the target
	)

	*/
	int status = 0;
	int ack = 0;

	socklen_t tolen = sizeof(struct sockaddr_in);

	while(ack != 1){
		
		if(sendto(sockfd, newMsg, sizeof(*newMsg), 0, (const struct sockaddr *) serv, sizeof(struct sockaddr_in)) < 0){
			printf("\n[Error sendto]: Sending packet fault!\n");
			return -1;
		}
		else{
			printf("\n[Sendto ok]: Sending packet: socket id: %d!\n", sockfd);
		}

		if(recvfrom(sockfd, newMsg, sizeof(*newMsg), 0, (struct sockaddr *) serv, &tolen) < 2){

			printf("\n[Error recvfrom]: Receiving ack fault!\n");
			return -2;
		}
		else
		{
			printf("\n[Recvfrom ok]: Received ack: socket id: %d!\n", sockfd);
		}

		ack = 1;
	}

	printf("Got an ack: %s\n", newMsg->buffer);

	return 0;
}

int __receive_packet(int *id_msg, char *buffer, int sockfd, struct sockaddr_in *serv){

	/*
	ssize_t sendto (
		int s,				=> descriptor for socket
		const void * msg,	=> pointer to the message that we want to send
		size_t len, 		=> length of message
		int flags			=> 
		const struct sockaddr *to > pointer to a sockaddr object that specifies the addess of the target
		socklen_t tolen 	=> object that specifies the size of the target
	)

	struct Frame {	
    	int id_msg;
    	int ack;
    	char buffer[256];
    	char user[25];
	};
	*/

	Frame packet;
	int status = 0;

	socklen_t fromlen = sizeof(struct sockaddr_in);

	do{
		status = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) serv, &fromlen);
		if(status < 0){
			printf("\n[Error recvfrom]: Receiving packet fault!\n");
			return -1;
		}
		else{
			printf("\n[Recvfrom ok]: Receiving packet: socket id: %d, msg id: %d!\n", sockfd, packet.id_msg);
		}

		if(packet.id_msg == *id_msg){
			packet.ack = 1;
		}

		status = sendto(sockfd, &packet, sizeof(packet), 0, (const struct sockaddr *) serv, sizeof(struct sockaddr_in));
		if(status < 2){

			printf("\n[Error sendto]: Sending ack fault!\n");
			return -2;
		}

		packet.ack = 1;
	}
	while(packet.ack != 1);

	printf("Got an ack: %s\n", buffer);
	

	*id_msg = *id_msg +1;
	return 0;

}

int __send_msg(int sockfd, struct sockaddr_in serv_addr, struct sockaddr_in from){

	TextMessage newMsg;

	bzero(newMsg.buffer, BUFFER_SIZE);
	strcpy(newMsg.buffer, "CLIENT COMMUNICATION");

	if(__send_packet(&newMsg, sockfd, &serv_addr) < 0){
		printf("\nERROR start sending packet!!\n");
		return -1;
	}

	return 0;

}

int login(char *server_raw, char *port_raw){

    int sockfd;
	struct sockaddr_in serv_addr, from;
	struct hostent *server;

	int port = atoi(port_raw);

	if ((server = gethostbyname(server_raw)) == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

	//socket (int domain, int type, int protocol)
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("ERROR opening socket");
        exit(0);
	}
	else{
		printf("\nFirst client socket %d\n", sockfd);
	}

	__create_socket(&serv_addr, port, server);

	if(__send_msg(sockfd, serv_addr, from) < 0){
		printf("[Error sending message]\n");
	}
		
	return sockfd;

}

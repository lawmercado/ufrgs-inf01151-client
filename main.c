#include "communication.h"


int main(int argc, char *argv[])
{

	if(argc != 3){
    	fprintf(stderr, "Usage: client host port\n");
    	exit(1);
  	}


    int sockfd = login(argv[1], argv[2]);
	logout(sockfd);
}
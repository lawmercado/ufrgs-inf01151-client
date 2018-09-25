#include "comm.h"

int main(int argc, char *argv[])
{
	if(argc != 3){
    	fprintf(stderr, "Usage: ./client username host port\n");
    	exit(1);
  	}

    printf("COMM INIT %d\n", comm_init(argv[1], argv[2], atoi(argv[3])));
}

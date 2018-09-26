#include <stdio.h>
#include <stdlib.h>
#include "comm.h"
#include "file.h"

int main(int argc, char *argv[])
{
	if(argc != 4){
    	fprintf(stderr, "Usage: ./client username host port\n");
    	exit(1);
  	}

	printf("COMM INIT %d\n", comm_init(argv[1], argv[2], atoi(argv[3])));

    comm_download("etapa2.tgz");
}

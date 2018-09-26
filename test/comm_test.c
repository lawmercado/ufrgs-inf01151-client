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

	char filename[256];
	file_get_name_from_path("sync_dir/narigudo/textodofrigo.txt", filename);
	printf("%s\n", filename);
    printf("COMM INIT %d\n", comm_init(argv[1], argv[2], atoi(argv[3])));
}

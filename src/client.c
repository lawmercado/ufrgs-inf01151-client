#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if( argc != 3 )
    {
    	printf("Usage: ./client <host> <port>\n");

    	exit(1);
  	}

    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include "sync.h"

int main(int argc, char** argv)
{
    if( sync_init("sync_dir") == 0 )
    {
        sleep(20);

        sync_stop();
    }
}

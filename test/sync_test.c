#include <stdio.h>
#include <unistd.h>
#include "sync.h"

int main(int argc, char** argv)
{
    if( sync_init("sync_dir") == 0 )
    {
        sleep(20);

        sync_list_files();

        sync_update_file("test_write.txt", "I'm a writed file by the 'sync_update_file' function", 52);

        sync_list_files();

        sync_stop();
    }
}

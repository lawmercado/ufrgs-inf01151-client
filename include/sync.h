#ifndef __SYNC_H__
#define __SYNC_H__

#include "file.h"

#define SYNC_READ_TIMEOUT 1000

int sync_setup(char *dir_path);

int sync_watcher_init();

void sync_watcher_stop();

int sync_delete_file(char name[FILE_NAME_LENGTH]);

int sync_list_files();

#endif

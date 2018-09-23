#ifndef __CLIENT___
#define __CLIENT___

#define MAX_INPUT_LENGTH 400
#define MAX_COMMAND_LENGTH 12

void upload(char *file);

void download(char *file);

void delete(char *file);

void list_server();

void list_client();

void get_sync_dir();

#endif
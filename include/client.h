#ifndef __CLIENT_H__
#define __CLIENT_H__

#define MAX_INPUT_LENGTH 400
#define MAX_COMMAND_LENGTH 12

void client_upload(char *file);

void client_download(char *file);

void client_delete(char *file);

void client_list_server();

void client_list_client();

void client_get_sync_dir();

int client_setup();

#endif

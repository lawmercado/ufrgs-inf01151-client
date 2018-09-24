#ifndef __LOG_H___
#define __LOG_H___

#define MAX_MESSAGE_LENGTH 255

void log_debug(char* module_name, const char* message, ...);

void log_error(char* module_name, const char* message, ...);

#endif

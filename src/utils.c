#include <stdio.h>
#include <stdarg.h>

void utils_debug(char* module_name, const char* message)
{
    #ifdef DEBUG
        printf("DEBUG - %s:  %s\n", module_name, message);
    #endif
}

void utils_debug_fmt( char* module_name, const char* message, ...)
{
    char buf[200];

    va_list args;
    va_start(args, message);

    vsnprintf( buf, sizeof(buf), message, args );

    va_end(args);

    utils_debug(module_name, buf);
}

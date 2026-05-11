#pragma once

#include <stdarg.h>
#include <stdio.h>

struct FunctionReturnStatus{
    int error;
    char message[1024];

    static FunctionReturnStatus format(int error, const char* format, ...){
        FunctionReturnStatus status;
        status.error = error;
        va_list args;
        va_start(args, format);

        vsnprintf(status.message, sizeof(status.message), "Problema ao associar o socket com a porta %d.\n", args);
        va_end(args);

        return status;
    }
};

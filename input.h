#pragma once

#include <cstdio>

enum {
    _GET_LINE_MAX = 256;
}

typedef struct _InputBuffer {
    char        buffer[_GET_LINE_MAX];
    uint32_t    buffer_length;
    int32_t     input_length;
    _InputBuffer()
    {
        buffer[0] = '\0';
        buffer_length = _GET_LINE_MAX;
        input_length = 0;
    }
} InputBuffer;

void read_input(InputBuffer& input_buffer);

/*  Return  : length of line. 
            : -1, IF length of line is greater than buf_max
    
    Caution :  buf_max can't exceed the constant _GET_LINE_MAX */
int _get_line(FILE* stream, char* buf, int buf_max);

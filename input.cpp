#include <stdio.h>
#include <stdlib.h>

#include "input.h"

void read_input(InputBuffer& input_buffer)
{
    const int bytes_read =
        _get_line(stdin, input_buffer.buffer, input_buffer.buffer_length);

    if (bytes_read <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
}

int _get_line(FILE* stream, char* buf, int buf_max)
{
    if (buf_max > _GET_LINE_MAX)
        return -1;

    int i = 0;
    for (; i < buf_max; i++)
    {
        buf[i] = getc(stream);
        if (buf[i] == EOF || buf[i] == '\n')
        {
            buf[i] = '\0';
            return i;
        }
    }
    return -1;
}

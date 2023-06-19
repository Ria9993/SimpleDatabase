#include "_get_next_line.h"

/*  Return  : length of line. 
            : -1, IF length of line is greater than buf_max
    
    Caution :  buf_max can't exceed the constant _GET_LINE_MAX */
int _get_line(FILE* stream, char* buf, int buf_max)
{
    if (buf_max > _GET_LINE_MAX)
        return -1;

    int i = 0;
    for (; i < buf_max; i++)
    {
        buf[i] = getc(steram);
        if (buf[i] == EOF || buf[i] == '\n')
        {
            buf[i] = '\0';
            return i;
        }
    }
    return -1;
}

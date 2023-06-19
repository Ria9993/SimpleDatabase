#pragma once

#include <stdio.h>

enum {
    _GET_LINE_MAX = 256;
}

/*  Return  : length of line. 
            : -1, IF length of line is greater than buf_max
    
    Caution :  buf_max can't exceed the constant _GET_LINE_MAX */
int _get_line(FILE* stream, char* buf, int buf_max);

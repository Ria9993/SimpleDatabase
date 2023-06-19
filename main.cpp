#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "input.h"

#ifdef _WIN32
#elif __linux__
#endif

void print_prompt() { printf("db > "); }

int main(void)
{
    InputBuffer input_buffer;
    while (true)
    {
        print_prompt();
        read_input(input_buffer);

        if (strcmp(input_buffer.buffer, ".exit") == 0)
            exit(EXIT_SUCCESS);
        else
            printf("Unrecognized command '%s'.\n", input_buffer.buffer);
    }
    
    return 0;
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "input.h"

#ifdef _WIN32
#elif __linux__
#endif

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

MetaCommandResult do_meta_command(InputBuffer& input_buffer)
{
    if (strcmp(input_buffer.buffer, ".exit") == 0)
        exit(EXIT_SUCCESS);
    else
        return META_COMMAND_UNRECOGNIZED_COMMAND;
}

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct {
    StatementType type;
} Statement;

PrepareResult prepare_statement(InputBuffer& input_buffer, Statement& statement)
{
    if (strncmp(input_buffer.buffer, "insert", 6) == 0)
    {
        statement.type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if (strcmp(input_buffer.buffer, "select") == 0)
    {
        statement.type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement& statement)
{
    switch (statement.type)
    {
        case STATEMENT_INSERT:
            printf("TODO insert.\n");
            break;
        case STATEMENT_SELECT:
            printf("TODO select.\n");
            break;
    }
}

void print_prompt() { printf("db > "); }

int main(void)
{
    InputBuffer input_buffer;
    while (true)
    {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer.buffer[0] == '.')
        {
            switch (do_meta_command(input_buffer))
            {
                case META_COMMAND_SUCCESS:
                    continue;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    printf("Unrecognized command '%s'.\n", input_buffer.buffer);
                    continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, statement))
        {
            case PREPARE_SUCCESS:
                break;
            case PREPARE_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized keyword at start of '%s'.\n", input_buffer.buffer);
                continue;
        }
        
        execute_statement(statement);
        printf("Executed.\n");
    }
    
    return 0;
}

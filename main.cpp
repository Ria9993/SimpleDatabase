#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "input.h"

#ifdef _WIN32
#elif __linux__
#elif __APPLE__
#else
    #error "Unknown Platform"
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

/* Statement */
typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID,
    PREPARE_SYNTAX_ERROR
} PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

enum {
    COLUMN_USERNAME_SIZE = 32,
    COLUMN_EMAIL_SIZE = 255
};

typedef struct {
    uint32_t    id;
    char        username[COLUMN_USERNAME_SIZE + 1];
    char        email[COLUMN_EMAIL_SIZE + 1];
} Row;

void print_row(Row& row)
{
    printf("(%d, %s, %s)\n", row.id, row.username, row.email);
}

#include <cstddef>
#define attribute_sizeof(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

/* Row layout */
#define ID_SIZE         attribute_sizeof(Row, id)
#define USERNAME_SIZE   attribute_sizeof(Row, username)
#define EMAIL_SIZE      attribute_sizeof(Row, email)
    
#define ID_OFFSET       offsetof(Row, id)
#define USERNAME_OFFSET offsetof(Row, username)
#define EMAIL_OFFSET    offsetof(Row, email)

#define ROW_SIZE        sizeof(Row)

/* Table */

enum Table_layout {
    PAGE_SIZE       = 4096, //< Correspond OS page size
    TABLE_MAX_PAGES = 100,
    ROWS_PER_PAGE   = PAGE_SIZE / ROW_SIZE,
    TABLE_MAX_ROW   = ROWS_PER_PAGE * TABLE_MAX_PAGES
};

typedef struct _Table {
    uint32_t    num_rows;
    void*       pages[TABLE_MAX_PAGES];
    _Table() :
        num_rows(0)
    {
        num_rows = 0;
        for (int i = 0; i < TABLE_MAX_PAGES; i++)
            pages[i] = NULL;
    }
    ~_Table()
    {
        for (int i = 0; pages[i] != NULL; i++)
            free(pages[i]);
    }
} Table;

void* row_slot(Table& table, uint32_t row_num)
{
    const uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = table.pages[page_num];
    if (page == NULL)
    {
        table.pages[page_num] = malloc(PAGE_SIZE);
        page = table.pages[page_num];
    }
    
    const uint32_t row_offset = row_num % ROWS_PER_PAGE;
    const uint32_t byte_offset = row_offset * ROW_SIZE;
    return (char*)page + byte_offset;
}

/* Statement */

typedef struct {
    StatementType   type;
    Row             row_to_insert;
} Statement;

PrepareResult prepare_inseart(InputBuffer input_buffer, Statement& statement)
{
    statement.type = STATEMENT_INSERT;

    char* keyword   = strtok(input_buffer.buffer, " ");
    char* id_str    = strtok(NULL, " ");
    char* username  = strtok(NULL, " ");
    char* email     = strtok(NULL, " ");
    
    if (id_str == NULL || username == NULL || email == NULL)
        return PREPARE_SYNTAX_ERROR;

    int id = atoi(id_str);
    if (id < 0)
        return PREPARE_NEGATIVE_ID;
    if (strlen(username) > COLUMN_USERNAME_SIZE)
        return PREPARE_STRING_TOO_LONG;
    if (strlen(email) > COLUMN_EMAIL_SIZE)
        return PREPARE_STRING_TOO_LONG;
    
    statement.row_to_insert.id = id;
    strcpy(statement.row_to_insert.username, username);
    strcpy(statement.row_to_insert.email, email);

    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer& input_buffer, Statement& statement)
{
    if (strncmp(input_buffer.buffer, "insert", 6) == 0)
    {
        return prepare_inseart(input_buffer, statement);
    }
    if (strcmp(input_buffer.buffer, "select") == 0)
    {
        statement.type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void serialize_row(Row* source, void* dest)
{
    memcpy(dest, source, ROW_SIZE);
}

void deserialize_row(void* source, Row* dest)
{
    memcpy(dest, source, ROW_SIZE);
}

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;

ExecuteResult execute_insert(Statement& statement, Table& table);
ExecuteResult execute_select(Statement& statement, Table& table);

ExecuteResult execute_statement(Statement& statement, Table& table)
{
    switch (statement.type)
    {
        case STATEMENT_INSERT:
            return execute_insert(statement, table);
        case STATEMENT_SELECT:
            return execute_select(statement, table);
    }
}

ExecuteResult execute_insert(Statement& statement, Table& table)
{
    if (table.num_rows >= TABLE_MAX_ROW)
        return EXECUTE_TABLE_FULL;
    
    Row* row_to_insert = &(statement.row_to_insert);

    serialize_row(row_to_insert, row_slot(table, table.num_rows));
    table.num_rows += 1;
    
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement& statement, Table& table)
{
    Row row;
    for (uint32_t i = 0; i < table.num_rows; i++)
    {
        deserialize_row(row_slot(table, i), &row);
        print_row(row);
    }
    return EXECUTE_SUCCESS;
}

void print_prompt() { printf("db > "); }

int main(void)
{
    Table table;
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
            case PREPARE_STRING_TOO_LONG:
                printf("Strings is too long.\n");
                continue;
            case PREPARE_NEGATIVE_ID:
                printf("ID must be positive.\n");
                continue;
            case PREPARE_SYNTAX_ERROR:
                printf("Syntax error. Could not parse statement.\n");
                continue;
            case PREPARE_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized keyword at start of '%s'.\n", input_buffer.buffer);
                continue;
        }
        
        switch (execute_statement(statement, table))
        {
            case EXECUTE_SUCCESS:
                printf("Executed.\n");
                break;
            case EXECUTE_TABLE_FULL:
                printf("Error: Table full.\n");
                break;
        }
    }
    
    return 0;
}

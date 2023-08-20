#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <cassert>

#ifdef _WIN32
#elif __linux__
#elif __APPLE__
#else
    #error "Unknown Platform"
#endif

#include "input.h"
#include "LayoutDefine.h"
#include "Table.h"
#include "Cursor.h"
#include "B-tree.h"
#include "Serialize.h"

/* Statement */

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

void print_constants() {
    printf("ROW_SIZE: %d\n", (int)ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", (int)COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

void print_leaf_node(void* node) {
    uint32_t num_cells = *get_leaf_node_num_cells(node);
    printf("leaf (size %d)\n", num_cells);
    for (uint32_t i = 0; i < num_cells; i++) {
        uint32_t key  = *get_leaf_node_key(node, i);
        printf("  -  %d : %d\n", i, key);
    }
}

MetaCommandResult do_meta_command(InputBuffer& input_buffer, Table& table)
{
    if (strcmp(input_buffer.buffer, ".exit") == 0)
    {
        table.~Table();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(input_buffer.buffer, ".constants") == 0)
    {
        printf("Constants:\n");
        print_constants();
        return META_COMMAND_SUCCESS;
    }
    else if (strcmp(input_buffer.buffer, ".btree") == 0)
    {
        printf("Tree:\n");
        print_leaf_node(table.pager.get_page(0));
        return META_COMMAND_SUCCESS;
    }
    else
        return META_COMMAND_UNRECOGNIZED_COMMAND;
}

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

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY
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
    void* node = table.pager.get_page(table.root_page_num);
    uint32_t num_cells = *get_leaf_node_num_cells(node);

    Row* row_to_insert = &(statement.row_to_insert);
    uint32_t key_to_insert = row_to_insert->id;
    Cursor cursor(&table);
    cursor = table_find(table, key_to_insert);

    if (cursor.cell_num < num_cells) 
    {
        uint32_t key_at_index = *get_leaf_node_key(node, cursor.cell_num);
        if (key_at_index == key_to_insert)
            return EXECUTE_DUPLICATE_KEY;
    }
    cursor.insert_leaf_node(row_to_insert->id, row_to_insert);
    
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement& statement, Table& table)
{
    Cursor cursor(&table);
    Row row;
    while (cursor.end_of_table != true)
    {
        deserialize_row(&row, cursor.value());
        printf("(%u, %s, %s)\n", row.id, row.username, row.email);
        cursor.advance();
    }
    return EXECUTE_SUCCESS;
}

void print_prompt() { printf("db > "); }

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }

    const char* filename = argv[1];
    Table table(filename);
    InputBuffer input_buffer;
    while (true)
    {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer.buffer[0] == '.')
        {
            switch (do_meta_command(input_buffer, table))
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
            case EXECUTE_DUPLICATE_KEY:
                printf("Error: Duplicate key.\n");
                break;
        }
    }
    
    return 0;
}

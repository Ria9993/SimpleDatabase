#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cerrno>

#include "input.h"

#ifdef _WIN32
#elif __linux__
#elif __APPLE__
#else
    #error "Unknown Platform"
#endif

/* Table */

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

enum Table_layout {
    PAGE_SIZE       = 4096, //< Correspond OS page size
    TABLE_MAX_PAGES = 100,
    ROWS_PER_PAGE   = PAGE_SIZE / ROW_SIZE,
    TABLE_MAX_ROW   = ROWS_PER_PAGE * TABLE_MAX_PAGES
};

class Table;

class Pager {
    friend class Table;
public:
    FILE*       fs;
    uint32_t    file_length;
    int8_t*     pages[TABLE_MAX_PAGES];
private:
    Pager(const char* filename)
        : fs(NULL)
        , file_length(0)
    {
        fs = fopen(filename, "r+b");
        if (fs == NULL && errno == ENOENT)
        {
            fs = fopen(filename, "w");
            if (fs == NULL || fclose(fs) != 0)
            {
                printf("Error creating new file : %d\n", errno);
                exit(EXIT_FAILURE);
            }
            printf("Create new DB file : '%s'.\n", filename);
            fs = fopen(filename, "r+b");
        }
        if (fs == NULL) 
        {
            printf("Unable to open file : %d\n", errno);
            exit(EXIT_FAILURE);
        }

        if (fseek(fs, 0, SEEK_END) == -1)
        {
            printf("Error to fseek() : %d\n", errno);
            exit(EXIT_FAILURE);
        }
        file_length = ftell(fs);

        for (int i = 0; i < TABLE_MAX_PAGES; i++)
            pages[i] = NULL;
    }
    ~Pager()
    {
        // Table class should be responsible for pages memory free.
    }
};

void pager_flush(Pager& pager, uint32_t page_num, uint32_t bytes)
{
    if (pager.pages[page_num] == NULL)
    {
        printf("Tried to flush null page.\n");
        exit(EXIT_FAILURE);
    }

    if (fseek(pager.fs, page_num * PAGE_SIZE, SEEK_SET) == -1)
    {
        printf("Error to fseek() : %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = fwrite(pager.pages[page_num], sizeof(int8_t), bytes, pager.fs);
    if (bytes_written != bytes)
    {
        printf("Error to fwrite() : %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

class Table {
public:
    uint32_t    num_rows;
    Pager       pager;
    Table(const char* filename)
        : pager(filename)
    {
        num_rows = pager.file_length / ROW_SIZE;
    }
    ~Table()
    {
        const uint32_t num_full_pages = num_rows / ROWS_PER_PAGE;

        for (uint32_t i = 0; i < num_full_pages; i++)
        {
            if (pager.pages[i] == NULL)
                continue;
            pager_flush(pager, i, PAGE_SIZE);
            delete pager.pages[i];
            pager.pages[i] = NULL;
        }

        // There may be a partial page to write to the end of the file
        // This should not be needed after we switch to a B-tree.
        uint32_t num_additional_rows = num_rows % ROWS_PER_PAGE;
        if (num_additional_rows > 0)
        {
            uint32_t page_num = num_full_pages;
            if (pager.pages[page_num] != NULL)
            {
                pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
                delete pager.pages[page_num];
                pager.pages[page_num] = NULL;
            }
        }

        int result = fclose(pager.fs);
        if (result != 0)
        {
            printf("Error closing db file : %d\n", errno);
            exit(EXIT_FAILURE);
        }
        for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
        {
            if (pager.pages[i] != NULL)
            {
                delete pager.pages[i];
                pager.pages[i] = NULL;
            }
        }
    }
};

void* get_page(Pager& pager, uint32_t page_num)
{
    if (page_num >= TABLE_MAX_PAGES)
    {
        printf("Tried to fetch page number out of bounds. %d > %d\n"
            , page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }
    
    if (pager.pages[page_num] == NULL)
    {
        // Cache miss. Allocate memory and load from file
        void* page = new int8_t[PAGE_SIZE];
        uint32_t num_pages = pager.file_length / PAGE_SIZE;

        // We might save a partial page at the end of file
        if (pager.file_length % PAGE_SIZE != 0)
            num_pages += 1;
        
        if (page_num < num_pages)
        {
            fseek(pager.fs, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = fread(page, sizeof(int8_t), PAGE_SIZE, pager.fs);
            if (bytes_read == -1)
            {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        pager.pages[page_num] = (int8_t*)page;
    }
    return pager.pages[page_num];
}

// void* row_slot(Table& table, uint32_t row_num)
// {
//     const uint32_t page_num = row_num / ROWS_PER_PAGE;
//     void* const page = get_page(table.pager, page_num);
    
//     const uint32_t row_offset = row_num % ROWS_PER_PAGE;
//     const uint32_t byte_offset = row_offset * ROW_SIZE;
//     return (int8_t*)page + byte_offset;
// }

/* cursor */

class Cursor
{
public:
    Table*      table;
    uint32_t    row_num;
    bool        end_of_table; // Indicates a position one past the last element

    Cursor(Table* table0)
        : table(table0)
        , row_num(0)
        , end_of_table(table0->num_rows == 0)
    {
    }

    void set_table_start()
    {
        if (table == NULL)
            return;
        row_num = 0;
        end_of_table = (table->num_rows == 0);
    }
    void set_table_end()
    {
        if (table == NULL)
            return;
        row_num = table->num_rows;
        end_of_table = true;
    }
    void* value()
    {
        const uint32_t page_num = row_num / ROWS_PER_PAGE;
        void* page = get_page(table->pager, page_num);
        const uint32_t row_offset = row_num % ROWS_PER_PAGE;
        const uint32_t byte_offset = row_offset * ROW_SIZE;
        return (int8_t*)page + byte_offset;
    }
    void advance()
    {
        row_num += 1;
        if (row_num >= table->num_rows)
            end_of_table = true;
    }
};

/* Statement */

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

MetaCommandResult do_meta_command(InputBuffer& input_buffer, Table& table)
{
    if (strcmp(input_buffer.buffer, ".exit") == 0)
    {
        table.~Table();
        exit(EXIT_SUCCESS);
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
    Cursor cursor(&table);
    cursor.set_table_end();

    serialize_row(row_to_insert, cursor.value());
    table.num_rows += 1;
    
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement& statement, Table& table)
{
    Cursor cursor(&table);
    Row row;
    while (cursor.end_of_table != true)
    {
        deserialize_row(cursor.value(), &row);
        print_row(row);
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
        }
    }
    
    return 0;
}

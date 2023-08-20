#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <cassert>

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
    printf("(%u, %s, %s)\n", row.id, row.username, row.email);
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

#define PAGE_SIZE       (4096) //< Correspond OS page size
#define TABLE_MAX_PAGES (100)

/* B-TREE */
#include "B-tree.h"


void serialize_row(void* dest, Row* source)
{
    memcpy(dest, source, ROW_SIZE);
}

void deserialize_row(Row* dest, void* source)
{
    memcpy(dest, source, ROW_SIZE);
}

class Table;
class Pager {
    friend class Table;
public:
    FILE*       fs;
    uint32_t    file_length;
    uint32_t    num_pages;
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
        num_pages = (file_length / PAGE_SIZE);

        if (file_length % PAGE_SIZE != 0) {
            printf("DB file is not a whole number of pages. Corrupt file.\n");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < TABLE_MAX_PAGES; i++)
            pages[i] = NULL;
    }
    ~Pager()
    {
        // Table class should be responsible for memory free of pages
    }

public:
    void flush(uint32_t page_num)
    {
        if (pages[page_num] == NULL)
        {
            printf("Tried to flush null page.\n");
            exit(EXIT_FAILURE);
        }

        if (fseek(fs, page_num * PAGE_SIZE, SEEK_SET) == -1)
        {
            printf("Error to fseek() : %d\n", errno);
            exit(EXIT_FAILURE);
        }

        ssize_t bytes_written = fwrite(pages[page_num], sizeof(int8_t), PAGE_SIZE, fs);
        if (bytes_written != PAGE_SIZE)
        {
            printf("Error to fwrite() : %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }

    void* get_page(uint32_t page_num)
    {
        if (page_num >= TABLE_MAX_PAGES)
        {
            printf("Tried to fetch page number out of bounds. %u > %d\n",
                page_num, TABLE_MAX_PAGES);
            exit(EXIT_FAILURE);
        }
        
        if (pages[page_num] == NULL)
        {
            // Cache miss. Allocate memory and load from file
            void* page = new int8_t[PAGE_SIZE];
            uint32_t num_file_pages = file_length / PAGE_SIZE;

            // We might save a partial page at the end of file
            if (file_length % PAGE_SIZE != 0)
                num_file_pages += 1;
            
            if (page_num < num_file_pages)
            {
                fseek(fs, page_num * PAGE_SIZE, SEEK_SET);
                ssize_t bytes_read = fread(page, sizeof(int8_t), PAGE_SIZE, fs);
                if (bytes_read == -1)
                {
                    printf("Error reading file: %d\n", errno);
                    exit(EXIT_FAILURE);
                }
            }

            pages[page_num] = (int8_t*)page;

            if (page_num >= num_pages) {
                num_pages = page_num + 1;
            }
        }
        return pages[page_num];
    }
};

class Table {
public:
    Pager       pager;
    uint32_t    root_page_num;
    Table(const char* filename)
        : pager(filename)
        , root_page_num(0)
    {
        if (pager.num_pages == 0) {
            // New database file. Initialize page 0 as leaf node.
            void* root_node = pager.get_page(0);
            initialize_leaf_node(root_node);
        }
    }
    ~Table()
    {
        for (uint32_t i = 0; i < pager.num_pages; i++)
        {
            if (pager.pages[i] == NULL)
                continue;
            pager.flush(i);
            delete pager.pages[i];
            pager.pages[i] = NULL;
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
    uint32_t    page_num;
    uint32_t    cell_num;
    bool        end_of_table; // Indicates a position one past the last element

    Cursor(Table* table0)
        : table(table0)
        , page_num(table->root_page_num)
        , cell_num(0)
    {
        assert(table0 != NULL);
        set_table_start();
    }

    void set_table_start()
    {
        page_num = table->root_page_num;
        cell_num = 0;
        void*       root_node = table->pager.get_page(table->root_page_num);
        uint32_t    num_cells = *get_leaf_node_num_cells(root_node);
        end_of_table = (num_cells == 0);
    }
    /*  Set the position of the given key.
        If the key is not present, set the position
        where it should be insreted */
    void find(uint32_t key)
    {
        uint32_t root_page_num = table->root_page_num;
        void* root_node = table->pager.get_page(root_page_num);

        if (get_node_type(root_node) == NODE_LEAF) 
        {
            return search_leaf_node(root_page_num, key);    
        }
        else
        {
            printf("Need to implement searching an internal node\n");
            exit(EXIT_FAILURE);
        }
    }

    void search_leaf_node(uint32_t page_num0, uint32_t key)
    {
        void* node = table->pager.get_page(page_num0);
        uint32_t num_cells = *get_leaf_node_num_cells(node);
        page_num = page_num0;

        // Binary search
        uint32_t min_index = 0;
        uint32_t one_past_max_index = num_cells;
        while (min_index != one_past_max_index)
        {
            uint32_t index = (min_index + one_past_max_index) / 2;
            uint32_t key_at_index = *get_leaf_node_key(node, index);
            if (key == key_at_index)
            {
                cell_num = index;
                return;
            }
            if (key < key_at_index)
                one_past_max_index = index;
            else
                min_index = index + 1;
        }

        cell_num = min_index;
    }

    void* value()
    {
        void* page = table->pager.get_page(page_num);
        return (int8_t*)get_leaf_node_value(page, cell_num);
    }

    void advance()
    {
        void* node = table->pager.get_page(page_num);

        cell_num += 1;
        if (cell_num >= *get_leaf_node_num_cells(node)) {
            end_of_table = true;
        }
    }
    
    void insert_leaf_node(uint32_t key, Row* value)
    {
        void* node = table->pager.get_page(page_num);

        uint32_t num_cells = *get_leaf_node_num_cells(node);
        if (num_cells >= LEAF_NODE_MAX_CELLS) 
        {
            // Node full
            printf("Need to implement splitting a leaf node.\n");
            exit(EXIT_FAILURE);
        }

        if (cell_num < num_cells) 
        {
            // Make room for new cell
            for (uint32_t i = num_cells; i > cell_num; i--)
            {
                memcpy(get_leaf_node_cell(node, i)
                    , get_leaf_node_cell(node, i - 1)
                    , LEAF_NODE_CELL_SIZE);    
            }
        }

        *get_leaf_node_num_cells(node) += 1;
        *get_leaf_node_key(node, cell_num) = key;
        serialize_row(get_leaf_node_value(node, cell_num), value);
    }
};

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
    if (num_cells >= LEAF_NODE_MAX_CELLS)
        return EXECUTE_TABLE_FULL;
    
    Row* row_to_insert = &(statement.row_to_insert);
    uint32_t key_to_insert = row_to_insert->id;
    Cursor cursor(&table);
    cursor.find(key_to_insert);

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
            case EXECUTE_DUPLICATE_KEY:
                printf("Error: Duplicate key.\n");
                break;
        }
    }
    
    return 0;
}

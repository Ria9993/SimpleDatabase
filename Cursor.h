#pragma once

#include <cstdlib>
#include <cstdint>
#include "LayoutDefine.h"

class Table;

class Cursor
{
public:
    Table*      table;
    uint32_t    page_num;
    uint32_t    cell_num;
    bool        end_of_table; // Indicates a position one past the last element

    Cursor(Table* table0);

    void set_table_start();

    void* value();

    void advance();
    
    void insert_leaf_node(uint32_t key, Row* value);
};

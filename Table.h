#pragma once

#include <cstdint>
#include <cstdlib>
#include "Pager.h"

class Cursor;

class Table {
public:
    Pager       pager;
    uint32_t    root_page_num;

    Table(const char* filename);
    ~Table();

	/*  Set the position of the given key.
	If the key is not present, set the position
	where it should be insreted */
	Cursor table_find(uint32_t key);

	void insert_leaf_node(Cursor& cursor, uint32_t key, Row* value);
};

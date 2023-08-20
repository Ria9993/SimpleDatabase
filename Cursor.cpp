#include "Cursor.h"

#include <cassert>
#include <cstring>
#include "Table.h"
#include "Serialize.h"
#include "B-tree.h"

Cursor::Cursor(Table* table0)
	: table(table0)
	, page_num(table->root_page_num)
	, cell_num(0)
{
	assert(table0 != NULL);
	set_table_start();
}

void Cursor::set_table_start()
{
	page_num = table->root_page_num;
	cell_num = 0;
	void*       root_node = table->pager.get_page(table->root_page_num);
	uint32_t    num_cells = *get_leaf_node_num_cells(root_node);
	end_of_table = (num_cells == 0);
}

void* Cursor::value()
{
	void* page = table->pager.get_page(page_num);
	return (int8_t*)get_leaf_node_value(page, cell_num);
}

void Cursor::advance()
{
	void* node = table->pager.get_page(page_num);

	cell_num += 1;
	if (cell_num >= *get_leaf_node_num_cells(node)) {
		end_of_table = true;
	}
}

void Cursor::insert_leaf_node(uint32_t key, Row* value)
{
	void* node = table->pager.get_page(page_num);

	uint32_t num_cells = *get_leaf_node_num_cells(node);
	if (num_cells >= LEAF_NODE_MAX_CELLS) 
	{
		// Node full
		//split_and_insert_leaf_node(key, value);
		return;
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

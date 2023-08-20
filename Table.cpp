#include "Table.h"

#include <cstring>
#include "B-tree.h"
#include "LayoutDefine.h"
#include "Serialize.h"
#include "Cursor.h"

Table::Table(const char* filename)
	: pager(filename)
	, root_page_num(0)
{
	if (pager.num_pages == 0) {
		// New database file. Initialize page 0 as leaf node.
		void* root_node = pager.get_page(0);
		initialize_leaf_node(root_node);
	}
}

Table::~Table()
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

Cursor Table::table_find(uint32_t key)
{
	uint32_t root_page_num = root_page_num;
	void* root_node = pager.get_page(root_page_num);

	if (get_node_type(root_node) == NODE_LEAF) 
	{
		return leaf_node_find(*this, root_page_num, key);    
	}
	else
	{
		printf("Need to implement searching an internal node\n");
		exit(EXIT_FAILURE);
	}
}

void Table::insert_leaf_node(Cursor& cursor, uint32_t key, Row* value)
{
	void* node = pager.get_page(cursor.page_num);

	uint32_t num_cells = *get_leaf_node_num_cells(node);
	if (num_cells >= LEAF_NODE_MAX_CELLS) 
	{
		// Node full
		//split_and_insert_leaf_node(key, value);
		return;
	}

	if (cursor.cell_num < num_cells) 
	{
		// Make room for new cell
		for (uint32_t i = num_cells; i > cursor.cell_num; i--)
		{
			memcpy(get_leaf_node_cell(node, i)
				, get_leaf_node_cell(node, i - 1)
				, LEAF_NODE_CELL_SIZE);    
		}
	}

	*get_leaf_node_num_cells(node) += 1;
	*get_leaf_node_key(node, cursor.cell_num) = key;
	serialize_row(get_leaf_node_value(node, cursor.cell_num), value);
}


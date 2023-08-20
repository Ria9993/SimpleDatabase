#include "B-tree.h"

#include <cstdio>

uint32_t* get_leaf_node_num_cells(void* node) {
	return (uint32_t*)((char*)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

void* get_leaf_node_cell(void* node, uint32_t cell_num) {
	return (uint32_t*)((char*)node + LEAF_NODE_HEADER_SIZE + (cell_num * LEAF_NODE_CELL_SIZE)); 
}

uint32_t* get_leaf_node_key(void* node, uint32_t cell_num) {
	return (uint32_t*)get_leaf_node_cell(node, cell_num);
}

void* get_leaf_node_value(void* node, uint32_t cell_num) {
  return (char*)get_leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

NodeType get_node_type(void* node) {
	uint8_t value = *((uint8_t*)node + NODE_TYPE_OFFSET);
	return (NodeType)value;
}

void set_node_type(void* node, NodeType type) {
	*((uint8_t*)node + NODE_TYPE_OFFSET) = type;
}

void initialize_leaf_node(void* node) {
	set_node_type(node, NODE_LEAF);
	 *get_leaf_node_num_cells(node) = 0;
}

/*  Set the position of the given key.
	If the key is not present, set the position
	where it should be insreted */
Cursor table_find(Table& table, uint32_t key)
{
	uint32_t root_page_num = table.root_page_num;
	void* root_node = table.pager.get_page(root_page_num);

	if (get_node_type(root_node) == NODE_LEAF) 
	{
		return leaf_node_find(table, root_page_num, key);    
	}
	else
	{
		printf("Need to implement searching an internal node\n");
		exit(EXIT_FAILURE);
	}
}

Cursor leaf_node_find(Table& table, uint32_t page_num0, uint32_t key)
{
	void* node = table.pager.get_page(page_num0);
	uint32_t num_cells = *get_leaf_node_num_cells(node);

	Cursor cursor(&table);
	cursor.page_num = page_num0;

	// Binary search
	uint32_t min_index = 0;
	uint32_t one_past_max_index = num_cells;
	while (min_index != one_past_max_index)
	{
		uint32_t index = (min_index + one_past_max_index) / 2;
		uint32_t key_at_index = *get_leaf_node_key(node, index);
		if (key == key_at_index)
		{
			cursor.cell_num = index;
			return cursor;
		}
		if (key < key_at_index)
			one_past_max_index = index;
		else
			min_index = index + 1;
	}

	cursor.cell_num = min_index;
	return cursor;
}

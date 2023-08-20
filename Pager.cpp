#include "Pager.h"

Pager::Pager(const char* filename)
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

void Pager::flush(uint32_t page_num)
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

void* Pager::get_page(uint32_t page_num)
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

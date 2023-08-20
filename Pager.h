#pragma once

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include "LayoutDefine.h"

class Table;
class Pager {
    friend class Table;
public:
    FILE*       fs;
    uint32_t    file_length;
    uint32_t    num_pages;
    int8_t*     pages[TABLE_MAX_PAGES];
private:
    Pager(const char* filename);
    ~Pager(){} //< Table class should be responsible for memory free of pages

public:
    void flush(uint32_t page_num);

    void* get_page(uint32_t page_num);
};

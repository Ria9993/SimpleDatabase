#pragma once

#include <cstddef>
#include <cstdint>

#define attribute_sizeof(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

/* Table Define */
enum {
    COLUMN_USERNAME_SIZE = 32,
    COLUMN_EMAIL_SIZE = 255
};

typedef struct {
    uint32_t    id;
    char        username[COLUMN_USERNAME_SIZE + 1];
    char        email[COLUMN_EMAIL_SIZE + 1];
} Row;

/* 
 *	Row layout 
 */
#define ID_SIZE         attribute_sizeof(Row, id)
#define USERNAME_SIZE   attribute_sizeof(Row, username)
#define EMAIL_SIZE      attribute_sizeof(Row, email)
    
#define ID_OFFSET       offsetof(Row, id)
#define USERNAME_OFFSET offsetof(Row, username)
#define EMAIL_OFFSET    offsetof(Row, email)

#define ROW_SIZE        sizeof(Row)

#define PAGE_SIZE       (4096) //< Correspond OS page size
#define TABLE_MAX_PAGES (100)

#pragma once

#include <cstring>
#include "LayoutDefine.h"

inline void serialize_row(void* dest, Row* source)
{
    memcpy(dest, source, ROW_SIZE);
}

inline void deserialize_row(Row* dest, void* source)
{
    memcpy(dest, source, ROW_SIZE);
}

#include "pch.h"
#include "helpers.h"

static bool get_bit(BYTE source, size_t i)
{
    while (i > 0)
    {
        source <<= 1;
        --i;
    }
    return source & 0b1000'0000;
}

static BYTE set_bit(BYTE source, size_t i, bool value)
{
    BYTE mask = 0b1000'0000;
    while (i > 0)
    {
        mask >>= 1;
        --i;
    }
    if (value)
    {
        return source | mask;
    }
    else
    {
        return source & ~mask;
    }
}

bool get_bit(const std::vector<BYTE>& source, size_t i)
{
    return get_bit(source[i / 8], i % 8);
}

void set_bit(std::vector<BYTE>& dest, size_t i, bool value)
{
    while (dest.size() <= i / 8)
    {
        dest.push_back(0);
    }
    dest[i / 8] = set_bit(dest[i / 8], i % 8, value);
}

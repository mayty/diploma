#pragma once
#include <vector>
#include "Byte.h"

bool get_bit(const std::vector<BYTE>& source, size_t i);
void set_bit(std::vector<BYTE>& dest, size_t i, bool val);
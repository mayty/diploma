#include "pch.h"
#include "Decoder.h"
#include <vector>

std::vector<BYTE> g_data;
size_t d_size;


size_t create_decoder()
{
    return 0;
}

void destroy_decoder(size_t handle)
{
}

void feed_data_to_decoder(size_t handle, BYTE* data, size_t data_size)
{
    for (size_t i = 0; i < data_size / 8; ++i)
    {
        g_data.push_back(data[i]);
    }
    d_size = data_size / 8;
}

void decode_data(size_t handle)
{
}

size_t get_decoded_length(size_t handle)
{
    return d_size;
}

void get_decoded_data(size_t handle, BYTE* data_buf, size_t data_size)
{
    if (data_size < d_size)
    {
        throw std::exception{};
    }
    for (size_t i = 0; i < d_size; ++i)
    {
        data_buf[i] = g_data[i];
    }
}

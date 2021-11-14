#include "pch.h"
#include "Decoder.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include "decoding_machine.h"


static std::unordered_map<size_t, std::unique_ptr<decoding_machine>> decoders;
static size_t next_handle = 0;

std::vector<BYTE> g_data;
size_t d_size;


size_t create_decoder()
{
    decoders[next_handle] = std::make_unique<decoding_machine>();
    return next_handle++;
}

void destroy_decoder(size_t handle)
{
    decoders.erase(handle);
}

void decode_from_file(size_t handle, const char* filename)
{
    decoders[handle]->feed_data_from_file(std::string{ filename });
}


size_t get_decoded_length(size_t handle)
{
    return size_t();
}

size_t get_decoded_bits_count(size_t handle)
{
    return decoders[handle]->get_decoded_bits_count();
}

void get_decoded_data(size_t handle, BYTE* data_buf)
{
    auto encoded_data = decoders[handle]->get_decoded_data();
    for (size_t i = 0; i < encoded_data.size(); ++i)
    {
        data_buf[i] = encoded_data[i];
    }
}


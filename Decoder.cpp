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

void decode_file_to_file(size_t handle, const char* source_file, const char* destination_file)
{
    decoders[handle]->feed_data_from_file(std::string{ source_file });
    decoders[handle]->save_to_file(std::string{ destination_file });
}

size_t get_decoded_bits_count(size_t handle)
{
    return decoders[handle]->get_decoded_bits_count();
}

void get_decoded_data(size_t handle, BYTE* data_buf)
{
    auto decoded_data = decoders[handle]->get_decoded_data();
    for (size_t i = 0; i < decoded_data.size(); ++i)
    {
        data_buf[i] = decoded_data[i];
    }
}


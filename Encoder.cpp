#include "pch.h"
#include "Encoder.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include "encoding_machine.h"


static std::unordered_map<size_t, std::unique_ptr<encoding_machine>> encoders;
static size_t next_handle = 0;

size_t create_encoder(unsigned int sample_resolution, unsigned int block_size)
{
    encoders[next_handle] = std::make_unique<encoding_machine>(sample_resolution, block_size);
    return next_handle++;
}

void destroy_encoder(size_t handle)
{
    encoders.erase(handle);
}

bool se_is_better(size_t handle)
{
    return encoders[handle]->se_better;
}

void feed_data_to_encoder(size_t handle, BYTE* data, size_t data_size)
{
    std::vector<BYTE> data_vector;
    data_vector.resize(data_size);
    for (size_t i = 0; i < data_size; ++i)
    {
        data_vector[i] = data[i];
    }
    encoders[handle]->feed_data(data_vector);
}

void encode_data(size_t handle)
{
    encoders[handle]->encode_data();
}

void encode_to_file(size_t handle, const char* filename)
{
    encoders[handle]->save_to_file(std::string{ filename });
}

void encode_file_to_file(size_t handle, const char* source_filename, const char* destinataion_filename)
{
    encoders[handle]->feed_data_from_file(std::string{ source_filename });
    encoders[handle]->save_to_file(std::string{ destinataion_filename });
}

size_t get_encoded_bits_count(size_t handle)
{
    return encoders[handle]->get_encoded_bits_count();
}

void get_encoded_data(size_t handle, BYTE* data_buf, size_t data_size)
{
    if (data_size * 8 < encoders[handle]->get_encoded_bits_count())
    {
        throw std::exception{};
    }
    auto encoded_data = encoders[handle]->get_encoded_data();
    for (size_t i = 0; i < encoded_data.size(); ++i)
    {
        data_buf[i] = encoded_data[i];
    }
}


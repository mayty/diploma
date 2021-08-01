#pragma once
#include "Encoder.h"
#include "Byte.h"
#include "preprocessor.h"
#include <vector>
#include <array>
#include <memory>
#include "helpers.h"

template<unsigned int sample_size, unsigned int block_size>
class encoding_machine
{
private:
    std::vector<BYTE> source_data;
    std::vector<BYTE> encoded_data;
    size_t encoded_data_length = 0;
    bool was_encoded = false;

    constexpr static unsigned int max_k = []() constexpr -> unsigned int {
        if (sample_size <= 2)
            return 0u;
        if (sample_size <= 4)
            return 1u;
        if (sample_size <= 8)
            return 5u;
        if (sample_size <= 16)
            return 13u;
        return 29u;
    }();

    std::unique_ptr<preprocessor> preprocessor;
public:
    encoding_machine();

    void feed_data(const std::vector<BYTE>& data);
    void encode_data();
    std::vector<BYTE> get_encoded_data();
    size_t get_encoded_bits_count();
private:
    std::vector<BYTE> encode_sample(uint64_t sample);
    std::vector<std::array<uint32_t, block_size>> get_blocks();
    std::pair<std::vector<BYTE>, size_t> encode_block(const std::array<uint32_t, block_size>& block);
};

template<unsigned int sample_size, unsigned int block_size>
encoding_machine<sample_size, block_size>::encoding_machine()
    :preprocessor{ new unit_delay_preprocesson<sample_size> }
{
    static_assert(sample_size > 0 && sample_size <= 32);
    static_assert(block_size == 64 || block_size == 32 || block_size == 16 || block_size == 8);
}

template<unsigned int sample_size, unsigned int block_size>
void encoding_machine<sample_size, block_size>::feed_data(const std::vector<BYTE>& data)
{
    this->source_data = data;
    this->was_encoded = false;
}

template<unsigned int sample_size, unsigned int block_size>
void encoding_machine<sample_size, block_size>::encode_data()
{
    std::vector<BYTE> result;
    size_t bit_i = 0;

    for (const auto& block : get_blocks())
    {
        auto [encoded_block, encoded_block_size] = encode_block(block);
        for (int i = 0; i < encoded_block_size; ++i)
        {
            set_bit(result, bit_i++, get_bit(encoded_block, i));
        }
    }

    this->encoded_data = result;
    this->encoded_data_length = this->encoded_data.size() * 8;

    this->was_encoded = true;
}

template<unsigned int sample_size, unsigned int block_size>
std::vector<BYTE> encoding_machine<sample_size, block_size>::get_encoded_data()
{
    if (!was_encoded)
    {
        this->encode_data();
    }
    return this->encoded_data;
}

template<unsigned int sample_size, unsigned int block_size>
size_t encoding_machine<sample_size, block_size>::get_encoded_bits_count()
{
    if (!was_encoded)
    {
        this->encode_data();
    }
    return this->encoded_data_length;
}

template<unsigned int sample_size, unsigned int block_size>
inline std::vector<BYTE> encoding_machine<sample_size, block_size>::encode_sample(uint64_t sample)
{
    uint64_t processed = (*preprocessor)(sample);
    std::vector<BYTE> result;
    do
    {
        result.insert(result.begin(), processed & 0xFF);
        processed >>= 8;
    } while (processed > 0);
    return result;
}

template<unsigned int sample_size, unsigned int block_size>
inline std::vector<std::array<uint32_t, block_size>> encoding_machine<sample_size, block_size>::get_blocks()
{
    size_t bit_i = 0;
    size_t sample_i = 0;
    uint32_t sample = 0;
    std::vector<std::array<uint32_t, block_size>> blocks;
    bool sample_completed = false;
    for (size_t i = 0; i < this->source_data.size(); ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            sample <<= 1;
            if (get_bit(this->source_data, bit_i++))
            {
                sample |= 1;
            }
            if (bit_i % sample_size == 0)
            {
                if (sample_i % block_size)
                {
                    blocks.push_back(std::array<uint32_t, block_size>{});
                }
                blocks[sample_i / block_size][sample_i % block_size] = sample;
                ++sample_i;
                sample = 0;
                sample_completed = true;
            }
            else
            {
                sample_completed = false;
            }
        }
    }
    while(!sample_completed)
    {
        sample <<= 1;
        ++bit_i;
        if (bit_i % sample_size == 0)
        {
            if (sample_i % block_size)
            {
                blocks.push_back(std::array<uint32_t, block_size>{});
            }
            blocks[sample_i / block_size][sample_i % block_size] = sample;
            ++sample_i;
            sample = 0;
            sample_completed = true;
        }
    }

    return blocks;
}

template<unsigned int sample_size, unsigned int block_size>
inline std::pair<std::vector<BYTE>, size_t> encoding_machine<sample_size, block_size>::encode_block(const std::array<uint32_t, block_size>& block)
{
    size_t se_size = 0; // TODO
    size_t nc_size = 0;
    std::vector<size_t> k_sizes;
    for (int i = 0; i <= max_k; ++i)
    {
        k_sizes.push_back(0);
    }

    bool all_zero = true;
    for (int i = 0; i < block.size(); ++i)
    {
        uint32_t sample = block[i];
        if (sample != 0)
        {
            all_zero = false;
        }

        nc_size += sample_size;
        for (int j = 0; j <= max_k; ++j)
        {
            k_sizes[j - 1] += j + 1 + (sample >> j);
        }
    }
    if (all_zero)
    {
        std::pair<std::vector<BYTE>, size_t> result{};
        result.second = 0;
        return result;
    }

    std::pair<std::vector<BYTE>, size_t> result{};
    size_t min_size = nc_size;
    int min_size_k = -1;
    for (int i = 0; i <= max_k; ++i)
    {
        if (k_sizes[i] < min_size)
        {
            min_size = k_sizes[i];
            min_size_k = i;
        }
    }

    return std::pair<std::vector<BYTE>, size_t>();
}

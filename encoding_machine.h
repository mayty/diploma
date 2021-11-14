#pragma once
#include "Encoder.h"
#include "Byte.h"
#include "preprocessor.h"
#include <vector>
#include <array>
#include <memory>
#include <set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "helpers.h"

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
class encoding_machine
{
private:
    std::vector<BYTE> source_data;
    std::vector<BYTE> encoded_data;
    size_t encoded_data_length = 0;
    bool was_encoded = false;
    size_t sample_count = 0;

    constexpr static unsigned int max_k = []() constexpr -> unsigned int {
        if (sample_resolution <= 2)
            return 0u;
        if (sample_resolution <= 4)
            return 1u;
        if (sample_resolution <= 8)
            return 5u;
        if (sample_resolution <= 16)
            return min(sample_resolution, 13u);
        return min(sample_resolution, 29u);
    }();
    constexpr static unsigned int no_compression_prefix = []() constexpr -> unsigned int {
        if (sample_resolution <= 2)
            return 0b1;
        if (sample_resolution <= 4)
            return 0b11;
        if (sample_resolution <= 8)
            return 0b111;
        if (sample_resolution <= 16)
            return 0b1111;
        return 0b11111;
    }();
    constexpr static unsigned int no_compression_prefix_size = []() constexpr -> unsigned int {
        if (sample_resolution <= 2)
            return 1;
        if (sample_resolution <= 4)
            return 2;
        if (sample_resolution <= 8)
            return 3;
        if (sample_resolution <= 16)
            return 4;
        return 5;
    }();

    std::unique_ptr<preprocessor> preprocessor;
public:
    encoding_machine();

    void feed_data(const std::vector<BYTE>& data);
    void encode_data();
    void save_to_file(const std::string& filename);
    std::vector<BYTE> get_encoded_data();
    size_t get_encoded_bits_count();
private:
    std::vector<BYTE> encode_sample(uint64_t sample);
    std::vector<std::array<uint32_t, block_size>> get_blocks();
    std::pair<std::vector<BYTE>, size_t> encode_block(const std::array<uint32_t, block_size>& block);
    void save_header(std::ofstream& out);
};

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
encoding_machine<sample_resolution, block_size>::encoding_machine()
    :preprocessor{ new unit_delay_preprocesson<sample_resolution> }
{
    // static_assert(sample_resolution > 0 && sample_resolution <= 32);
    // static_assert(block_size == 64 || block_size == 32 || block_size == 16 || block_size == 8);
}

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
inline void encoding_machine<sample_resolution, block_size>::feed_data(const std::vector<BYTE>& data)
{
    this->source_data = data;
    this->was_encoded = false;
}

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
inline void encoding_machine<sample_resolution, block_size>::encode_data()
{
    std::vector<BYTE> result;
    size_t bit_i = 0;
    sample_count = 0;

    for (const auto& block : get_blocks())
    {
        sample_count += block_size;
        auto [encoded_block, encoded_block_size] = encode_block(block);
        for (int i = 0; i < encoded_block_size; ++i)
        {
            set_bit(result, bit_i++, get_bit(encoded_block, i));
        }
    }

    this->encoded_data = result;
    this->encoded_data_length = bit_i;

    this->was_encoded = true;
}

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
inline void encoding_machine<sample_resolution, block_size>::save_to_file(const std::string& filename)
{
    if (!was_encoded)
    {
        this->encode_data();
    }

    std::ofstream file{filename, std::ofstream::out | std::ofstream::binary};
    save_header(file);
    for (BYTE byte : encoded_data)
    {
        file.write((const char*)(&byte), 1);
    }
}

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
std::vector<BYTE> encoding_machine<sample_resolution, block_size>::get_encoded_data()
{
    if (!was_encoded)
    {
        this->encode_data();
    }
    return this->encoded_data;
}

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
inline size_t encoding_machine<sample_resolution, block_size>::get_encoded_bits_count()
{
    if (!was_encoded)
    {
        this->encode_data();
    }
    return this->encoded_data_length;
}

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
inline std::vector<BYTE> encoding_machine<sample_resolution, block_size>::encode_sample(uint64_t sample)
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

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
inline std::vector<std::array<uint32_t, block_size>> encoding_machine<sample_resolution, block_size>::get_blocks()
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
            if (bit_i % sample_resolution == 0)
            {
                if (sample_i % block_size == 0)
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
        if (bit_i % sample_resolution == 0)
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

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
inline std::pair<std::vector<BYTE>, size_t> encoding_machine<sample_resolution, block_size>::encode_block(const std::array<uint32_t, block_size>& block)
{
    size_t se_size = 0; // TODO
    size_t no_compression_size = sample_resolution * block_size + no_compression_prefix_size;
    size_t k_size = 0;
    std::array<size_t, max_k + 1> k_sizes;
    for (int i = 0; i <= max_k; ++i)
    {
        k_sizes[i] = no_compression_prefix_size;
    }
    // std::vector<size_t> k_sizes;
    // fundamental sequens is k = 0

    bool all_zero = true;
    // for (int i = 0; i < block_size; ++i)
    for (const auto sample : block)
    {
        // uint32_t sample = block[i];
        if (sample != 0)
        {
            all_zero = false;
        }
        for (uint64_t j = 0; j <= max_k; ++j)
        {
            k_sizes[j] += j + ((sample >> j) + 1);
        }
    }
    //if (all_zero)
    //{
    //    std::pair<std::vector<BYTE>, size_t> result{};
    //    result.second = 0;
    //    return result;
    //}

    std::pair<std::vector<BYTE>, size_t> result{};
    size_t min_size = no_compression_size;
    int min_size_k = -1;
    for (int i = 0; i <= max_k; ++i)
    {
        if (k_sizes[i] < min_size)
        {
            min_size = k_sizes[i];
            min_size_k = i;
        }
    }
    size_t prefix = min_size_k + 1;
    if (min_size_k == -1)
    {
        prefix = min_size_k + 2;
    }

    for (size_t i = 1; i <= no_compression_prefix_size; ++i)
    {
        bool val = (prefix >> (no_compression_prefix_size - i)) & 1;
        set_bit(result.first, i - 1, val);
    }
    size_t i = no_compression_prefix_size;
    if (min_size_k != -1)
    {
        for (const auto sample : block)
        {
            size_t fs_size = sample >> min_size_k;
            for (int j = 0; j < fs_size; ++j)
            {
                set_bit(result.first, i++, false);
            }
            set_bit(result.first, i++, true);
        }
    }
    if (min_size_k != 0)
    {
        size_t split_size = min_size_k;
        if (split_size == -1)
        {
            split_size = sample_resolution;
        }
        uint32_t mask = 0xFFFFFFFFu >> (32 - split_size);
        for (const auto sample : block)
        {
            uint32_t split_bits = sample & mask;
            for (int j = 1; j <= split_size; ++j)
            {
                bool val = (split_bits >> (split_size - j)) & 1;
                set_bit(result.first, i++, val);
            }
        }
    }
    result.second = i;

    return result;
}

template<unsigned int sample_resolution, unsigned int block_size>
requires (
    (block_size == 8 || block_size == 16 || block_size == 32 || block_size == 64) &&
    (sample_resolution <= 32)
)
inline void encoding_machine<sample_resolution, block_size>::save_header(std::ofstream& out)
{
    BYTE* header = new BYTE[6];
    header[0] = 0b01110000;
    header[1] = 0b00100000;
    header[2] = (sample_resolution - 1) & 0b11111;
    BYTE line_3;
    if (block_size == 8)
        line_3 = 0b00011111;
    else if (block_size == 16)
        line_3 = 0b00111111;
    else if (block_size == 32)
        line_3 = 0b01011111;
    else
        line_3 = 0b01111111;

    header[3] = line_3;  // 0[block_size:2]1[reference:4
    header[4] = 0b11111111;  // sample interval:8]
    header[5] = 0b00000000;
    if ((sample_count & 0xFFFFFFFFFFFFull) != sample_count)
    {
        throw std::exception{};
    }
    out.write((char*)header, 6);
    BYTE* s_count = new BYTE[6];
    for (int i = 0; i < 48; ++i)
    {
        if (i % 8 == 0)
        {
            s_count[i / 8] = 0;
        }
        s_count[i / 8] |= ((sample_count >> (47 - i)) & 1) << (7 - (i % 8));
    }
    out.write((char*)s_count, 6);
    delete[] header;
    delete[] s_count;
}

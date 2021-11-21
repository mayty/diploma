#include "pch.h"
#include "encoding_machine.h"

unsigned int get_max_k(unsigned int sample_resolution)
{
    if (sample_resolution <= 2)
        return 0u;
    if (sample_resolution <= 4)
        return 1u;
    if (sample_resolution <= 8)
        return 5u;
    if (sample_resolution <= 16)
        return min(sample_resolution, 13u);
    return min(sample_resolution, 29u);
}

unsigned int get_no_compression_prefix(unsigned int sample_resolution)
{
    if (sample_resolution <= 2)
        return 0b1;
    if (sample_resolution <= 4)
        return 0b11;
    if (sample_resolution <= 8)
        return 0b111;
    if (sample_resolution <= 16)
        return 0b1111;
    return 0b11111;
}

unsigned int get_no_compression_prefix_size(unsigned int sample_resolution)
{
    if (sample_resolution <= 2)
        return 1;
    if (sample_resolution <= 4)
        return 2;
    if (sample_resolution <= 8)
        return 3;
    if (sample_resolution <= 16)
        return 4;
    return 5;
}

encoding_machine::encoding_machine(unsigned int sample_resolution, unsigned int block_size)
    : 
    preprocessor{ new unit_delay_preprocesson{sample_resolution} },
    sample_resolution{ sample_resolution },
    block_size{ block_size }
{
    if (sample_resolution == 0 || sample_resolution > 32)
    {
        throw std::exception{};
    }
    if (block_size != 8 && block_size != 16 && block_size != 32 && block_size != 64)
    {
        throw std::exception{};
    }
    no_compression_prefix_size = get_no_compression_prefix_size(sample_resolution);
    no_compression_prefix = get_no_compression_prefix(sample_resolution);
    max_k = get_max_k(sample_resolution);
}

void encoding_machine::feed_data(const std::vector<BYTE>& data)
{
    this->source_data = data;
    this->was_encoded = false;
}

void encoding_machine::encode_data()
{
    std::vector<BYTE> result;
    size_t bit_i = 0;
    size_t zero_blocks_count = 0;
    bool zero_block_needs_ref = false;
    uint64_t zero_block_reference = 0;
    size_t reference_sample_interval = 4096;
    size_t current_block = 0;

    for (const auto& block : get_blocks())
    {
        bool reference = current_block++ % reference_sample_interval == 0;
        uint64_t reference_sample = preprocessor->get_reference();
        auto [encoded_block, encoded_block_size] = encode_block(block, reference);
        if (encoded_block_size == 0)
        {
            if (zero_blocks_count == 0)
            {
                zero_block_needs_ref = reference;
                if (reference)
                {
                    zero_block_reference = reference_sample;
                }
            }
            zero_blocks_count += 1;
            if (zero_blocks_count == 63 || (reference && zero_blocks_count > 1))
            {
                auto [encoded_zero_block, encoded_zero_block_size] = encode_zero_blocks(zero_blocks_count, zero_block_needs_ref, zero_block_reference);
                for (int i = 0; i < encoded_zero_block_size; ++i)
                {
                    set_bit(result, bit_i++, get_bit(encoded_zero_block, i));
                }
                zero_blocks_count = 0;
                zero_block_needs_ref = false;
            }
            continue;
        }
        if (zero_blocks_count != 0)
        {
            auto [encoded_zero_block, encoded_zero_block_size] = encode_zero_blocks(zero_blocks_count, zero_block_needs_ref, zero_block_reference);
            for (int i = 0; i < encoded_zero_block_size; ++i)
            {
                set_bit(result, bit_i++, get_bit(encoded_zero_block, i));
            }
            zero_blocks_count = 0;
            zero_block_needs_ref = false;
        }
        for (int i = 0; i < encoded_block_size; ++i)
        {
            set_bit(result, bit_i++, get_bit(encoded_block, i));
        }
    }
    if (zero_blocks_count != 0)
    {
        auto [encoded_zero_block, encoded_zero_block_size] = encode_zero_blocks(zero_blocks_count, zero_block_needs_ref, zero_block_reference);
        for (int i = 0; i < encoded_zero_block_size; ++i)
        {
            set_bit(result, bit_i++, get_bit(encoded_zero_block, i));
        }
        zero_blocks_count = 0;
        zero_block_needs_ref = false;
    }

    this->encoded_data = result;
    this->encoded_data_length = bit_i;

    this->was_encoded = true;
}

void encoding_machine::save_to_file(const std::string& filename)
{
    if (!was_encoded)
    {
        this->encode_data();
    }

    std::ofstream file{ filename, std::ofstream::out | std::ofstream::binary };
    save_header(file);
    for (BYTE byte : encoded_data)
    {
        file.write((const char*)(&byte), 1);
    }
}

std::vector<BYTE> encoding_machine::get_encoded_data()
{
    if (!was_encoded)
    {
        this->encode_data();
    }
    return this->encoded_data;
}

size_t encoding_machine::get_encoded_bits_count()
{
    if (!was_encoded)
    {
        this->encode_data();
    }
    return this->encoded_data_length;
}

std::vector<std::vector<uint32_t>> encoding_machine::get_blocks()
{
    size_t bit_i = 0;
    size_t sample_i = 0;
    uint32_t sample = 0;
    std::vector<std::vector<uint32_t>> blocks;
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
                    blocks.push_back(std::vector<uint32_t>{});
                    blocks[sample_i / block_size].resize(block_size);
                }
                blocks[sample_i / block_size][sample_i % block_size] = sample;
                if (sample_i == 0)
                {
                    preprocessor->get_preprocessed(sample);
                }
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
    while (!sample_completed)
    {
        sample <<= 1;
        ++bit_i;
        if (bit_i % sample_resolution == 0)
        {
            if (sample_i % block_size)
            {
                blocks.push_back(std::vector<uint32_t>{});
                blocks[sample_i / block_size].resize(block_size);
            }
            blocks[sample_i / block_size][sample_i % block_size] = sample;
            ++sample_i;
            sample_completed = true;
        }
    }
    sample_count = sample_i;

    return blocks;
}

std::pair<std::vector<BYTE>, size_t> encoding_machine::encode_block(const std::vector<uint32_t>& block, bool reference)
{
    if (block.size() != block_size)
    {
        throw std::exception{};
    }
    uint32_t reference_value = preprocessor->get_reference();
    std::vector<uint32_t> preprocessed_block{};
    for (const auto sample : block)
    {
        preprocessed_block.push_back(preprocessor->get_preprocessed(sample));
    }
    size_t se_size = 0; // TODO
    size_t no_compression_size = sample_resolution * block_size + no_compression_prefix_size;
    size_t k_size = 0;
    std::vector<size_t> k_sizes;
    k_sizes.resize(max_k + 1);
    for (unsigned int i = 0; i <= max_k; ++i)
    {
        k_sizes[i] = no_compression_prefix_size;
    }
    // fundamental sequens is k = 0

    bool all_zero = true;
    for (const auto sample : preprocessed_block)
    {
        if (sample != 0)
        {
            all_zero = false;
        }
        for (uint64_t j = 0; j <= max_k; ++j)
        {
            k_sizes[j] += j + ((sample >> j) + 1);
        }
    }
    if (all_zero)
    {
        std::pair<std::vector<BYTE>, size_t> result{};
        result.second = 0;
        return result;
    }

    std::pair<std::vector<BYTE>, size_t> result{};
    size_t min_size = no_compression_size;
    int min_size_k = -1;
    for (unsigned int i = 0; i <= max_k; ++i)
    {
        if (k_sizes[i] < min_size)
        {
            min_size = k_sizes[i];
            min_size_k = i;
        }
    }
    size_t prefix = min_size_k + 1;
    if (min_size_k == -1) // no compression is best
    {
        prefix = no_compression_prefix;
    }

    for (size_t i = 1; i <= no_compression_prefix_size; ++i) // store prefix
    {
        bool val = (prefix >> (no_compression_prefix_size - i)) & 1;
        set_bit(result.first, i - 1, val);
    }

    size_t i = no_compression_prefix_size;
    if (reference) // store reference value
    {
        for (size_t j = 1; j <= sample_resolution; ++j)
        {
            bool val = (reference_value >> (sample_resolution - j)) & 1;
            set_bit(result.first, i++, val);
        }
    }
    
    if (prefix != no_compression_prefix)
    {
        if (min_size_k != -1) // store samples
        {
            int k = 0;
            if (reference)
            {
                k = 1;
            }
            for (k; k < preprocessed_block.size(); ++k)
            {
                uint32_t sample = preprocessed_block[k];
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
            int k = 0;
            if (reference)
            {
                k = 1;
            }
            for (k; k < preprocessed_block.size(); ++k)
            {
                uint32_t sample = preprocessed_block[k];
                uint32_t split_bits = sample & mask;
                for (int j = 1; j <= split_size; ++j)
                {
                    bool val = (split_bits >> (split_size - j)) & 1;
                    set_bit(result.first, i++, val);
                }
            }
        }
    }
    else
    {
        int k = 0;
        if (reference)
        {
            k = 1;
        }
        for (k; k < preprocessed_block.size(); ++k)
        {
            uint32_t sample = preprocessed_block[k];
            for (int j = 1; j <= sample_resolution; ++j)
            {
                bool val = (sample >> (sample_resolution - j)) & 1;
                set_bit(result.first, i++, val);
            }
        }
    }
    result.second = i;

    return result;
}

std::pair<std::vector<BYTE>, size_t> encoding_machine::encode_zero_blocks(size_t zero_block_count, bool reference, uint32_t reference_value)
{
    std::pair<std::vector<BYTE>, size_t> result{};
    size_t i = 0;
    for (int j = 0; j < no_compression_prefix_size + 1; ++j)
    {
        set_bit(result.first, i++, false);
    }
    if (reference)
    {
        for (int j = 1; j <= sample_resolution; ++j)
        {
            bool value = (reference_value >> (sample_resolution - j)) & 1;
            set_bit(result.first, i++, value);
        }
    }
    size_t trailing_zeroes_count = zero_block_count;
    if (zero_block_count < 5)
    {
        trailing_zeroes_count -= 1;
    }
    for (int j = 0; j < trailing_zeroes_count; ++j)
    {
        set_bit(result.first, i++, false);
    }
    set_bit(result.first, i++, true);
    result.second = i;
    return result;
}

void encoding_machine::save_header(std::ofstream& out)
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

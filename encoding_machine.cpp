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

void encoding_machine::feed_data_from_file(const std::string& filename)
{
    in_file.open(filename, std::ifstream::in | std::ifstream::binary);
    std::ifstream in = std::ifstream{ filename, std::ifstream::in | std::ifstream::binary };

    this->was_encoded = false;
    source_data.clear();
    encoded_data.clear();
    do
    {
        BYTE in_buf[64];
        in.read((char*)in_buf, 64);
        if (in)
        {
            for (int i = 0; i < sizeof(in_buf) / sizeof(BYTE); ++i)
            {
                source_data.push_back(in_buf[i]);
            }
        }
        else
        {
            for (int i = 0; i < in.gcount(); ++i)
            {
                source_data.push_back(in_buf[i]);
            }
        }
    } while (in);
}

size_t encoding_machine::get_second_extension_size(const std::vector<uint32_t>& block, bool reference)
{
    size_t result = no_compression_prefix_size + 1;
    for (int i = 1; i <= block.size() / 2; ++i)
    {
        size_t i1 = (i * 2) - 2;
        size_t i2 = (i * 2) - 1;
        uint64_t sample_a = block[i1];
        if (reference && i == 1)
        {
            sample_a = 0;
        }
        uint64_t sample_b = block[i2];
        uint64_t sample = (sample_a + sample_b) * (sample_a + sample_b + 1) / 2 + sample_b;
        result += sample + 1;
    }
    return result;
}


std::vector<uint64_t> encoding_machine::get_second_extension_block(const std::vector<uint32_t>& block, bool reference)
{
    std::vector<uint64_t> result;
    for (int i = 1; i <= block.size() / 2; ++i)
    {
        size_t i1 = (i * 2) - 2;
        size_t i2 = (i * 2) - 1;
        uint64_t sample_a = block[i1];
        if (reference && i == 1)
        {
            sample_a = 0;
        }
        uint64_t sample_b = block[i2];
        uint64_t sample = (sample_a + sample_b) * (sample_a + sample_b + 1) / 2 + sample_b;
        result.push_back(sample);
    }
    return result;
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
    uint32_t zero_block_reference = 0;
    size_t reference_sample_interval = 4096;
    size_t current_block = 0;
    sample_count = 0;

    while(true)
    //for (const auto& block : get_blocks())
    {
        const auto& block = get_next_block();
        if (block.size() == 0)
        {
            break;
        }
        bool reference = current_block++ % reference_sample_interval == 0;
        uint32_t reference_sample = preprocessor->get_reference();
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
    file.close();
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

    uint32_t reference_value = 0;// = preprocessor->get_reference();
    if (reference)
    {
        preprocessor->get_preprocessed(block[0]);
        reference_value = preprocessor->get_reference();
    }

    std::vector<uint32_t> preprocessed_block{};
    for (const auto sample : block)
    {
        preprocessed_block.push_back(preprocessor->get_preprocessed(sample));
    }

    size_t se_size = get_second_extension_size(preprocessed_block, reference);
    size_t no_compression_size = sample_resolution * block_size + no_compression_prefix_size;

    std::vector<size_t> k_sizes;
    k_sizes.resize(max_k + 1);
    for (unsigned int i = 0; i <= max_k; ++i)
    {
        k_sizes[i] = no_compression_prefix_size;
    }

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

    if (min_size > se_size)
    {
        return encode_second_extension(preprocessed_block, reference, reference_value);
    }
    if (min_size_k == -1)
    {
        return encode_no_compression(preprocessed_block, reference, reference_value);
    }
    if (min_size_k == 0)
    {
        return encode_fundamental_sequence(preprocessed_block, reference, reference_value);
    }
    return encode_split_sample(preprocessed_block, reference, reference_value, min_size_k);
}

std::vector<uint32_t> encoding_machine::get_next_block()
{
    std::vector<uint32_t> next_block;
    BYTE in_buf[64 * 32];

    std::vector<BYTE> in_data;

    in_file.read((char*)in_buf, sample_resolution * block_size / 8);
    if (in_file)
    {
        for (int i = 0; i < sample_resolution * block_size / 8; ++i)
        {
            in_data.push_back(in_buf[i]);
        }
    }
    else
    {
        for (int i = 0; i < in_file.gcount(); ++i)
        {
            in_data.push_back(in_buf[i]);
        }
    }

    if (in_data.size() == 0)
    {
        return next_block;
    }


    size_t bits_count = in_data.size() * 8;
    size_t current_i = 0;
    size_t actual_count = 0;

    for (int i = 0; i < block_size; ++i)
    {
        uint32_t sample = 0;
        for (int j = 0; j < sample_resolution; ++j)
        {
            if (current_i >= bits_count)
            {
                for (int k = j; k < sample_resolution; ++k)
                {
                    sample <<= 1;
                }
                break;
            }
            sample <<= 1;
            if (get_bit(in_data, current_i++))
            {
                sample |= 1;
            }
        }
        next_block.push_back(sample);
        ++actual_count;
        if (current_i >= bits_count)
        {
            for (i = i + 1; i < block_size; ++i)
            {
                next_block.push_back(0);
            }
        }
    }
    sample_count += actual_count;
    return next_block;
}

std::pair<std::vector<BYTE>, size_t> encoding_machine::encode_no_compression(const std::vector<uint32_t>& block, bool reference, uint32_t reference_value)
{
    std::pair<std::vector<BYTE>, size_t> result{};

    for (int i = 0; i < no_compression_prefix_size; ++i)
    {
        set_bit(result.first, i, true);
    }
    //for (size_t i = 1; i <= no_compression_prefix_size; ++i) // store prefix
    //{
    //    bool val = (no_compression_prefix >> (no_compression_prefix_size - i)) & 1;
    //    set_bit(result.first, i - 1, val);
    //}

    size_t i = no_compression_prefix_size;
    if (reference) // store reference value
    {
        for (size_t j = 1; j <= sample_resolution; ++j)
        {
            bool val = (reference_value >> (sample_resolution - j)) & 1;
            set_bit(result.first, i++, val);
        }
    }

    int k = 0;
    if (reference)
    {
        k = 1;
    }
    for (k; k < block.size(); ++k)
    {
        uint32_t sample = block[k];
        for (int j = 1; j <= sample_resolution; ++j)
        {
            bool val = (sample >> (sample_resolution - j)) & 1;
            set_bit(result.first, i++, val);
        }
    }

    result.second = i;
    return result;
}

std::pair<std::vector<BYTE>, size_t> encoding_machine::encode_second_extension(const std::vector<uint32_t>& block, bool reference, uint32_t reference_value)
{
    std::pair<std::vector<BYTE>, size_t> result{};
    size_t prefix = 1;
    for (int i = 0; i < no_compression_prefix_size; ++i)
    {
        set_bit(result.first, i, false);
    }
    set_bit(result.first, no_compression_prefix_size, true);


    size_t i = no_compression_prefix_size + 1;
    if (reference) // store reference value
    {
        for (size_t j = 1; j <= sample_resolution; ++j)
        {
            bool val = (reference_value >> (sample_resolution - j)) & 1;
            set_bit(result.first, i++, val);
        }
    }

    for (const auto& sample : get_second_extension_block(block, reference))
    {
        for (int j = 0; j < sample; ++j)
        {
            set_bit(result.first, i++, false);
        }
        set_bit(result.first, i++, true);
    }

    result.second = i;
    return result;
}

std::pair<std::vector<BYTE>, size_t> encoding_machine::encode_fundamental_sequence(const std::vector<uint32_t>& block, bool reference, uint32_t reference_value)
{
    std::pair<std::vector<BYTE>, size_t> result{};
    size_t prefix = 1;
    for (int i = 1; i < no_compression_prefix_size; ++i)
    {
        set_bit(result.first, i - 1, false);
    }
    set_bit(result.first, no_compression_prefix_size - 1, true);


    size_t i = no_compression_prefix_size;
    if (reference) // store reference value
    {
        for (size_t j = 1; j <= sample_resolution; ++j)
        {
            bool val = (reference_value >> (sample_resolution - j)) & 1;
            set_bit(result.first, i++, val);
        }
    }

    int k = 0;
    if (reference)
    {
        k = 1;
    }
    for (k; k < block.size(); ++k)
    {
        uint32_t sample = block[k];
        for (int j = 0; j < sample; ++j)
        {
            set_bit(result.first, i++, false);
        }
        set_bit(result.first, i++, true);
    }
    result.second = i;
    return result;
}

std::pair<std::vector<BYTE>, size_t> encoding_machine::encode_split_sample(const std::vector<uint32_t>& block, bool reference, uint32_t reference_value, size_t k)
{
    std::pair<std::vector<BYTE>, size_t> result{};
    size_t prefix = k + 1;
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

    int k1 = 0;
    if (reference)
    {
        k1 = 1;
    }
    for (k1; k1 < block.size(); ++k1)
    {
        uint32_t sample = block[k1];
        size_t fs_size = sample >> k;
        for (int j = 0; j < fs_size; ++j)
        {
            set_bit(result.first, i++, false);
        }
        set_bit(result.first, i++, true);
    }

    uint32_t mask = 0xFFFFFFFFu >> (32 - k);
    k1 = 0;
    if (reference)
    {
        k1 = 1;
    }
    for (k1; k1 < block.size(); ++k1)
    {
        uint32_t sample = block[k1];
        uint32_t split_bits = sample & mask;
        for (int j = 1; j <= k; ++j)
        {
            bool val = (split_bits >> (k - j)) & 1;
            set_bit(result.first, i++, val);
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

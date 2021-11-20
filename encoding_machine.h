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

class encoding_machine
{
private:
    std::vector<BYTE> source_data;
    std::vector<BYTE> encoded_data;
    size_t encoded_data_length = 0;
    size_t sample_count = 0;

    bool was_encoded = false;
    
    unsigned int sample_resolution;
    unsigned int block_size;

    unsigned int max_k;
    unsigned int no_compression_prefix;
    unsigned int no_compression_prefix_size;

    std::unique_ptr<preprocessor> preprocessor;
public:
    encoding_machine(unsigned int sample_resolution, unsigned int block_size);

    void feed_data(const std::vector<BYTE>& data);
    void encode_data();
    void save_to_file(const std::string& filename);
    std::vector<BYTE> get_encoded_data();
    size_t get_encoded_bits_count();
private:
    std::vector<std::vector<uint32_t>> get_blocks();
    std::pair<std::vector<BYTE>, size_t> encode_block(const std::vector<uint32_t>& block, bool reference);
    std::pair<std::vector<BYTE>, size_t> encode_zero_blocks(size_t zero_block_count, bool reference, uint32_t reference_value);
    void save_header(std::ofstream& out);
};

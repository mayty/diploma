#pragma once
#include <vector>
#include "Byte.h"
#include <string>

class decoding_machine
{
private:
	size_t sample_resolution = 0;
	size_t block_size = 0;
	size_t reference_sample_interval = 0;
	size_t sample_count = 0;
	std::vector<BYTE> decoded_data;
	std::vector<BYTE> encoded_data;
	bool data_decoded = false;
public:
	size_t get_decoded_bits_count();
	std::vector<BYTE> get_decoded_data();
	void feed_data_from_file(const std::string& filename);
	void decode_data();
private:
	void init_from_file(std::ifstream& in);
	void read_data_from_file(std::ifstream& in);
	size_t get_prefix_size();
	size_t decode_no_compression(size_t& i_encoded, size_t& i_decoded);
	size_t decode_zero_block(size_t& i_encoded, size_t& i_decoded);
	size_t decode_second_extension(size_t& i_encoded, size_t& i_decoded);
	size_t decode_fundamental_sequence(size_t& i_encoded, size_t& i_decoded);
	size_t decode_k(size_t& i_encoded, size_t& i_decoded, size_t k);
};


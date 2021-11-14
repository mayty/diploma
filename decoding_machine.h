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
public:
	size_t get_decoded_bits_count();
	std::vector<BYTE> get_decoded_data();
	void feed_data_from_file(const std::string& filename);
	void decode_data();
private:
	void init_from_file(std::ifstream& in);
};


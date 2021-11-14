#include "pch.h"
#include "decoding_machine.h"
#include <fstream>

size_t decoding_machine::get_decoded_bits_count()
{
	return size_t();
}

std::vector<BYTE> decoding_machine::get_decoded_data()
{
	return std::vector<BYTE>();
}

void decoding_machine::feed_data_from_file(const std::string& filename)
{
	std::ifstream file = std::ifstream{ filename, std::ifstream::in | std::ifstream::binary };
	init_from_file(file);
}

void decoding_machine::decode_data()
{
}

void decoding_machine::init_from_file(std::ifstream& in)
{
	BYTE header[12];
	in.read((char*)header, 12);
	if (header[0] != 0b01110000)
		throw std::exception{};
	if (header[1] != 0b00100000)
		throw std::exception{};
	if (header[2] & 0b11100000)
		throw std::exception{};
	sample_resolution = header[2] + 1;
	if (header[3] & 0b10000000 || !(header[3] & 0b00010000))
		throw std::exception{};
	block_size = (header[3] & 0b01100000) >> 5;
	block_size = 8 << block_size;
	reference_sample_interval = 0;
	for (int i = 0; i < 4; ++i)
	{
		reference_sample_interval <<= 1;
		reference_sample_interval |= (header[3] >> (3 - i)) & 1;
	}
	for (int i = 0; i < 8; ++i)
	{
		reference_sample_interval <<= 1;
		reference_sample_interval |= (header[4] >> (7 - i)) & 1;
	}
	sample_count = 0;
	for (int i = 0; i < 48; ++i)
	{
		sample_count <<= 1;
		sample_count |= (header[6 + (i / 8)] >> (7 - (i % 8))) & 1;
	}
}

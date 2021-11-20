#include "pch.h"
#include "decoding_machine.h"
#include "helpers.h"
#include <fstream>

size_t decoding_machine::get_decoded_bits_count()
{
	if (!data_decoded)
	{
		decode_data();
	}
	return sample_count * sample_resolution;
}

std::vector<BYTE> decoding_machine::get_decoded_data()
{
	if (!data_decoded)
	{
		decode_data();
	}
	return decoded_data;
}

void decoding_machine::feed_data_from_file(const std::string& filename)
{
	std::ifstream file = std::ifstream{ filename, std::ifstream::in | std::ifstream::binary };
	init_from_file(file);
	read_data_from_file(file);
}

void decoding_machine::decode_data()
{
	size_t i_encoded = 0;
	size_t i_decoded = 0;
	size_t read_sample_count = 0;
	int64_t samples_to_read_count = sample_count;
	size_t prefix_size = get_prefix_size();
	// get block encoding type
	while (samples_to_read_count > 0)
	{
		size_t prefix = 0;
		bool extended_prefix = false;
		for (int j = 0; j < prefix_size; ++j)
		{
			prefix <<= 1;
			if (get_bit(encoded_data, i_encoded++))
			{
				prefix |= 1;
			}
		}
		if (prefix == 0)
		{
			extended_prefix = true;
			prefix <<= 1;
			if (get_bit(encoded_data, i_encoded++))
			{
				prefix |= 1;
			}
		}

		if (prefix == (1 << prefix_size) - 1)  // no compression
		{
			samples_to_read_count -= decode_no_compression(i_encoded, i_decoded, samples_to_read_count);
		}
		else if (prefix == 0)  // Zero-Block
		{
			samples_to_read_count -= decode_zero_block(i_encoded, i_decoded, samples_to_read_count);
		}
		else if (extended_prefix)  // Second-Extension
		{
			samples_to_read_count -= decode_second_extension(i_encoded, i_decoded, samples_to_read_count);
		}
		else if (prefix == 1) // fundamental sequence
		{
			samples_to_read_count -= decode_fundamental_sequence(i_encoded, i_decoded, samples_to_read_count);
		}
		else  // split sample
		{
			samples_to_read_count -= decode_k(i_encoded, i_decoded, prefix - 1, samples_to_read_count);
		}
	}
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
	block_size = 8ull << block_size;
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

void decoding_machine::read_data_from_file(std::ifstream& in)
{
	encoded_data.clear();
	do
	{
		BYTE in_buf[64];
		in.read((char*)in_buf, 64);
		if (in)
		{
			for (int i = 0; i < sizeof(in_buf) / sizeof(BYTE); ++i)
			{
				encoded_data.push_back(in_buf[i]);
			}
		}
		else
		{
			for (int i = 0; i < in.gcount(); ++i)
			{
				encoded_data.push_back(in_buf[i]);
			}
		}
	} while (in);
}

size_t decoding_machine::get_prefix_size()
{
	if (sample_resolution <= 2)
		return 1;
	if (sample_resolution <= 4)
		return 2;
	if (sample_resolution <= 8)
		return 3;
	if (sample_resolution <= 16)
		return 4;
	if (sample_resolution <= 32)
		return 5;
}

size_t decoding_machine::decode_no_compression(size_t& i_encoded, size_t& i_decoded, size_t samples_left)
{
	size_t required_samples_count = min(block_size, samples_left);

	for (int i = 0; i < sample_resolution * required_samples_count; ++i)
	{
		set_bit(decoded_data, i_decoded++, get_bit(encoded_data, i_encoded++));
	}
	return required_samples_count;
}

size_t decoding_machine::decode_zero_block(size_t& i_encoded, size_t& i_decoded, size_t samples_left)
{
	size_t trailing_zeroes_count = 0;
	while (!get_bit(encoded_data, i_encoded++))
	{
		trailing_zeroes_count += 1;
	}
	size_t zero_blocks_count = trailing_zeroes_count;
	if (trailing_zeroes_count < 4)
	{
		zero_blocks_count += 1;
	}
	size_t samples_count = min(zero_blocks_count * block_size, samples_left);
	for (int i = 0; i < samples_count * sample_resolution; ++i)
	{
		set_bit(decoded_data, i_decoded++, false);
	}

	return samples_count;
}

size_t decoding_machine::decode_second_extension(size_t& i_encoded, size_t& i_decoded, size_t samples_left)
{
	throw std::exception{};
	return size_t();
}

size_t decoding_machine::decode_fundamental_sequence(size_t& i_encoded, size_t& i_decoded, size_t samples_left)
{
	size_t required_samples_count = min(samples_left, block_size);
	for (int i = 0; i < required_samples_count; ++i)
	{
		size_t sample = 0;
		while (!get_bit(encoded_data, i_encoded++))
		{
			sample += 1;
		}
		for (int j = 1; j <= sample_resolution; ++j)
		{
			set_bit(decoded_data, i_decoded++, (sample >> (sample_resolution - j)) & 1);
		}
	}
	return required_samples_count;
}

size_t decoding_machine::decode_k(size_t& i_encoded, size_t& i_decoded, size_t k, size_t samples_left)
{
	size_t required_samples_count = min(samples_left, block_size);
	std::vector<size_t> elder_bits{};
	elder_bits.resize(required_samples_count);
	for (int i = 0; i < block_size; ++i)
	{
		size_t sample = 0;
		while (!get_bit(encoded_data, i_encoded++))
		{
			sample += 1;
		}
		if (i < required_samples_count)
		{
			elder_bits[i] = sample << k;
		}
	}
	for (int i = 0; i < required_samples_count; ++i)
	{
		size_t sample = 0;
		for (int j = 0; j < k; ++j)
		{
			sample <<= 1;
			if (get_bit(encoded_data, i_encoded++))
			{
				sample |= 1;
			}
		}
		sample |= elder_bits[i];
		for (int j = 1; j <= sample_resolution; ++j)
		{
			set_bit(decoded_data, i_decoded++, (sample >> (sample_resolution - j)) & 1);
		}
	}
	return required_samples_count;
}

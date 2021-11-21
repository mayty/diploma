#pragma once
#include <cstdint>

class reverse_preprocessor
{
private:
	uint32_t reference;
	uint32_t max_value = 0;
public:
	reverse_preprocessor() = default;
	reverse_preprocessor(unsigned int sample_resolution);
	void set_sample_resolution(unsigned int sample_resolution);
	void set_reference(uint32_t reference);
	uint32_t get_value(uint32_t preprocessed);
private:
	uint32_t _get_value(uint32_t preprocessed);
};


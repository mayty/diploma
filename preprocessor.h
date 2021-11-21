#pragma once
#include <cstdint>
#include <cstdlib>
#include <exception>

class preprocessor
{
public:
    virtual uint32_t get_preprocessed(uint32_t sample) = 0;
    virtual uint32_t get_reference() = 0;
    virtual ~preprocessor() = default;
};

class unit_delay_preprocesson : public preprocessor
{
private:
	uint32_t max_value;
    uint32_t min_value = 0;
	unsigned int sample_size;
    uint32_t preceeding_sample = 0;
    uint32_t map(int64_t error);
public:
    unit_delay_preprocesson(unsigned int sample_size);
    uint32_t get_preprocessed(uint32_t sample) override;
    uint32_t get_reference() override;
	~unit_delay_preprocesson() override = default;
};

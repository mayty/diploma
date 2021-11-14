#pragma once
#include "Byte.h"

#ifdef MATHLIBRARY_EXPORTS
#define ENCODER_API __declspec(dllexport)
#else
#define ENCODER_API __declspec(dllimport)
#endif

extern "C" ENCODER_API size_t create_encoder();
extern "C" ENCODER_API void destroy_encoder(size_t handle);
extern "C" ENCODER_API void feed_data_to_encoder(size_t handle, BYTE* data, size_t data_size);
extern "C" ENCODER_API void encode_data(size_t handle);
extern "C" ENCODER_API void encode_to_file(size_t handle, const char* filename);
extern "C" ENCODER_API size_t get_encoded_bits_count(size_t handle);
extern "C" ENCODER_API void get_encoded_data(size_t handle, BYTE* data_buf, size_t data_size);

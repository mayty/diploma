#pragma once
#include "Byte.h"

#ifdef MATHLIBRARY_EXPORTS
#define ENCODER_API __declspec(dllexport)
#else
#define ENCODER_API __declspec(dllimport)
#endif

extern "C" ENCODER_API size_t create_decoder();
extern "C" ENCODER_API void destroy_decoder(size_t handle);
extern "C" ENCODER_API void decode_from_file(size_t handle, const char* filename);
extern "C" ENCODER_API size_t get_decoded_bits_count(size_t handle);
extern "C" ENCODER_API void get_decoded_data(size_t handle, BYTE * data_buf);

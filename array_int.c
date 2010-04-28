#include "array.h"
#include <endian.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN

#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
#define MAKE_KEY_BIGENDIAN() \
	uint64_t real_key = __builtin_bswap64(key)
#else
#define MAKE_KEY_BIGENDIAN() \
	uint8_t real_key[8]; \
	real_key[7] = key & 0xff; \
	real_key[6] = (key >> 8) & 0xff; \
	real_key[5] = (key >> 16) & 0xff; \
	real_key[4] = (key >> 24) & 0xff; \
	real_key[3] = (key >> 32) & 0xff; \
	real_key[2] = (key >> 40) & 0xff; \
	real_key[1] = (key >> 48) & 0xff; \
	real_key[0] = (key >> 56) & 0xff;
#endif

void *array_get_int(array_t *array, uint64_t key) {
	MAKE_KEY_BIGENDIAN();
	return array_get(array, 8, (uint8_t*)&real_key);
}

bool array_insert_int(array_t *array, uint64_t key, void *value) {
	MAKE_KEY_BIGENDIAN();
	return array_insert(array, 8, (uint8_t*)&real_key, value, true);
}

bool array_update_int(array_t *array, uint64_t key, void *value) {
	MAKE_KEY_BIGENDIAN();
	return array_update(array, 8, (uint8_t*)&real_key, value, true);
}

bool array_remove_int(array_t *array, uint64_t key) {
	MAKE_KEY_BIGENDIAN();
	return array_remove(array, 8, (uint8_t*)&real_key);
}

void *array_take_int(array_t *array, uint64_t key) {
	MAKE_KEY_BIGENDIAN();
	return array_take(array, 8, (uint8_t*)&real_key);
}

uint64_t array_key_to_int(const uint8_t *_key) {
	uint64_t key = *((uint64_t*)_key);
	MAKE_KEY_BIGENDIAN();
	return *((uint64_t*)&real_key);
}

#elif __BYTE_ORDER == __BIG_ENDIAN

void *array_get_int(array_t *array, uint64_t key) {
	return array_get(array, 8, (uint8_t*)&key);
}

bool array_insert_int(array_t *array, uint64_t key, void *value) {
	return array_insert(array, 8, (uint8_t*)&key, value, true);
}

bool array_update_int(array_t *array, uint64_t key, void *value) {
	return array_update(array, 8, (uint8_t*)&key, value, true);
}

bool array_remove_int(array_t *array, uint64_t key) {
	return array_remove(array, 8, (uint8_t*)&key);
}

void *array_take_int(array_t *array, uint64_t key) {
	return array_take(array, 8, (uint8_t*)&key);
}

uint64_t array_key_to_int(const uint8_t *_key) {
	return *((uint64_t*)_key);
}

#else
#error Unknown endianness type
#endif


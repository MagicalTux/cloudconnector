/**
 * TreeArray .h file
 */

#include <stdint.h>
#include <stdbool.h>

typedef struct array_base {
	struct array_node *root;
	uint32_t count;
} array_t;

typedef struct array_iterator {
	array_t *array;
	struct array_node *node;
	struct array_node *next;
	int seen;
	const uint8_t *key;
	void *value;
	int keylen;
	bool is_type;
} array_iterator_t;

struct array_node {
	struct array_node *parent;
	uint8_t node_key;
	uint16_t children; // children count, up to 256
	bool has_value;
	uint8_t *value_key;
	uint32_t value_keylen; // length of key for the value
	bool value_is_type;
	void *value;
	struct array_node **nodes;
};

// Basic functions
array_t *array_new();
void array_free(array_t *);
void array_truncate(array_t *);
void array_optimize(array_t *);

void *array_get(array_t *, int keylen, const uint8_t *key);
bool array_insert(array_t *, int keylen, const uint8_t *key, void *value, bool is_type);
bool array_update(array_t *, int keylen, const uint8_t *key, void *value, bool is_type);
void *array_take(array_t *, int keylen, const uint8_t *key);
bool array_remove(array_t *, int keylen, const uint8_t *key);
bool array_remove_iterator(array_iterator_t *iterator);

// debugging/dump
void array_dump(array_t *);
void array_debug(array_t *);

// Iterators (and related)
array_iterator_t *array_iterator(array_t *);
void array_iterator_free(array_iterator_t*);
bool array_next(array_iterator_t *);
uint64_t array_key_to_int(const uint8_t*);

// Integer functions
bool array_insert_int(array_t *, uint64_t key, void *value);
bool array_update_int(array_t *, uint64_t key, void *value);
void *array_get_int(array_t *, uint64_t key);
void *array_take_int(array_t *, uint64_t key);
bool array_remove_int(array_t *, uint64_t key);

// string defines
#define array_insert_string_const(array, key, value) array_insert(array, sizeof(key)-1, (uint8_t*)(key), value, false)
#define array_insert_string(array, key, value) array_insert(array, strlen(key), (uint8_t*)(key), value, false)

#define array_update_string_const(array, key, value) array_update(array, sizeof(key)-1, (uint8_t*)(key), value, false)
#define array_update_string(array, key, value) array_update(array, strlen(key), (uint8_t*)(key), value, false)

#define array_get_string_const(array, key) array_get(array, sizeof(key)-1, (uint8_t*)(key))
#define array_get_string(array, key) array_get(array, strlen(key), (uint8_t*)(key))

#define array_remove_string_const(array, key) array_remove(array, sizeof(key)-1, (uint8_t*)(key))
#define array_remove_string(array, key) array_remove(array, strlen(key), (uint8_t*)(key))

#define array_take_string_const(array, key) array_take(array, sizeof(key)-1, (uint8_t*)(key))
#define array_take_string(array, key) array_take(array, strlen(key), (uint8_t*)(key))


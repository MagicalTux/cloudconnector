#include <string.h>
#include <stdlib.h>
#include "array.h"

array_t *array_new() {
	array_t *res = calloc(sizeof(array_t), 1);
	return res;
}

static void array_free_node(array_t *array, struct array_node *node) {
	if (node->children) {
		for(int i = 0; i < 256; i++) {
			if (node->nodes[i] != NULL)
				array_free_node(array, node->nodes[i]);
		}
		free(node->nodes);
	}
	if (node->has_value) {
		free(node->value_key);
	}
	free(node);
}

void array_optimize(array_t *array) {
	// simple: create new array, put stuff in it :)
	array_iterator_t *it = array_iterator(array);
	struct array_node *root = array->root; // keep root here
	array->root = NULL;
	array->count = 0;

	while(array_next(it)) {
		array_insert(array, it->keylen, it->key, it->value, it->is_type);
	}

	array_iterator_free(it);
	array_free_node(NULL, root);
}

void array_truncate(array_t *array) {
	if (array->root == NULL) return;
	array_free_node(array, array->root);
	array->root = NULL;
	array->count = 0;
}

void array_free(array_t *array) {
	array_truncate(array);
	free(array);
}

bool array_insert(array_t *array, int keylen, const uint8_t *key, void *value, bool is_type) {
	// find end node with this key
	struct array_node *node, *parent = NULL;
	int pos = 0;
	if (array->root == NULL) {
		// ok, create a root node
		array->root = calloc(sizeof(struct array_node), 1);
	}
	node = array->root;
	while(1) {
		if (pos == keylen) {
			if (node->has_value) return false;
			break;
		}
		if (node->children > 0) {
			if (node->nodes[key[pos]] != NULL) {
				parent = node;
				node = node->nodes[key[pos]];
				pos++;
				continue;
			}
			// create new child node
			node->nodes[key[pos]] = calloc(sizeof(struct array_node), 1);
			node->children++;
			parent = node;
			node = node->nodes[key[pos]];
			node->parent = parent;
			node->node_key = key[pos];
			pos++;
			continue;
		}

		// found a valid node!
		if ((!node->has_value) || ((node->value_keylen == pos) && (pos > keylen)))
			break; // no value, or value matches pos, we got our node!

		// this node has a value with a larger key, need a new intermediate node!

		// is this a duplicate of the value we are inserting?
		if ((node->value_keylen == keylen) && (memcmp(node->value_key+pos, key+pos, keylen-pos) == 0))
			return false;

		// create intermediate node and move the current node lower
		struct array_node *newnode = calloc(sizeof(struct array_node), 1);
		newnode->nodes = calloc(sizeof(void*), 256);
		newnode->nodes[node->value_key[pos]] = node;
		newnode->children = 1;
		node->parent = newnode;
		node->node_key = node->value_key[pos];
		if (parent == NULL) {
			array->root = newnode;
		} else {
			parent->nodes[key[pos-1]] = newnode;
			newnode->parent = parent;
			newnode->node_key = key[pos-1];
		}
		parent = node;
		node = newnode;
	}

	node->has_value = true;
	node->value = value;
	node->value_keylen = keylen;
	node->value_key = malloc(keylen);
	node->value_is_type = is_type;
	memcpy(node->value_key, key, keylen);
	array->count++;
	return true;
}

struct array_node *array_get_node(array_t *array, int keylen, const uint8_t *key) {
	struct array_node *node = array->root;
	int pos = 0;
	while(pos < keylen) {
		if ((node->nodes != NULL) && (node->nodes[key[pos]] != NULL)) {
			node = node->nodes[key[pos]];
			pos++;
			continue;
		}
		if (!node->has_value) return NULL;
		if ((node->value_keylen == keylen) && (memcmp(node->value_key+pos, key+pos, keylen-pos) == 0)) break;
		return NULL;
	}

	return node;
}

void *array_get(array_t *array, int keylen, const uint8_t *key) {
	struct array_node *node = array_get_node(array, keylen, key);
	if (node == NULL) return NULL;
	if (!node->has_value) return NULL;
	return node->value;
}

bool array_update(array_t *array, int keylen, const uint8_t *key, void *value, bool is_type) {
	struct array_node *node = array_get_node(array, keylen, key);
	if (node == NULL) return false;
	if (!node->has_value) return false;
	node->value = value;
	node->value_is_type = is_type;

	return true;
}

bool array_remove(array_t *array, int keylen, const uint8_t *key) {
	struct array_node *node = array_get_node(array, keylen, key);
	if (node == NULL) return false;
	if (!node->has_value) return false;

	free(node->value_key);
	node->has_value = false;
	array->count--;

	while ((node->children == 0) && (!node->has_value)) {
		if (node->parent == NULL) { // root node
			// this array is now empty!
			free(node);
			array->root = NULL;
			return true;
		}

		struct array_node *parent = node->parent;
		parent->nodes[node->node_key] = NULL;
		parent->children--;
		free(node);
		node = parent;
		if (node->children == 0) {
			free(node->nodes);
			node->nodes = NULL;
		}
	}

	return true;
}

bool array_remove_iterator(array_iterator_t *iterator) {
	array_t *array = iterator->array;
	struct array_node *node = iterator->node;
	if (node == NULL) return false;
	if (!node->has_value) return false;

	free(node->value_key);
	node->has_value = false;
	array->count--;

	while ((node->children == 0) && (!node->has_value)) {
		if (node->parent == NULL) { // root node
			// this array is now empty!
			free(node);
			array->root = NULL;
			return true;
		}

		struct array_node *parent = node->parent;
		parent->nodes[node->node_key] = NULL;
		parent->children--;
		free(node);
		node = parent;
		if (node->children == 0) {
			free(node->nodes);
			node->nodes = NULL;
		}
	}

	return true;
}

void *array_take(array_t *array, int keylen, const uint8_t *key) {
	struct array_node *node = array_get_node(array, keylen, key);
	if (node == NULL) return NULL;
	if (!node->has_value) return NULL;

	void *res = node->value;

	free(node->value_key);
	node->has_value = false;
	array->count--;

	while ((node->children == 0) && (!node->has_value)) {
		if (node->parent == NULL) { // root node
			// this array is now empty!
			free(node);
			array->root = NULL;
			return res;
		}

		struct array_node *parent = node->parent;
		parent->nodes[node->node_key] = NULL;
		parent->children--;
		free(node);
		node = parent;
		if (node->children == 0) {
			free(node->nodes);
			node->nodes = NULL;
		}
	}

	return res;
}

static void array_iterator_load_value(array_iterator_t *it) {
	++it->seen;
	it->keylen = it->node->value_keylen;
	it->key = it->node->value_key;
	it->value = it->node->value;
	it->is_type = it->node->value_is_type;
}

static bool array_iterator_find_next(array_iterator_t *it) {
	if (it->next == NULL) return false;
	struct array_node *node = it->next;
	if ((it->seen == 0) && (node->has_value)) {
		it->next = node;
		return true;
	}

	if (node->children) {
		for(int i = 0; i < 256; i++) {
			if (node->nodes[i] == NULL) continue;
			it->next = node->nodes[i];
			if (it->next->has_value)
				return true;
			if (array_iterator_find_next(it)) return true;
		}
	}

	// return to parent
	while(1) {
		if (node->parent == NULL) {
			it->next = NULL;
			return false;
		}
		node = node->parent;

		for(int i = it->next->node_key + 1; i < 256; i++) {
			if (node->nodes[i] == NULL) continue;
			it->next = node->nodes[i];
			if (it->next->has_value)
				return true;
			if (array_iterator_find_next(it)) return true;
		}
		it->next = node;
	}
}

array_iterator_t *array_iterator(array_t *array) {
	array_iterator_t *it = calloc(sizeof(array_iterator_t), 1);
	it->array = array;
	it->next = array->root;

	array_iterator_find_next(it);

	return it;
}

void array_iterator_free(array_iterator_t *it) {
	free(it);
}

bool array_next(array_iterator_t *it) {
	it->node = it->next;
	if (it->node == NULL) return false;
	array_iterator_load_value(it);
	array_iterator_find_next(it);
	return true;
}


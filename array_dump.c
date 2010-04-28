#include <stdio.h>
#include "array.h"

static void array_node_debug_pr(int level) {
	for(int i = 0; i < level; i++) printf("  ");
}

static void array_node_debug(struct array_node *node, int level, struct array_node *parent, uint8_t key) {
	if ((node->has_value) && (node->children > 0) && (node->value_keylen != level)) {
		array_node_debug_pr(level);
		printf("* BAD NODE (has both value and children while more levels are possible)\n");
	}
	if (node->parent != parent) {
		array_node_debug_pr(level);
		printf("* BAD NODE (invalid parent value, got %p while should be %p)\n", node->parent, parent);
	}
	if (node->node_key != key) {
		array_node_debug_pr(level);
		printf("* BAD NODE (invalid key, got %02hhx while shoud be %02hhx)\n", node->node_key, key);
	}
	if (node->has_value) {
		array_node_debug_pr(level);
		printf("%02hhx", node->value_key[0]);
		for(int i = 1; i < node->value_keylen; i++)
			printf(":%02hhx", node->value_key[i] & 0xff);
		printf(" = %p\n", node->value);
	}

	int count = 0;
	if (node->nodes != NULL) {
		for(int i = 0; i < 256; i++) {
			if (node->nodes[i] == NULL) continue;
			count++;
			array_node_debug_pr(level);
			printf("%02hhx (children=%d ptr=%p)\n", i, node->nodes[i]->children, node->nodes[i]);
			array_node_debug(node->nodes[i], level+1, node, i);
		}
	}

	if (count != node->children) {
		array_node_debug_pr(level);
		printf("* BAD NODE (invalid number of children, got %d while expecting %d)\n", count, node->children);
	}
}

void array_debug(array_t *array) {
	if (array->root == NULL) {
		printf("(empty)\n");
		return;
	}
	printf("array size=%d\n", array->count);
	array_node_debug(array->root, 0, NULL, 0);
}

static void array_node_dump(struct array_node *node, int level) {
	if (node->has_value) {
		printf("%02hhx", node->value_key[0]);
		for(int i = 1; i < node->value_keylen; i++)
			printf(":%02hhx", node->value_key[i] & 0xff);
		printf(" = %p\n", node->value);
	}

	if (node->nodes != NULL) {
		for(int i = 0; i < 256; i++) {
			if (node->nodes[i] != NULL)
				array_node_dump(node->nodes[i], level+1);
		}
	}
}

void array_dump(array_t *array) {
	if (array->root == NULL) {
		printf("(empty)\n");
		return;
	}
	array_node_dump(array->root, 0);
}


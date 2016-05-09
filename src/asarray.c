#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "asarray.h"

//Inits the array with the given keys
void init_ar(struct AsArray *asar, char *keys[]) {
	//Get size
	unsigned int i = 0;
	while (keys[i] != (char*) 0) i++;

	//Apply size
	asar->size = i;

	//Create arrays
	asar->key   = keys;
	asar->value = malloc(sizeof(char*) *asar->size);
}

//Scans for key, returns its value, or 0 if it doesn't exist
char* ar_get(struct AsArray *ar, const char* key) {
	for (unsigned int i = 0; i < ar->size; i++) {
		if ( strcmp(ar->key[i], key) == 0) {
			return ar->value[i];
		}
	}

	printf("error: '%s' not present in associative array\n", key);
	return 0;
}

//Scans for key, changes its value to the given onen.
//returns -1 if it doesn't exist
int ar_set(struct AsArray *ar, const char* key, char* value) {
	for (unsigned int i = 0; i < ar->size; i++) {
		if ( strcmp(ar->key[i], key) == 0) {
			ar->value[i] = value;
			return 0;
		}
	}

	printf("error: '%s' not present in associative array\n", key);
	return -1;
}

//Scans for key, returns 1 (true) if it exists, or 0 (false) if not
int ar_exists(struct AsArray *ar, const char* key) {
	for (unsigned int i = 0; i < ar->size; i++) {
		if ( strcmp(ar->key[i], key) == 0) {
			return 1;
		}
	}

	return 0;
}

//Scans for key, returns its value as int, or 0 if it doesn't exist
int ar_get_int(struct AsArray *ar, const char* key) {
	for (unsigned int i = 0; i < ar->size; i++) {
		if ( strcmp(ar->key[i], key) == 0) {
			return atoi(ar->value[i]);
		}
	}

	printf("error: '%s' not present in associative array\n", key);
	return 0;
}

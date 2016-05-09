#ifndef _AS_ARRAY_H_
#define _AS_ARRAY_H_

//Asosiative Array
struct AsArray {
	unsigned int size;
	char **key;
	char **value;
};

//Init
void init_ar(struct AsArray *asar, char *keys[]);

//Getters/setters
char* ar_get(struct AsArray *ar, const char* key);
int ar_get_int(struct AsArray *ar, const char* key);
int ar_set(struct AsArray *ar, const char* key, char* value);

//Check if key exists
int ar_exists(struct AsArray *ar, const char* key);

#endif

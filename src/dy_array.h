#ifndef _NODE_ARRAY_H_
#define _NODE_ARRAY_H_

#include "asarray.h"

struct dy_array {
	struct AsArray *ar;
	unsigned int elements;
	unsigned int size;
};

void dr_init(struct dy_array *dr);
void dr_add(struct dy_array *dr, struct AsArray ar);

#endif

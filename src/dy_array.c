#include <stdio.h>
#include <stdlib.h>
#include "dy_array.h"
#include "asarray.h"

void dr_init(struct dy_array *dr) {
	dr->ar = 0;
	dr->elements = 0;
	dr->size = 0;
}

void dr_add(struct dy_array *dr, struct AsArray ar) {
	if (dr->size == 0) {
		dr->size = 3;
	}
	else
		dr->size *= 2;

	struct AsArray *temp = realloc(dr->ar, sizeof(struct AsArray) *dr->size);

	if (!temp) {
		printf("failed to realloc\n");
	}

	dr->ar = temp;
	dr->ar[dr->elements++] = ar;
}

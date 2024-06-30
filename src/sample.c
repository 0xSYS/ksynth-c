#include "sample.h"
#include <stdbool.h>
#include <stdlib.h>

void sample_free(struct Sample* sample) {
	if(sample != NULL) {
		free(sample);
	}
}

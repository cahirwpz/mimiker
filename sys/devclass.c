#include <device.h>
#include <devclass.h>
#include <stdc.h>

SET_DECLARE(devclasses, devclass_t);

devclass_t *devclass_find(const char *classname){
	devclass_t **dc;

	SET_FOREACH (dc, devclasses) {
		if(strcmp((*dc)->name, classname) == 0)
			return *dc;
	}
	return NULL;
}

#include <psutils/psutils.h>

#include "sysdeps/sysdeps.h"

int psutils_init(void) {
	return psutils_sysdep_init();
}

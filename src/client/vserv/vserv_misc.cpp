#include <client/vserv/vserv_misc.h>

void * gs_aux_argown(void **ptr)
{
	void *ret = *ptr;
	*ptr = NULL;
	return ret;
}

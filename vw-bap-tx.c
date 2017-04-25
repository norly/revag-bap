#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/can.h>

#include "vw-bap.h"




struct BAP_TXer* vw_bap_txer_alloc()
{
	struct BAP_TXer *bap;

	bap = calloc(1, sizeof(*bap));
	if (!bap) {
		return NULL;
	}

	return bap;
}




void vw_bap_txer_free(struct BAP_TXer *bap)
{
	free(bap);
}

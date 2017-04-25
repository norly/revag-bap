#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/can.h>

#include "vw-bap.h"



int vw_bap_txer_build_first_can_frame(struct BAP_Frame *bap_frame, struct can_frame *frame)
{
	if (!vw_bap_frame_is_valid(bap_frame)) {
		return -1;
	}

	/* Build header */
	if (!bap_frame->is_multiframe) {
		frame->data[0]  = (bap_frame->opcode & 0x7) << 4;
		frame->data[0] |= (bap_frame->node & 0x3f) >> 2;
		frame->data[1]  = (bap_frame->node & 0x3) << 6;
		frame->data[1] |=  bap_frame->function & 0x3f;

		frame->can_dlc = 2 + bap_frame->len;
		memcpy(&frame->data[2], bap_frame->data, bap_frame->len);
	} else { /* bap->frame->is_multiframe */
		/* TODO, maybe: Support simultaneous streams */
		frame->data[0]  = 0x80;
		frame->data[0] |= (bap_frame->len & 0xfff) >> 8;
		frame->data[1]  =  bap_frame->len & 0xff;

		frame->data[2]  = (bap_frame->opcode & 0x7) << 4;
		frame->data[2] |= (bap_frame->node & 0x3f) >> 2;
		frame->data[3]  = (bap_frame->node & 0x3) << 6;
		frame->data[3] |=  bap_frame->function & 0x3f;

		if (bap_frame->len > 4) {
			return -2;
			/* TODO: Support continuation frames */
		} else {
			frame->can_dlc = 4 + bap_frame->len;
			memcpy(&frame->data[4], bap_frame->data, bap_frame->len);
		}
	}

	return 0;
}



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

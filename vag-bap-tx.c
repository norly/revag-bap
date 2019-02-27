#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/can.h>

#include "vag-bap.h"



static void vag_bap_txer_build_only_can_frame(struct BAP_Frame *bap_frame, struct can_frame *frame)
{
	assert(!bap_frame->is_multiframe);

	frame->data[0]  = (bap_frame->opcode & 0x7) << 4;
	frame->data[0] |= (bap_frame->node & 0x3f) >> 2;
	frame->data[1]  = (bap_frame->node & 0x3) << 6;
	frame->data[1] |=  bap_frame->port & 0x3f;

	frame->can_dlc = 2 + bap_frame->len;
	memcpy(&frame->data[2], bap_frame->data, bap_frame->len);
}


static void vag_bap_txer_build_first_can_frame(struct BAP_TXer* bap, unsigned slotnum, struct can_frame *frame)
{
	struct BAP_Frame *bap_frame = bap->slot[slotnum];

	assert(slotnum < 4);

	/* Build header */
	/* TODO, maybe: Support simultaneous streams */
	frame->data[0]  = 0x80 + slotnum;
	frame->data[0] |= (bap_frame->len & 0xfff) >> 8;
	frame->data[1]  =  bap_frame->len & 0xff;

	frame->data[2]  = (bap_frame->opcode & 0x7) << 4;
	frame->data[2] |= (bap_frame->node & 0x3f) >> 2;
	frame->data[3]  = (bap_frame->node & 0x3) << 6;
	frame->data[3] |=  bap_frame->port & 0x3f;

	if (bap_frame->len <= 4) {
		frame->can_dlc = 4 + bap_frame->len;
		memcpy(&frame->data[4], bap_frame->data, bap_frame->len);

		bap->slot[slotnum] = NULL;
	} else {
		/* Send first 4 data bytes */
		frame->can_dlc = 4 + 4;
		memcpy(&frame->data[4], bap_frame->data, 4);

		/* Note bytes done */
		bap->slot_done[slotnum] = 4;
	}
}



int vag_bap_txer_queue(struct BAP_TXer* bap, struct BAP_Frame *bap_frame, struct can_frame *frame)
{
	if (!vag_bap_frame_is_valid(bap_frame)) {
		return -EINVAL;
	}

	if (!bap_frame->is_multiframe) {
		vag_bap_txer_build_only_can_frame(bap_frame, frame);
		return 0;
	} else { /* bap->frame->is_multiframe */
		unsigned i;

		/* Find available slot */
		for (i = 0; i < 4; i++) {
			if (!bap->slot[i]) {
				break;
			}
		}
		if (i > 3) {
			return -EBUSY;
		}

		/* Found empty slot */
		bap->slot[i] = vag_bap_frame_clone(bap_frame);
		if (!bap->slot[i]) {
			return -ENOMEM;
		}

		vag_bap_txer_build_first_can_frame(bap, i, frame);

		if (bap->slot[i]) {
			return 1;
		}

		return 0;
	}
}



struct BAP_TXer* vag_bap_txer_alloc()
{
	struct BAP_TXer *bap;

	bap = calloc(1, sizeof(*bap));
	if (!bap) {
		return NULL;
	}

	return bap;
}




void vag_bap_txer_free(struct BAP_TXer *bap)
{
	free(bap);
}

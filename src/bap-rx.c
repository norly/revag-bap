#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/can.h>

#include "vag-bap.h"


struct BAP_Frame* vag_bap_handle_can_frame(struct BAP_RXer *bap, struct can_frame *frame)
{
	struct BAP_Frame *bap_frame = NULL;
	unsigned short header;
	unsigned this_len;

	//printf("Received BAP frame from CAN ID %03x\n", frame->can_id);

	if (frame->can_dlc < 2) {
		VAG_DEBUG("Error: Frame too short\n");
	}

	if (frame->data[0] & 0x80) {
		/* This is a multi-frame BAP message */
		int mfchannel = (frame->data[0] >> 4) & 0x3;

		if (!(frame->data[0] & 0x40)) {
			/* Start frame */

			unsigned short header;
			unsigned this_len;

			/* Sanity checks */
			if (frame->can_dlc < 4) {
				VAG_DEBUG("bap_handle_can_frame: Frame too short\n");
			}

			if (bap->mfchannel[mfchannel]) {
				VAG_DEBUG("bap_handle_can_frame: new start frame for open channel\n");
			}
			bap->mfchannel[mfchannel] = NULL;

			/* Frame looks okay, start parsing */
			bap_frame = vag_bap_frame_alloc();
			if (!bap_frame) {
				VAG_DEBUG("bap_handle_can_frame: Failed to allocate new frame\n");
				return NULL;
			}

			bap_frame->is_multiframe = 1;

			header = (frame->data[2] << 8) | frame->data[3];
			bap_frame->opcode = (header >> 12) & 0x7;
			bap_frame->node = (header >> 6) & 0x3F;
			bap_frame->port = (header >> 0) & 0x3F;

			bap_frame->len = ((frame->data[0] & 0xF) << 8) | frame->data[1];

			this_len = frame->can_dlc - 4;

			if (this_len > bap_frame->len) {
				VAG_DEBUG("bap_handle_can_frame: this_len > len\n");

				free(bap_frame);
				bap->mfchannel[mfchannel] = NULL;

				return NULL;
			}

			memcpy(&bap_frame->data[0], &frame->data[frame->can_dlc - this_len], this_len);
			bap->len_done[mfchannel] = this_len;

			if (bap->len_done[mfchannel] == bap_frame->len) {
				/* Frame is complete, remove from buffer */
				bap->mfchannel[mfchannel] = NULL;
				return bap_frame;
			} else {
				/* Frame is incomplete, don't emit it yet */
				bap->mfchannel[mfchannel] = bap_frame;
				return NULL;
			}
		} else {
			/* Continuation frame */

			bap_frame = bap->mfchannel[mfchannel];

			if (!bap_frame) {
				VAG_DEBUG("bap_handle_can_frame: continuation frame for unknown mf channel %d\n",
					mfchannel);
			}

			this_len = frame->can_dlc - 1;

			if ((bap->len_done[mfchannel] + this_len) > bap_frame->len) {
				VAG_DEBUG("bap_handle_can_frame: len_done + this_len > len\n");

				free(bap_frame);
				bap->mfchannel[mfchannel] = NULL;

				return NULL;
			}

			memcpy(&bap_frame->data[bap->len_done[mfchannel]],
				&frame->data[frame->can_dlc - this_len],
				this_len);
			bap->len_done[mfchannel] += this_len;

			if (bap->len_done[mfchannel] == bap_frame->len) {
				/* Frame is complete, remove from buffer */
				bap->mfchannel[mfchannel] = NULL;
				return bap_frame;
			} else {
				/* Frame is incomplete, don't emit it yet */
				return NULL;
			}
		}
	} else {
		/* Single-frame BAP message */

		bap_frame = calloc(1, sizeof(struct BAP_Frame));
		if (!bap_frame) {
			VAG_DEBUG("bap_handle_can_frame: Failed to allocate new frame\n");
			return NULL;
		}

		bap_frame->is_multiframe = 0;

		header = (frame->data[0] << 8) | frame->data[1];
		bap_frame->opcode = (header >> 12) & 0x7;
		bap_frame->node = (header >> 6) & 0x3F;
		bap_frame->port = (header >> 0) & 0x3F;

		this_len = frame->can_dlc - 2;

		bap_frame->len = this_len;

		memcpy(&bap_frame->data[0], &frame->data[frame->can_dlc - this_len], this_len);

		return bap_frame;
	}
}





struct BAP_RXer* vag_bap_rxer_alloc()
{
	struct BAP_RXer *bap;

	bap = calloc(1, sizeof(*bap));
	if (!bap) {
		return NULL;
	}

	return bap;
}




void vag_bap_rxer_free(struct BAP_RXer *bap)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (bap->mfchannel[i]) {
			vag_bap_frame_free(bap->mfchannel[i]);
		}
	}

	free(bap);
}

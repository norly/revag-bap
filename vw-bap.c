#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/can.h>

#include "vw-bap.h"


void vw_bap_frame_dump(struct BAP_Frame *bap_frame)
{
	unsigned i;

	printf("%u. %2i/%-2i .%02i --",
		bap_frame->opcode,
		bap_frame->subnode,
		bap_frame->function,
		bap_frame->len);

	for (i = 0; i < bap_frame->len; i++) {
		if (!(i % 4)) {
			printf(" ");
		}
		printf("%02x", (unsigned char)(bap_frame->data[i]));
	}

	printf("\n        '");
	for (i = 0; i < bap_frame->len; i++) {
		unsigned char c = bap_frame->data[i];
		if (!isprint(c) || c == '\n' || c == '\r') {
			c = '.';
		}
		printf("%c", c);
	}
	printf("'");

	printf("\n");

	fflush(stdout);
}





struct BAP_Frame* vw_bap_handle_can_frame(struct BAP_RXer *bap, struct can_frame *frame)
{
	struct BAP_Frame *bap_frame = NULL;
	unsigned short header;
	unsigned this_len;

	//printf("Received BAP frame from CAN ID %03x\n", frame->can_id);

	if (frame->can_dlc < 2) {
		printf("Error: Frame too short\n");
	}

	if (frame->data[0] & 0x80) {
		/* This is a multi-frame BAP message */
		int mfchannel = (frame->data[0] >> 4) & 0x3;

		if (!(frame->data[0] & 0x40)) {
			/* Start frame */

			unsigned short header;
			unsigned this_len;

			if (frame->can_dlc < 4) {
				printf("Error: Frame too short\n");
			}

			if (bap->mfchannel[mfchannel]) {
				printf("bap_handle_can_frame: new start frame for open channel\n");
			}
			bap->mfchannel[mfchannel] = NULL;

			bap_frame = calloc(1, sizeof(struct BAP_Frame));
			if (!bap_frame) {
				printf("bap_handle_can_frame: Failed to allocate new frame\n");
				return NULL;
			}

			header = (frame->data[2] << 8) | frame->data[3];
			bap_frame->opcode = (header >> 12) & 0x7;
			bap_frame->subnode = (header >> 6) & 0x3F;
			bap_frame->function = (header >> 0) & 0x3F;

			bap_frame->len = ((frame->data[0] & 0xF) << 8) | frame->data[1];

			this_len = frame->can_dlc - 4;

			if (this_len > bap_frame->len) {
				printf("bap_handle_can_frame: this_len > len\n");

				free(bap_frame);
				bap->mfchannel[mfchannel] = NULL;

				return NULL;
			}

			memcpy(&bap_frame->data[0], &frame->data[frame->can_dlc - this_len], this_len);
			bap_frame->len_done = this_len;

			if (bap_frame->len_done == bap_frame->len) {
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
				printf("bap_handle_can_frame: continuation frame for unknown mf channel %d\n",
					mfchannel);
			}

			this_len = frame->can_dlc - 1;

			if ((bap_frame->len_done + this_len) > bap_frame->len) {
				printf("bap_handle_can_frame: len_done + this_len > len\n");

				free(bap_frame);
				bap->mfchannel[mfchannel] = NULL;

				return NULL;
			}

			memcpy(&bap_frame->data[bap_frame->len_done],
				&frame->data[frame->can_dlc - this_len],
				this_len);
			bap_frame->len_done += this_len;

			if (bap_frame->len_done == bap_frame->len) {
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
			printf("bap_handle_can_frame: Failed to allocate new frame\n");
			return NULL;
		}

		header = (frame->data[0] << 8) | frame->data[1];
		bap_frame->opcode = (header >> 12) & 0x7;
		bap_frame->subnode = (header >> 6) & 0x3F;
		bap_frame->function = (header >> 0) & 0x3F;

		this_len = frame->can_dlc - 2;

		bap_frame->len = this_len;

		memcpy(&bap_frame->data[0], &frame->data[frame->can_dlc - this_len], this_len);
		bap_frame->len_done = this_len;

		return bap_frame;
	}
}





struct BAP_RXer* vw_bap_alloc()
{
	struct BAP_RXer *bap;

	bap = calloc(1, sizeof(*bap));
	if (!bap) {
		return NULL;
	}

	return bap;
}




void vw_bap_free(struct BAP_RXer *bap)
{
	/* TODO */
	free(bap);
}



void vw_bap_frame_free(struct BAP_Frame *bap_frame)
{
	/* TODO */
	free(bap_frame);
}

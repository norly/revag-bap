#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vw-bap.h"



struct BAP_Frame* vw_bap_frame_alloc(void)
{
	struct BAP_Frame* bap_frame;

	bap_frame = calloc(1, sizeof(struct BAP_Frame));

	return bap_frame;
}


void vw_bap_frame_free(struct BAP_Frame *bap_frame)
{
	free(bap_frame);
}


int vw_bap_frame_is_valid(struct BAP_Frame *bap_frame)
{
	if ( 0
		|| (bap_frame->opcode > 7)
		|| (bap_frame->node > 63)
		|| (bap_frame->port > 63)
		|| (bap_frame->len > 4095)
		|| (!bap_frame->is_multiframe && bap_frame->len > 6)
		) {
		return 0;
	}

	return 1;
}


struct BAP_Frame* vw_bap_frame_clone(struct BAP_Frame *bap_frame)
{
	struct BAP_Frame *new_frame;

	if (!vw_bap_frame_is_valid(bap_frame)) {
		return NULL;
	}

	new_frame = vw_bap_frame_alloc();
	if (!new_frame) {
		return NULL;
	}

	memcpy(new_frame, bap_frame, sizeof(*new_frame));

	return new_frame;
}



void vw_bap_frame_dump(struct BAP_Frame *bap_frame)
{
	unsigned i;

	printf("%u. %2i/%-2i .%02i --",
		bap_frame->opcode,
		bap_frame->node,
		bap_frame->port,
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

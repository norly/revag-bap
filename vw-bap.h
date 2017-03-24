#ifndef __VW_BAP_H__
#define __VW_BAP_H__

#include <linux/can.h>


typedef unsigned char BAP_OpCode;
typedef unsigned char BAP_Node;
typedef unsigned char BAP_Function;
typedef unsigned short BAP_FrameLen;


struct BAP_Frame {
	/* True if frame was/will be transmitted in multi-frame syntax */
	int is_multiframe;

	BAP_OpCode opcode;
	BAP_Node node;
	BAP_Function function;
	BAP_FrameLen len;

	char data[4096];
};


struct BAP_RXer {
	/* Temporary storage for incomplete frames */
	struct BAP_Frame *mfchannel[8];

	/* How many bytes have we already received on each channel? */
	BAP_FrameLen len_done[8];
};





void vw_bap_frame_dump(struct BAP_Frame *bap_frame);

struct BAP_Frame* vw_bap_handle_can_frame(struct BAP_RXer *bap, struct can_frame *frame);

void vw_bap_frame_free(struct BAP_Frame *bap_frame);

struct BAP_RXer* vw_bap_alloc();
void vw_bap_free(struct BAP_RXer *bap);


#endif

#ifndef __VW_BAP_H__
#define __VW_BAP_H__

#include <linux/can.h>


typedef unsigned char BAP_OpCode;
typedef unsigned char BAP_SubNode;
typedef unsigned char BAP_SubFunction;
typedef unsigned short BAP_FrameLen;


struct BAP_Frame {
	BAP_OpCode opcode;
	BAP_SubNode subnode;
	BAP_SubFunction function;
	BAP_FrameLen len;
	BAP_FrameLen len_done;
	char data[4096];
};


struct BAP_RXer {
	struct BAP_Frame *mfchannel[8];
};





void bap_frame_dump(struct BAP_Frame *bap_frame);

struct BAP_Frame* bap_handle_can_frame(struct BAP_RXer *bap, struct can_frame *frame);

struct BAP_RXer* bap_alloc();
void bap_free(struct BAP_RXer *bap);
void bap_frame_free(struct BAP_Frame *bap_frame);



#endif

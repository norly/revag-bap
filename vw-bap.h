#ifndef __VW_BAP_H__
#define __VW_BAP_H__

#include <linux/can.h>


typedef unsigned char BAP_OpCode;
typedef unsigned char BAP_Node;
typedef unsigned char BAP_Function;
typedef unsigned short BAP_FrameLen;


/* A BAP frame at the BCL (BAP communication layer) as defined in
 * http://www.itwissen.info/BCL-BAP-communication-layer.html
 *
 * This is basically BAP's equivalent of IP and TCP.
 */
struct BAP_Frame {
	/* True if frame was/will be transmitted in multi-frame syntax */
	int is_multiframe;


	/* Request/reply, kind of */
	BAP_OpCode opcode;

	/* LSG = Logisches Steuergeraet
	 * https://www.springerprofessional.de/technische-informatik/eingebettete-systeme/neues-protokoll-vereinfacht-kommunikation-von-steuergeraeten/6592480
	 *
	 * BAP's equivalent of an IP address (to be confirmed).
	 *
	 * Always the same per CAN ID in the (simple) devices I looked at.
	 */
	BAP_Node node;

	/* The "RPC" function, or "status register" ID.
	 *
	 * BAP's equivalent of a "TCP port".
	 */
	BAP_Function function;

	/* Payload length, up to 2^12 = 4096 bytes.
	 *
	 * 4095 bytes according to:
	 * http://www.itwissen.info/BCL-BAP-communication-layer.html
	 */
	BAP_FrameLen len;


	/* Payload */
	char data[4096];
};



struct BAP_RXer {
	/* Temporary storage for incomplete frames */
	struct BAP_Frame *mfchannel[8];

	/* How many bytes have we already received on each channel? */
	BAP_FrameLen len_done[8];
};



/* BAP frame struct handling */
struct BAP_Frame* vw_bap_frame_alloc(void);
             void vw_bap_frame_free(struct BAP_Frame *bap_frame);

int vw_bap_frame_is_valid(struct BAP_Frame *bap_frame);

void vw_bap_frame_dump(struct BAP_Frame *bap_frame);


/* BAP reception */
struct BAP_Frame* vw_bap_handle_can_frame(struct BAP_RXer *bap, struct can_frame *frame);
struct BAP_RXer* vw_bap_rxer_alloc();
void vw_bap_rxer_free(struct BAP_RXer *bap);


#endif

#ifndef __VAG_BAP_H__
#define __VAG_BAP_H__

#include <stdio.h>
#define VAG_DEBUG(...) do { fprintf(stdout, __VA_ARGS__); } while(0)

#include <linux/can.h>


typedef unsigned char BAP_OpCode;
typedef unsigned char BAP_Node;
typedef unsigned char BAP_Port;
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

	/* The "RPC" port, or "status register" ID.
	 *
	 * BAP's equivalent of a "UDP port".
	 */
	BAP_Port port;

	/* Payload length, up to 2^12-1 = 4095 bytes. */
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


struct BAP_TXer {
	/* Temporary storage for frames not yet fully sent */
	struct BAP_Frame *slot[4];

	/* How many bytes have we already sent on each open slot? */
	BAP_FrameLen slot_done[4];
};



/* BAP frame struct handling */
struct BAP_Frame* vag_bap_frame_alloc(void);
             void vag_bap_frame_free(struct BAP_Frame *bap_frame);

int vag_bap_frame_is_valid(struct BAP_Frame *bap_frame);

struct BAP_Frame* vag_bap_frame_clone(struct BAP_Frame *bap_frame);

void vag_bap_frame_dump(struct BAP_Frame *bap_frame);


/* BAP reception */
struct BAP_Frame* vag_bap_handle_can_frame(struct BAP_RXer *bap, struct can_frame *frame);
struct BAP_RXer* vag_bap_rxer_alloc();
void vag_bap_rxer_free(struct BAP_RXer *bap);


/* BAP transmission */
int vag_bap_txer_queue(struct BAP_TXer* bap, struct BAP_Frame *bap_frame, struct can_frame *frame);
struct BAP_TXer* vag_bap_txer_alloc();
void vag_bap_txer_free(struct BAP_TXer *bap);


#endif

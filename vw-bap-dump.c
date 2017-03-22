/*
 * Copyright 2017 Max Staudt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License 2 as published
 * by the Free Software Foundation.
 */

#include <ctype.h>

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <endian.h>
#include <sys/time.h>




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





static void bap_frame_dump(struct BAP_Frame *bap_frame)
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





static struct BAP_Frame* bap_handle_can_frame(struct BAP_RXer *bap, struct can_frame *frame)
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





static int net_init(char *ifname)
{
        int s;
	int recv_own_msgs;
	struct sockaddr_can addr;
	struct ifreq ifr;

	s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (s < 0) {
		perror("socket");
		exit(1);
	}

	/* Convert interface name to index */
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		perror("SIOCGIFINDEX");
		exit(1);
	}

	/* Open the CAN interface */
	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 0;
	}

	recv_own_msgs = 1; /* 0 = disabled (default), 1 = enabled */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
			&recv_own_msgs, sizeof(recv_own_msgs));

	return s;
}



static struct BAP_RXer* bap_alloc()
{
	struct BAP_RXer *bap;

	bap = calloc(1, sizeof(*bap));
	if (!bap) {
		return NULL;
	}

	return bap;
}




static void bap_free(struct BAP_RXer *bap)
{
	/* TODO */
	free(bap);
}



static void bap_frame_free(struct BAP_Frame *bap_frame)
{
	/* TODO */
	free(bap_frame);
}





int main(int argc, char **argv)
{
  	fd_set rdfs;
	int s;
	int i;
	int can_id_count;
	unsigned *can_ids;
	struct BAP_RXer **baps;

	if (argc < 3) {
		printf("syntax: %s IFNAME CAN_ID [CAN_ID CAN_ID ...]\n", argv[0]);
		return 1;
	}

	can_id_count = argc - 2;

	can_ids = calloc(can_id_count, sizeof(*can_ids));
	baps = calloc(can_id_count, sizeof(*baps));
	if (!can_ids || !baps) {
		printf("Out of memory allocating meta structs.\n");
		return 1;
	}

	for (i = 0; i < can_id_count; i++) {
		can_ids[i] = strtoul(argv[2 + i], NULL, 0);
		baps[i] = bap_alloc();
		if (!baps[i]) {
			printf("Out of memory allocating BAP struct.\n");
			return 1;
		}

		printf("Listening for CAN ID: %x\n", can_ids[i]);
	}


	s = net_init(argv[1]);

	while (1) {
		int retval;

		FD_ZERO(&rdfs);
		FD_SET(s, &rdfs);

		retval = select(s+1, &rdfs, NULL, NULL, NULL);
		/* We currently rely on Linux timeout behavior here,
		 * i.e. the timeout now reflects the remaining time */
		if (retval < 0) {
			perror("select");
			return 1;
		} else if (!retval) {
			/* Timeout, we NEED to check this first */
		} else if (FD_ISSET(s, &rdfs)) {
			struct can_frame frame;
			ssize_t ret;

			ret = read(s, &frame, sizeof(frame));
			if (ret < 0) {
				perror("read from CAN socket");
				return 1;
			}

			for (i = 0; i < can_id_count; i++) {
				if (can_ids[i] == frame.can_id) {
					struct BAP_Frame *bap_frame;
					struct BAP_RXer *bap = baps[i];

					bap_frame = bap_handle_can_frame(bap, &frame);
					if (bap_frame) {
						printf("%03x:  ", frame.can_id);
						bap_frame_dump(bap_frame);
						bap_frame_free(bap_frame);
					}

					break;
				}
			}

			continue;
		}
	}

	/* TODO: Free */

	close(s);

	return 0;
}

/*
 * Copyright 2017 Max Staudt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License 2 as published
 * by the Free Software Foundation.
 */

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

#include "vag-bap.h"


static unsigned hex2bin(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'A' && c <= 'F') {
		return c + 10 - 'A';
	} else if (c >= 'a' && c <= 'f') {
		return c + 10 - 'a';
	} else {
		return 127;
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



static void can_tx(int socket, struct can_frame *frame)
{
	ssize_t ret;

	ret = write(socket, frame, sizeof(*frame));
	if (ret != sizeof(*frame)) {
		perror("write to CAN socket");
		exit(1);
	}
}




int main(int argc, char **argv)
{
  	fd_set wfds;
	int s, ret;
	struct can_frame frame;
	struct BAP_TXer *bap;
	struct BAP_Frame *bap_frame;
	unsigned can_id;
	unsigned bap_multiframe;
	unsigned bap_opcode;
	unsigned bap_node;
	unsigned bap_port;

	if (argc < 8) {
		printf("syntax: %s IFNAME CAN_ID multiframe bap_opcode bap_node bap_port bap_data\n", argv[0]);
		return 1;
	}

	/* Parse arguments */
	can_id = strtoul(argv[2], NULL, 0);
	bap_multiframe = strtoul(argv[3], NULL, 0);
	bap_opcode = strtoul(argv[4], NULL, 0);
	bap_node = strtoul(argv[5], NULL, 0);
	bap_port = strtoul(argv[6], NULL, 0);
	/* bap_data */


	bap = vag_bap_txer_alloc();
	if (!bap) {
		printf("Out of memory allocating BAP TXer struct.\n");
		return 1;
	}

	bap_frame = vag_bap_frame_alloc();
	if (!bap_frame) {
		printf("Out of memory allocating BAP frame.\n");
		return 1;
	}



	/* Fill frame */
	bap_frame->is_multiframe = !(!bap_multiframe);
	bap_frame->opcode = bap_opcode & 0x7;
	bap_frame->node = bap_node & 0x3f;
	bap_frame->port = bap_port & 0x3f;
	bap_frame->len = 0;

	/* Fill payload */
	while(bap_frame->len < 4095) {
		unsigned high, low;
		unsigned i = bap_frame->len;

		if (!argv[7][2*i] || !argv[7][2*i + 1]) {
			break;
		}

		high = hex2bin(argv[7][2*i]);
		if (high > 15) {
			break;
		}
		low = hex2bin(argv[7][2*i + 1]);
		if (low > 15) {
			break;
		}

		bap_frame->data[i] = (high << 4) | low;
		bap_frame->len++;
	}



	s = net_init(argv[1]);


	/* Fill TXer */

	printf("Will send frame:\n");
	vag_bap_frame_dump(bap_frame);

	ret = vag_bap_txer_queue(bap, bap_frame, &frame);
	frame.can_id = can_id;
	can_tx(s, &frame);

	/* HACK since we don't have continuation frames yet */
	return 0;


	while (1) {
		int retval;

		FD_ZERO(&wfds);
		FD_SET(s, &wfds);

		retval = select(s+1, NULL, &wfds, NULL, NULL);
		/* We currently rely on Linux timeout behavior here,
		 * i.e. the timeout now reflects the remaining time */
		if (retval < 0) {
			perror("select");
			return 1;
		} else if (!retval) {
			/* Timeout, we NEED to check this first */
		} else if (FD_ISSET(s, &wfds)) {

			continue;
		}
	}



	vag_bap_txer_free(bap);

	close(s);

	return 0;
}

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

#include "vw-bap.h"




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
		baps[i] = vw_bap_rxer_alloc();
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

					bap_frame = vw_bap_handle_can_frame(bap, &frame);
					if (bap_frame) {
						printf("%03x:  ", frame.can_id);
						vw_bap_frame_dump(bap_frame);
						vw_bap_frame_free(bap_frame);
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

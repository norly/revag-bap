CFLAGS += -Wall -g

all: vw-bap-dump vw-bap-sniffer


vw-bap-dump: vw-bap-dump.c vw-bap.c

vw-bap-sniffer: vw-bap-sniffer.c vw-bap.c
	gcc -o $@ $^ -lncurses

CFLAGS += -Wall -g

BAPLIB = vw-bap.c vw-bap-frame.c vw-bap-rx.c

all: vw-bap-dump vw-bap-sniffer


vw-bap-dump: vw-bap-dump.c $(BAPLIB)

vw-bap-sniffer: vw-bap-sniffer.c $(BAPLIB)
	gcc -o $@ $^ -lncurses

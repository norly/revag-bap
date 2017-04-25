CFLAGS += -Wall -g

BAPLIB = vw-bap.c vw-bap-frame.c vw-bap-rx.c vw-bap-tx.c

all: vw-bap-dump vw-bap-sniffer vw-bap-send


vw-bap-dump: vw-bap-dump.c $(BAPLIB)

vw-bap-sniffer: vw-bap-sniffer.c $(BAPLIB)
	gcc -o $@ $^ -lncurses

vw-bap-send: vw-bap-send.c $(BAPLIB)

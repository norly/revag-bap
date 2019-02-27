CFLAGS += -Wall -g

BAPLIB = vag-bap.c vag-bap-frame.c vag-bap-rx.c vag-bap-tx.c

EXES = vag-bap-dump vag-bap-sniffer vag-bap-send


all: $(EXES)


vag-bap-dump: vag-bap-dump.c $(BAPLIB)

vag-bap-sniffer: vag-bap-sniffer.c $(BAPLIB)
	gcc -o $@ $^ -lncurses

vag-bap-send: vag-bap-send.c $(BAPLIB)


.PHONY: clean
clean:
	rm -f $(EXES)

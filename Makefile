LDFLAGS=-largtable2 -lrt -lm
lux: lux.c zwave.o
install: lux
	install lux /usr/bin

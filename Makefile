LDFLAGS=-largtable2 -lrt -lm
all: lux boil
lux: lux.c zwave.o
boil: boil.c zwave.o
install: lux boil
	install lux /usr/bin
	install boil /usr/bin

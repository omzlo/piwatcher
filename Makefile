
CC=gcc

all:	piwatcher

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

piwatcher:	main.o i2c.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f piwatcher *.o

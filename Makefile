CC=gcc
CFLAGS= -pthread -I. -O0

.PHONY:clean

app: main.o my_utils.o rtp.o 
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) *.o

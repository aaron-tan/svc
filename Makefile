CC=gcc
CFLAGS=-O0 -std=gnu11 -Wextra -fsanitize=address -g
DEPS=svc.h helper.h clean.h
OBJ=tester.o svc.o helper.o clean.o


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tester: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	rm *.o

clean:
	rm *.o

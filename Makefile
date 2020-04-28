CC=gcc
CFLAGS=-O0 -std=gnu11 -Wextra -fsanitize=address -g
DEPS=svc.h helper.h
OBJ=tester.o svc.o helper.o


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tester: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.o

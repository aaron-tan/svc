CC=gcc
CFLAGS=-O0 -std=gnu11 -lm -Wextra -fsanitize=address -g
DEPS=svc.h
OBJ=svc.o tester.o


%.o: %.c %.h $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tester: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.o

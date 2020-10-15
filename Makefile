CC=gcc
CFLAGS=-O0 -std=gnu11 -Wextra -g
CFLAG_SAN=$(CFLAGS) -fsanitize=address
DEPS=svc.h
HELPER_DEPS=utils/helper.c utils/helper.h
CLEAN_DEPS=utils/clean.c utils/clean.h
OBJ=tester.o svc.o helper.o clean.o file.o track.o
SAN_OBJ=tester.o svc.o helper_san.o clean_san.o file_san.o

asan:
	$(CC) -c -o helper.o utils/helper.c $(CFLAG_SAN)
	$(CC) -c -o clean.o utils/clean.c $(CFLAG_SAN)
	$(CC) -c -o track.o core/track.c $(CFLAG_SAN)
	$(CC) -c -o file.o core/file.c $(CFLAG_SAN)
	$(CC) -c -o svc.o svc.c $(CFLAG_SAN)
	$(CC) -o tester tester.c helper.o clean.o svc.o track.o file.o $(CFLAG_SAN)
	rm *.o

tester: $(OBJ)
	$(CC) -o $@ $^
	rm *.o

helper.o: utils/helper.c utils/helper.h
	$(CC) -c -o $@ $< $(CFLAGS)

helper_san.o: utils/helper.c utils/helper.h
	$(CC) -c -o $@ $< $(CFLAGS_SAN)

clean.o: utils/clean.c utils/clean.h
	$(CC) -c -o $@ $< $(CFLAGS)

file.o: core/file.c core/file.h
	$(CC) -c -o $@ $< $(CFLAGS)

track.o: core/track.c core/track.h
	$(CC) -c -o $@ $< $(CFLAGS)

clean_san.o: utils/clean.c utils/clean.h
	$(CC) -c -o $@ $< $(CFLAGS_SAN)

clean:
	rm *.o

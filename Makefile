CC = gcc
CFLAGS = -Wall -O2 -g
COMPILE = $(CC) $(CFLAGS) -c -g -m64

all: test

test: main.o parser.o jsonbare.o sh_bighash.o sh_hash.o sh_hash_func.o sh_page.o sh_page_manager.o
	$(CC) -o test main.o parser.o jsonbare.o sh_bighash.o sh_hash.o sh_hash_func.o sh_page.o sh_page_manager.o

combined: parser.o jsonbare.o sh_bighash.o sh_hash.o sh_hash_func.o sh_page.o sh_page_manager.o
	ld -r parser.o jsonbare.o sh_bighash.o sh_hash.o sh_hash_func.o sh_page.o sh_page_manager.o -o combined.o

parser.o: parser.c
	$(COMPILE) -o parser.o parser.c

main.o: main.c
	$(COMPILE) -o main.o main.c

jsonbare.o: jsonbare.c
	$(COMPILE) -o jsonbare.o jsonbare.c

sh_bighash.o: sh_bighash.c
	$(COMPILE) -o sh_bighash.o sh_bighash.c

sh_hash.o: sh_hash.c
	$(COMPILE) -o sh_hash.o sh_hash.c

sh_hash_func.o: sh_hash_func.c
	$(COMPILE) -o sh_hash_func.o sh_hash_func.c

sh_page.o: sh_page.c
	$(COMPILE) -o sh_page.o sh_page.c

sh_page_manager.o: sh_page_manager.c
	$(COMPILE) -o sh_page_manager.o sh_page_manager.c

clean:	
	rm -rf *.o test.exe test

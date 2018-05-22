CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes -lstdc++
LIBS=
CC=gcc
AR=ar

HEADERS=bitmap.h\
	disk_driver.h\
	simplefs.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS)

so_game: file_system_test.c $(OBJS)
	$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

fs: file_system_test.c bitmap.c disk_driver.c simplefs.c $(HEADERS)
	$(CC) $(CCOPTS) file_system_test.c bitmap.c disk_driver.c simplefs.c -o file_system

clean:
	rm -rf *.o *~ file_system_test disk.txt

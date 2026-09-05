CC = gcc
MALLOC_COUNT_DIR = ../include/malloc_count-0.7.1
CFLAGS = -O9 -I$(MALLOC_COUNT_DIR)

ALL_OBJS = array.o hash.o heap.o records.o basics.o malloc_count.o
COMMON_HDRS = array.h hash.h heap.h records.h basics.h $(MALLOC_COUNT_DIR)/malloc_count.h makefile

all: repair despair irepair idespair toint repair.tgz

repair.tgz: *.c *.h makefile COPYRIGHT README bal/*.c bal/*.h bal/makefile bal/README large/*.c large/*.h large/makefile large/README large/bal/*.c large/bal/*.h large/bal/makefile large/bal/README
	tar -czvf repair.tgz *.c *.h makefile COPYRIGHT README bal/*.c bal/*.h bal/makefile bal/README large/*.c large/*.h large/makefile large/README large/bal/*.c large/bal/*.h large/bal/makefile large/bal/README

# EXECUTABLES
repair: repair.o $(ALL_OBJS)
	$(CC) $(CFLAGS) -o repair repair.o $(ALL_OBJS)

despair: despair.o basics.o malloc_count.o makefile
	$(CC) $(CFLAGS) -o despair despair.o basics.o malloc_count.o

irepair: irepair.o $(ALL_OBJS)
	$(CC) $(CFLAGS) -o irepair irepair.o $(ALL_OBJS)

idespair: idespair.o basics.o malloc_count.o makefile
	$(CC) $(CFLAGS) -o idespair idespair.o basics.o malloc_count.o

toint: toint.c malloc_count.o $(MALLOC_COUNT_DIR)/malloc_count.h makefile
	$(CC) $(CFLAGS) -o toint toint.c malloc_count.o

# OBJECT FILES
malloc_count.o: $(MALLOC_COUNT_DIR)/malloc_count.c $(MALLOC_COUNT_DIR)/malloc_count.h makefile
	$(CC) $(CFLAGS) -c $(MALLOC_COUNT_DIR)/malloc_count.c

repair.o: repair.c $(COMMON_HDRS)
	$(CC) $(CFLAGS) -c repair.c

despair.o: despair.c basics.h $(MALLOC_COUNT_DIR)/malloc_count.h makefile
	$(CC) $(CFLAGS) -c despair.c

irepair.o: irepair.c $(COMMON_HDRS)
	$(CC) $(CFLAGS) -c irepair.c

idespair.o: idespair.c basics.h $(MALLOC_COUNT_DIR)/malloc_count.h makefile
	$(CC) $(CFLAGS) -c idespair.c

array.o: array.c $(COMMON_HDRS)
	$(CC) $(CFLAGS) -c array.c

hash.o: hash.c $(COMMON_HDRS)
	$(CC) $(CFLAGS) -c hash.c

heap.o: heap.c $(COMMON_HDRS)
	$(CC) $(CFLAGS) -c heap.c

records.o: records.c $(COMMON_HDRS)
	$(CC) $(CFLAGS) -c records.c

basics.o: basics.c basics.h $(MALLOC_COUNT_DIR)/malloc_count.h makefile
	$(CC) $(CFLAGS) -c basics.c

clean:
	rm -f *.o repair despair irepair idespair toint repair.tgz
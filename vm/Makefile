TARG=vm.a
OFILES=$(shell ls *.c | sed -e 's|\.c|\.o|g')
CFLAGS=-Wall -I . -I ..

all: $(TARG) Makefile
clean:
	- $(RM) *.o *.a
%.o: %.c ops.h vm.h
	$(CC) -c $*.c $(CFLAGS)
$(TARG): $(OFILES)
	$(AR) r $(TARG) $?

.PHONY: all clean
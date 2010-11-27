TARG=rb_tree.a
OFILES=$(shell ls *.c | sed -e 's|\.c|\.o|g')
CFLAGS=-Wall -U DEBUG_ASSERT -I .

all: $(TARG) Makefile
clean:
	- $(RM) *.o *.a
%.o: %.c %.h
	$(CC) -c $*.c $(CFLAGS)
$(TARG): $(OFILES)
	$(AR) r $(TARG) $?
.PHONY: all clean
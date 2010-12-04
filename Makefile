TARG=brain
OFILES=$(shell ls *.c | sed -e 's|\.c|\.o|g')
MODULES=vm rb_tree
CFLAGS=-Wall -I . -I vm/ -I rb_tree/
BUILDDIR=$(shell pwd)

all: $(TARG) Makefile
clean: ${MODULES:%=%-clean}
	- $(RM) *.o brain
%-clean:
	cd $*; $(MAKE) clean $(MFLAGS)
%.o: %.c
	$(CC) -c $^ $(CFLAGS)
%.a: force_look
	cd $*; $(MAKE) $(MFLAGS); cp $@ $(BUILDDIR)
brain: $(OFILES) ${MODULES:%=%.a}
	$(CC) -o $@ $^ $(LDFLAGS)
force_look:
	true
.PHONY: all clean %-clean %.a

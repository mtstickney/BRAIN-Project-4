all: brain Makefile
clean:
	- rm -f *.o brain
%.o: %.c
	gcc -c -Wall $^ -I vm/ -I rb_tree/ -I .
vm.o: vm/vm.c vm/vm.h
	gcc -c -Wall $^ -I vm/ -I .
stack.o: rb_tree/stack.c
	gcc -c -Wall $^ -I rb_tree/ -U DEBUG_ASSERT
red_black_tree.o: rb_tree/red_black_tree.c
	gcc -c -Wall $^ -I rb_tree/ -U DEBUG_ASSERT
misc.o: rb_tree/misc.c
	gcc -c -Wall $^ -I rb_tree/ -U DEBUG_ASSERT
brain: hole_list.o sem.o mem.o wait_queue.o vm.o stack.o red_black_tree.o misc.o sched.o loader.o
	gcc -o $@ $^
.PHONY: all clean

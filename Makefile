.PHONY: linux clean

all:
	@echo "Usage: make {linux|clean}"
linux:
	gcc -lncurses main.c str_replace.c -o pizza
clean:
	-rm pizza

CC := gcc
CFLAGS := -Wall -Wextra -Werror

all: pfsim

pfsim.o: pfsim.c
	@echo "CC    $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Clean"
	@rm -f pfsim.o

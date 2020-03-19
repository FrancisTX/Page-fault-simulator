all: pfsim

pfsim.o: pfsim.c
	gcc -Wall -Wextra -Werror -c -o pfsim.o pfsim.c

clean:
	@echo "Clean"
	@rm -f pfsim.o

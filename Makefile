scanner:src/main.c lib/libscanner.so
	gcc -o scanner -g src/main.c -I include -lscanner -ldl
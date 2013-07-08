libscanner.so:hash.c mm.c register.c scanner.c packet.c thread_pool.c util.c
	gcc -shared -o libscanner.so hash.c mm.c register.c scanner.c packet.c thread_pool.c util.c -I ../include -lpthread
	sudo cp libscanner.so /lib/
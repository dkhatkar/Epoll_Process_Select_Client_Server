CC=gcc
# Hey!, I am comment number 2. I want to say that CFLAGS will be the
# options I'll pass to the compiler.
CFLAGS=-Wall -ggdb

all: 
	$(CC) $(CFLAGS) p_svr.c -o p_svr
	$(CC) $(CFLAGS) p_client.c -o p_client
	$(CC) $(CFLAGS) s_svr.c -o s_svr
	$(CC) $(CFLAGS) e_svr.c -o e_svr
	
clean:
	rm -rf p_svr p_client s_svr e_svr
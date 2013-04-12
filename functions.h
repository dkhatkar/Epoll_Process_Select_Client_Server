#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	80		//Buffer length
#define TRUE	1
#define LISTENQ	5
#define PIPE_BUFFER 255
#define MAXLINE 4096
#define READ 0
#define WRITE 1
 
int getPort(int* socket);
int bind_address(int *port, int *socket);
void read_pipe(int pipes[2], char *recv_buffer);
void write_pipe(int pipes[2], char *send_buffer);
void write_to_pipe_client(int pipes[2], char *send_buffer);
void read_pipe_client(int pipes[2], char *recv_buffer);
 
// This function gets the port number of the client. This port number is
// Port X in the assignment. All data transfers go from server port 7000 to
// Port X.
int getPort(int* socket)
{
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	if (getsockname(*socket, (struct sockaddr *)&sin, &len) == -1) {
                return -1;
	}
	return ntohs(sin.sin_port);
}
 
// This function binds address and port to the given socket.
// This function is used when binding port 7000 to server 
// and when the client needs to bind port X, so there may be
// data transfer between the two.
int bind_address(int *port, int *socket)
{
    struct sockaddr_in address;
 
    bzero((char *)&address, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(*port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
 
    return bind(*socket, (struct sockaddr *)&address, sizeof(address));
}

//Our spawned process sits in this function forever, reading.
void read_pipe(int pipes[2], char *recv_buffer)
{
	FILE *fp;
	
	close(pipes[WRITE]); //we can only have one descripter open at a time.
	
	while(TRUE)
	{		
		//read from the pipe through the read descriptor.
		while(read(pipes[READ], recv_buffer, PIPE_BUFFER) > 0)
		{
			fp=fopen("connections.csv", "a+b");
			fprintf(fp,"%s\n", recv_buffer);
			fclose(fp);
			bzero(recv_buffer, 0);
		}
		
	}
}
void write_to_pipes(int pipes[2], char *send_buffer)
{
	close(pipes[READ]); //we are writing, so close read.
	//send to the pipe through the write descirptor
	write(pipes[WRITE], send_buffer, PIPE_BUFFER); 

}

void read_pipe_client(int pipes[2], char *recv_buffer)
{
	FILE *fp;
	
	close(pipes[WRITE]); //we can only have one descripter open at a time.
	
	while(TRUE)
	{		
		//read from the pipe through the read descriptor.
		while(read(pipes[READ], recv_buffer, PIPE_BUFFER) > 0)
		{
			fp=fopen("client_transfer_speeds.csv", "a+b");
			fprintf(fp,"%s\n", recv_buffer);
			fclose(fp);
			bzero(recv_buffer, 0);
		}
		
	}
}
void write_to_pipe_client(int pipes[2], char *send_buffer)
{
	close(pipes[READ]); //we are writing, so close read.
	//send to the pipe through the write descirptor
	write(pipes[WRITE], send_buffer, PIPE_BUFFER); 

}
 

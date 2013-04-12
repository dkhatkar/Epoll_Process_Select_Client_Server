/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	p_svr.c - A TCP server that forks new connections
--
--	PROGRAM:		p_svr.exe
--
--	FUNCTIONS:		Berkeley Sockets API, send_file, get_file,  
					and extra functions inside extra_functions.h
--
--	DATE:			October 3, 2011
--
--	REVISIONS:		(Date and Description)
--					February 2012
--					No revisions as of yet.
--
--
--	DESIGNER:		Daniel Khatkar
--
--	PROGRAMMER:	Daniel Khatkar
--
--	NOTES:
--	The program will accept TCP connections from client machines, concurrently, on port 7001.
-- 	The program will read the data request from the client socket and process it 
--  appropriately. After the server has received a connection from the client, and the first
--  function request, another connection will be opened to connect to the clients port x. This channel
--  is for data transfer. The server sends or receives data to/from the client on port 7000.  
--  If the request from the client is GET, the server will send the requested file to the client.
--  If it is SEND, the server will receive the file specified from the client. 
---------------------------------------------------------------------------------------*/
 
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#include "functions.h"
 
int send_file(int socketdes, char *bp, char* buf);
int get_file(int socketdes, char *bp, char* rbuf);
static void SystemFatal(const char* );
// This is the main function which binds the port of server to a socket.
// After this bind is done, the server listens for up to 5 connection requests.
// As soon as a client connects, a new socket is created with the accept() command.
// The server will receive the request from the client and then forks.
// within the child process the appropriate function will be initiated. From here
// a new connection will be created to the client for data transer. This is done with
// the data_connection function.
int main (int argc, char **argv)
{
	int	pid, c, bytes_to_read, n,arg, amount, i, requests, pid_read;
	int	sd, new_sd,  port;
	int 	pipes[2];
	//int  clientPort;
	socklen_t client_len;
	struct	sockaddr_in client;
	char	*bp, buf[BUFLEN], *trunc, *ss, sendBuffer[PIPE_BUFFER], recv_buffer[PIPE_BUFFER];
 
 	opterr = 0;
 	while ((c = getopt(argc, argv, "p:")) != -1)
 	{
		switch(c)
		{
			case 'p':
				port = atoi(optarg);
				break;
			case '?':
            fprintf(stderr, "Unrecognized option: -%c\n", optopt);
            break;
			default:
				exit(1);
		}
 	}
 	
 	pipe(pipes);
 	pid_read = fork();
 	
 	if(pid_read == 0){
 		read_pipe(pipes, recv_buffer);
 		exit(0);
 	}
 
	// Create a stream socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("Can't create a socket");
		exit(1);
	}
 
 	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
   arg = 1;
   if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
   {
                SystemFatal("setsockopt");
	}	
	// Bind an address to the socket
	if(bind_address(&port, &sd) == -1){
		SystemFatal("bind error");	
	}
 
	// Listen for connections
	// queue up to 5 connect requests
	listen(sd, 5);
 
	while (TRUE)
	{
		client_len = sizeof(client);
		if ((new_sd = accept (sd, (struct sockaddr *)&client, &client_len)) == -1)
		{
			fprintf(stderr, "Can't accept client\n");
			exit(1);
		}
 		pid = fork();
 		if(pid == 0){
			printf(" Remote Address:  %s\n", inet_ntoa(client.sin_addr));
			
			strcpy (sendBuffer,"Connected, ");
			strcat (sendBuffer,  inet_ntoa(client.sin_addr));
			write_to_pipes(pipes, sendBuffer);
			
			//clientPort = ntohs(client.sin_port); check what port client came from if wanted.
 			// length of string
			bp = buf;
			bytes_to_read = BUFLEN;
			
			while ((n = recv (new_sd, bp, bytes_to_read, 0)) < BUFLEN)
			{
					bp += n;
					bytes_to_read -= n;
			}
			printf ("%s\n", buf);
			trunc = strtok (buf,":");
   		amount = atoi(trunc);
    		trunc = strtok (NULL, ":");
    		requests = atoi(trunc);

			ss = malloc(sizeof(char*) * amount);
			memset(ss, 'a', amount);
 			for(i = 0; i < requests; i++){
				send (new_sd, ss, amount, 0);
			}
			free(ss);
			close(new_sd);
			bzero(sendBuffer, 0);
			strcpy (sendBuffer,"Disconnected, ");
			strcat (sendBuffer,  inet_ntoa(client.sin_addr));
			write_to_pipes(pipes, sendBuffer);
			close(pipes[READ]);
			return 0;
		}
		else{
			//close(new_sd);	
		}
	}
	close(sd);
	return(0);
}

static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}
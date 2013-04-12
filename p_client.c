/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	client.c - A TCP client file for data transfer
--
--	PROGRAM:		client.exe
--
--	FUNCTIONS:		send_file, get_file, Berkeley Sockets API 
					and extra functions inside extra_functions.h
--
--	DATE:			
--
--	REVISIONS:		(Date and Description)
--					October 2011
--					No revisions as of yet.
--
--
--	DESIGNERS:		Daniel Khatkar 

--
--	PROGRAMMERS:	Daniel Khatkar

--
--	NOTES:
--	The program will establish a TCP connection to a user specifed server.
-- 	The server can be specified using a fully qualified domain name or and
--	IP address. After the connection has been established the user will be
-- 	prompted for a function request. This may be either 'GET' or 'SEND'. After
--  this request is handled taken in from standard input,it is sent to the server.
--  Then the client will open another connection and begin listening for server requests. 
--  This channle is opened for data transfer. If the request is GET, the client will specify a file
--  it would like to receive from the server. If it is SEND, the client will specify a file it is 
--  about to send to the server. The client will listen on port x, which is the port the request was sent
--  to the server.
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
#include <getopt.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "functions.h"
 
 
// This is the main function for the Client. The client first creates a socket
// which can connect to the server. The client will handle the users input
// to decide if it is either a GET or SEND function that needs to be requested from the
// server. After the request has been sent and a connection is made, the client will
// bind itself to another port, which is port X, and begin listening for connections from the
// server. This is done in the data_listener function. Over this channel there will be data transfer to or from
// the server.

static void SystemFatal(const char* );
int main (int argc, char **argv)
{

	int sd, port, listener_port, c, arg, n, bytes_to_read, amount, i, connections, requests, z, pid, pid_read, status, y;
	struct hostent	*hp;
	struct sockaddr_in server;
	pid_t pid_wait;
	int pipes[2];
	struct timeval tv, tv1;
	double elapsed_time, seconds, useconds;
	char  *host, *bp, **pptr, *amount2, *requests2;
	char str[16], stringtosend[BUFLEN], *buffer, sendBuffer[PIPE_BUFFER], recv_buffer[PIPE_BUFFER], elapsedtime[BUFLEN];
 
	opterr = 0;
 	while ((c = getopt(argc, argv, "p:h:l:c:r:")) != -1)
 	{
		switch(c)
		{
			case 'p':
				port = atoi(optarg);
				break;
			case 'h':
				host = optarg;
				break;
			case 'l':
				amount2 = optarg;
				break;
			case 'c':
				connections =  atoi(optarg);
				break;
			case 'r':
				requests2 =  optarg;
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
 		read_pipe_client(pipes, recv_buffer);
 		exit(0);
 	}
 	
 	for(z = 0; z < connections; z++){
 		
 		pid = fork();
 	
 		if(pid == 0){
			// Create the socket for acknowledgement
			if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			{
				perror("Cannot create socket");
				exit(1);
			}
 			// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
   		arg = 1;
   		if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1){
     	  		SystemFatal("setsockopt");
   		}
			//assign the clients destination address (servers)
			bzero((char *)&server, sizeof(struct sockaddr_in));
			server.sin_family = AF_INET;
			server.sin_port = htons(port);
 
			if ((hp = gethostbyname(host)) == NULL)
			{
				fprintf(stderr, "Unknown server address\n");
				exit(1);
			}
			bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
 
			// Connecting to the server
			if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1)
			{
				fprintf(stderr, "Can't connect to server\n");
				exit(1);
			}
 
			printf("Connected:    Server Name: %s\n", hp->h_name);
			pptr = hp->h_addr_list;
			printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));
			listener_port = getPort(&sd);

			printf("\t\tMy Port (port x): %d\n", listener_port); //get port client sent first packet from to server.
			printf("REQUEST:\n");
 	
 			requests = atoi(requests2);
 			amount = atoi(amount2);
 	
			strcpy(stringtosend, amount2);
			strcat(stringtosend, ":");
			strcat(stringtosend, requests2);
 			//send variable length
 			printf ("%s\n", stringtosend);
 			printf ("%d\n", amount);
 			gettimeofday(&tv, NULL);
 			send (sd, stringtosend, BUFLEN, 0);
			for(i = 0; i < requests; i++)
			{
				buffer = malloc(sizeof(char*) * amount);		
				printf("Receive:\n");
				bp = buffer;
				bytes_to_read = amount;

				// client makes repeated calls to recv until no more data is expected to arrive.
				//n = 0;
				while ((n = recv (sd, bp, bytes_to_read, 0)) < amount)
				{
					bp += n;
					bytes_to_read -= n;
		
				}
				printf ("%d\n", n);
				printf ("%s\n", buffer);	
				free(buffer);
 			}
 			gettimeofday(&tv1, NULL);
  	 		seconds  = tv1.tv_sec  - tv.tv_sec;
  	 		useconds = tv1.tv_usec - tv.tv_usec;
   		elapsed_time = ((seconds) * 1000 + useconds/1000.0) + 0.5;
   		strcpy (sendBuffer,requests2);
			strcat (sendBuffer,  ", ");
			snprintf(elapsedtime, BUFLEN, "%f", elapsed_time);
			strcat (sendBuffer,  elapsedtime);	
			write_to_pipe_client(pipes, sendBuffer);
			close(pipes[READ]);
			close (sd);
			exit (0);
		}		
	}
	for (y = 0 ; y < connections ; y++ )
	{
  			pid_wait = wait(&status);
 			printf("Child with PID %ld exited with status 0x%x.\n", (long)pid_wait, status);
	}	

	return 0;
	
}

static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}
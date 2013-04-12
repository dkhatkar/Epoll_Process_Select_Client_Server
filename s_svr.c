#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include "functions.h"

// Function Prototypes
static void SystemFatal(const char* );

int main (int argc, char **argv)
{
	int i, maxi, nready, bytes_to_read, arg, amount, requests, c, pid_read;
	int listen_sd, new_sd, sockfd, port, maxfd, client[FD_SETSIZE];
	struct sockaddr_in client_addr;
	int pipes[2];
	socklen_t client_len;
	char *bp, buf[BUFLEN], *trunc, *ss, sendBuffer[PIPE_BUFFER], recv_buffer[PIPE_BUFFER];;
  	ssize_t n;
   fd_set rset, allset;

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
 	
	// Create a stream socket
	if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		SystemFatal("Cannot Create Socket!");
	}
	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
   arg = 1;
   if (setsockopt (listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
   {
    	SystemFatal("setsockopt");
	}
	// Bind an address to the socket
	if(bind_address(&port, &listen_sd) == -1){
		SystemFatal("bind error");	
	}
	// Listen for connections
	// queue up to LISTENQ connect requests
	listen(listen_sd, LISTENQ);

	maxfd	= listen_sd;	// initialize
   maxi	= -1;		// index into client[] array

	for (i = 0; i < FD_SETSIZE; i++)
	{
           	client[i] = -1;             // -1 indicates available entry
	} 	
 	FD_ZERO(&allset);
   FD_SET(listen_sd, &allset);

	while (TRUE)
	{
		
		//pipe(pipes);
 		//pid_read = fork();
 	
 		//if(pid_read == 0){
 			//read_pipe(pipes, recv_buffer);
 			//exit(0);
	 	//}
	 	
   	rset = allset;               // structure assignment
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

      if (FD_ISSET(listen_sd, &rset)) // new client connection
      {
			client_len = sizeof(client_addr);
			if ((new_sd = accept(listen_sd, (struct sockaddr *) &client_addr, &client_len)) == -1)
			{
				SystemFatal("accept error");
			}	
         printf(" Remote Address:  %s\n", inet_ntoa(client_addr.sin_addr));
         /*
			bzero(sendBuffer, 0);
         strcpy (sendBuffer,"Connected, ");
			strcat (sendBuffer,  inet_ntoa(client_addr.sin_addr));
			write_to_pipes(pipes, sendBuffer);
			*/
        	for (i = 0; i < FD_SETSIZE; i++)
        	{
				if (client[i] < 0)
         	{
					client[i] = new_sd;	// save descriptor
					break;
         	}
				if (i == FD_SETSIZE)
         	{
					printf ("Too many clients\n");
            	exit(1);
    			}
			}
			FD_SET (new_sd, &allset);     // add new descriptor to set
			if (new_sd > maxfd)
			{
				maxfd = new_sd;	// for select
			}
			if (i > maxi)
			{
				maxi = i;	// new max index in client[] array
			}
			if (--nready <= 0)
			{
				continue;	// no more readable descriptors
			}
			
     	}

		for (i = 0; i <= maxi; i++)	// check all clients for data
     	{
				if ((sockfd = client[i]) < 0)
				{
					continue;
				}
				if (FD_ISSET(sockfd, &rset))
         	{
         		
         		// length of string
					bp = buf;
					bytes_to_read = BUFLEN;
			
					while ((n = read (sockfd, bp, bytes_to_read) < BUFLEN))
					//while ((n = recv (sockfd, bp, bytes_to_read, 0)) > 0)
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
 						//send (sockfd, ss, amount, 0);
						write (sockfd, ss, amount);
					}
					
					if (n == 0) // connection closed by client
            	{
            		/*
            		bzero(sendBuffer, 0);
						strcpy (sendBuffer,"Disconnected, ");
						strcat (sendBuffer,  inet_ntoa(client_addr.sin_addr));
						write_to_pipes(pipes, sendBuffer);
            		//close(pipes[READ]);
            		*/
						printf(" Remote Address:  %s closed connection\n", inet_ntoa(client_addr.sin_addr));
						FD_CLR(sockfd, &allset);
               	client[i] = -1;
            	}
									            				
					if (--nready <= 0)
					{
            		break;        // no more readable descriptors
            	}
            	free(ss);
				}
     		}
     		
   	}
	return(0);
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}
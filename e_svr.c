#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include "functions.h"

#define FALSE 		0
#define EPOLL_QUEUE_LEN	256

//Globals
int fd_server;

// Function prototypes
static void SystemFatal (const char* message);
static int ClearSocket (int fd, int pipes[2], char* address);
void close_fd (int);

int main (int argc, char* argv[]) 
{
	int i, arg, pid_read; 
	int num_fds, fd_new, epoll_fd;
	static struct epoll_event events[EPOLL_QUEUE_LEN], event;
	int port, c;
	int pipes[2];
	struct sockaddr_in addr, remote_addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	struct sigaction act;
	char sendBuffer[PIPE_BUFFER], recv_buffer[PIPE_BUFFER];
	
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
 	

	// set up the signal handler to close the server socket when CTRL-c is received
   act.sa_handler = close_fd;
   act.sa_flags = 0;
   if ((sigemptyset (&act.sa_mask) == -1 || sigaction (SIGINT, &act, NULL) == -1))
   {
        perror ("Failed to set SIGINT handler");
        exit (EXIT_FAILURE);
   }
	
	// Create the listening socket
	fd_server = socket (AF_INET, SOCK_STREAM, 0);
   if (fd_server == -1) 
   {
		SystemFatal("socket");
   }	
   // set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
  	arg = 1;
   if (setsockopt (fd_server, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) 
	{
		SystemFatal("setsockopt");
   }	
   // Make the server listening socket non-blocking
   if (fcntl (fd_server, F_SETFL, O_NONBLOCK | fcntl (fd_server, F_GETFL, 0)) == -1) 
   {
		SystemFatal("fcntl");
   }	
   // Bind to the specified listening port
   memset (&addr, 0, sizeof (struct sockaddr_in));
   addr.sin_family = AF_INET;
  	addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);
   if (bind (fd_server, (struct sockaddr*) &addr, sizeof(addr)) == -1) 
   {
		SystemFatal("bind");
   } 	
   // Listen for fd_news; SOMAXCONN is 128 by default
   if (listen (fd_server, SOMAXCONN) == -1) 
   {
		SystemFatal("listen");
   } 	
   // Create the epoll file descriptor
   epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
   if (epoll_fd == -1) 
   {
		SystemFatal("epoll_create");
   }	
   // Add the server socket to the epoll event loop
   event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
   event.data.fd = fd_server;
   if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_server, &event) == -1) 
   {
		SystemFatal("epoll_ctl");
   } 	
   pipe(pipes);
 	pid_read = fork();
 	
 	//if(pid_read == 0){
 	//	read_pipe(pipes, recv_buffer);
 		//exit(0);
 //	}
	// Execute the epoll event loop
   while (TRUE) 
	{
		//struct epoll_event events[MAX_EVENTS];
		num_fds = epoll_wait (epoll_fd, events, EPOLL_QUEUE_LEN, -1);
		if (num_fds < 0)
		{ 
			SystemFatal ("Error in epoll_wait!");
		}
		for (i = 0; i < num_fds; i++) 
		{
	    		// Case 1: Error condition
	    	if (events[i].events & (EPOLLHUP | EPOLLERR)) 
			{
				fputs("epoll: EPOLLERR", stderr);
				close(events[i].data.fd);
				continue;
	    	}
	    	assert (events[i].events & EPOLLIN);

	    	// Case 2: Server is receiving a connection request
	    	if (events[i].data.fd == fd_server) 
			{
				//socklen_t addr_size = sizeof(remote_addr);
				fd_new = accept (fd_server, (struct sockaddr*) &remote_addr, &addr_size);
				if (fd_new == -1) 
				{
		    		if (errno != EAGAIN && errno != EWOULDBLOCK) 
					{
						perror("accept");
		    		}
		    		continue;
				}

				// Make the fd_new non-blocking
				if (fcntl (fd_new, F_SETFL, O_NONBLOCK | fcntl(fd_new, F_GETFL, 0)) == -1) 
				{
					SystemFatal("fcntl");
				}
				// Add the new socket descriptor to the epoll loop
				event.data.fd = fd_new;
				if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_new, &event) == -1) 
				{
					SystemFatal ("epoll_ctl");
				}
				//printf(" Remote Address:  %s\n", inet_ntoa(remote_addr.sin_addr));
				//strcpy (sendBuffer,"Connected, ");
				//strcat (sendBuffer,  inet_ntoa(remote_addr.sin_addr));
				//write_to_pipes(pipes, sendBuffer);
				//close(pipes[READ]);				
				continue;
	    	}
			
	    	// Case 3: One of the sockets has read data
	    	if (!ClearSocket(events[i].data.fd, pipes,  inet_ntoa(remote_addr.sin_addr))) 
			{
				// epoll will remove the fd from its set
				// automatically when the fd is closed
				close (events[i].data.fd);
				//printf(" Remote Address:  %s closed connection\n", inet_ntoa(remote_addr.sin_addr));
	    	}
		}
   }
	close(fd_server);
	exit (EXIT_SUCCESS);
}

static int ClearSocket (int fd, int pipes[2], char* address) 
{
	int	n, bytes_to_read, amount, i, requests;
	char	*bp, buf[BUFLEN], *trunc;
	char sendBuffer[PIPE_BUFFER];
	
	while (TRUE)
	{
		// length of string
		bp = buf;
		bytes_to_read = BUFLEN;
			
		while ((n = recv (fd, bp, bytes_to_read, 0)) > 0)
		{
			bp += n;
			bytes_to_read -= n;
		}
		//printf ("%s\n", buf);
		trunc = strtok (buf,":");
   	amount = atoi(trunc);
    	trunc = strtok (NULL, ":");
    	requests = atoi(trunc);

		char ss[amount];
		memset(ss, 'a', amount);
		//printf ("%d\n", requests);
 		for(i = 0; i < requests; i++){
				send (fd, ss, amount, 0);
		}
		//n = 0;
		if (n == -1) // connection closed by client
     	{
     		//bzero(sendBuffer, 0);
			//strcpy (sendBuffer,"Disconnected, ");
			//strcat (sendBuffer, address);
			//write_to_pipes(pipes, sendBuffer);
         //close(pipes[READ]);
     		//printf ("%s", address);
     		//printf ("%s", " disconnected");
			close(fd);
     	}

		return TRUE;
	}
	close(fd);
	return(0);

}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message) 
{
    perror (message);
    exit (EXIT_FAILURE);
}

// close fd
void close_fd (int signo)
{
   close(fd_server);
	exit (EXIT_SUCCESS);
}
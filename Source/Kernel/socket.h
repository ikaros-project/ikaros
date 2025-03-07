//	socket.h		Socket utilities for the IKAROS project

#ifndef SOCKET
#define SOCKET

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <netdb.h>


#include <string>

#include <system_error>
#include <cstring>
#include <csignal>

#include <stddef.h>

#include "utilities.h"
#include "deprecated.h"

#define PORTNO		8000 	// the default port clients will be connecting to

//
//	Socket used to request data from a HTTP server in a simple way
//
class Socket
	{
	public:
		Socket() {};
		~Socket();
		
		std::string 	HTTPGet(const std::string & url);
		bool			Poll(); // return true if data is waiting
		void			Close();

		bool			SendRequest(const char * hostname, int port, const char * request, const long size=-1);
		int				ReadData(char * result, int maxlen, bool fill=false);
		int				Get(const char * hostname, int port, const char * request, char * result, int maxlen);
			
	private:
		int sd = -1;
	};

//
//	The class ServerSocket implements a simple HTTP server
//

class ServerSocket
	{
	public:
		ServerSocket(int port=PORTNO);		// Create server socket on port
		~ServerSocket();
		
		Dictionary			header;									// The current HTTP request header
		bool				GetRequest(bool block=false);           // Check for request and read if available
		
		bool				SendHTTPHeader(Dictionary * d = NULL, const char * response=NULL);
		bool				SendData(const char * buffer, long size);
		bool				Send(const char * format, ...); // Maximum 1023 characters
		bool				Send(std::string data);
		bool				SendFile(const char * filename, const char * path="", Dictionary * header = NULL); // FIXME: add header automatically based on file type
		bool				SendFile(const std::string & filename, const std::string & path="");
		bool				SendBuffer();

		bool				Close();								// Close reply socket
		
		int					Port();									// Get port no
		const char *		Host();									// Get host name
		
		int					errcode;								// Last error
		std::string 		body;									// Body of PUT request
	private:
		int				portno;
		int 			sockfd;						// Listen on sock_fd,
		int				new_fd;						// New connection on new_fd
		int				block_flags;				// Original flags for blocking I/O
		int             request_allocated_size = 1024;
		char *		 	request = (char *)malloc(sizeof(char)*request_allocated_size);
		char			filepath[PATH_MAX];
		
		struct sockaddr_in	my_addr; 					// My address information
		struct sockaddr_in	their_addr;					// Connector's address information
		bool				Poll(bool block=false);                 // Poll for connection; return >=0 if accepted connection (or > 0 CHECK!!!)
		size_t				Read(char * buffer, int maxSize, bool fill=false);	// Read, fill means fill with maxSize, return read size
	};

#endif


//
//    IKAROS_Socket.cc		Socket utilities for the IKAROS project
//
//    Copyright (C) 2001-2011  Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    See http://www.ikaros-project.org/ for more information.
//

#include "IKAROS_System.h"

#ifdef USE_BSD_SOCKET

#include "IKAROS_Socket.h"
#include "IKAROS_Math.h"

using namespace ikaros;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
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


#define BACKLOG		10 		// how many pending connections queue will hold
#define TIMEOUT		1000	// number of attempts to read
#define TIMEOUTDEL	1000	// number of usec to sleep before each read



//
//	SocketData
//

class SocketData
	{
	public:
		int		sd;
		
		SocketData() : sd(-1)
		{};
	};



//
//	Socket - functions that gets data from a HTTP server
//

Socket::Socket()
{
    data = new SocketData();
}



Socket::~Socket()
{
    if (data->sd != -1) close(data->sd);
    delete data;
}



bool
Socket::SendRequest(const char * hostname, int port, const char * request)
{
    data->sd = -1;
    struct sockaddr_in localAddr, servAddr;
    struct hostent *h;
	
    if ((h = gethostbyname(hostname)) == NULL)
        return false;  // unkown host
	
    servAddr.sin_family = h->h_addrtype;
    memcpy((char *) &servAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    servAddr.sin_port = htons(port);
	
    if ((data->sd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        return false; // cannot open socket
	
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(0);
	
    if (bind(data->sd, (struct sockaddr *) &localAddr, sizeof(localAddr)) < 0)
        return false; // cannot open port
	
    if (connect(data->sd, (struct sockaddr *) &servAddr, sizeof(servAddr)) <0)
        return false; // cannot connect
	
    if (write(data->sd, request, strlen(request)) <0)
        return false; // cannot send data
	
    return data->sd != -1;
}



bool
Socket::Poll()
{
    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
	
    FD_ZERO(&readfds);
    FD_SET(data->sd, &readfds);
	
    select(data->sd+1, &readfds, NULL, NULL, &tv);
	
    if (FD_ISSET(data->sd, &readfds))
        return true;
    else
        return false;
}



int
Socket::ReadData(char * result, int maxlen, bool fill)
{
    if (data->sd == -1)
        return 0;
	
    const int BUFFER_SIZE = 1024;
    static char buffer[BUFFER_SIZE];
	
    int dst=0;
    ssize_t rc = 0;
    do
    {
        ssize_t read_size = (maxlen-dst < BUFFER_SIZE ? maxlen-dst : BUFFER_SIZE);
        rc = read(data->sd, buffer, read_size);
        for (int i=0; i<rc && dst<maxlen; i++)
            result[dst++] = buffer[i];
		
    }
    while (dst < maxlen && (rc >0 || fill));
	
    return dst;
}



int
Socket::Get(const char * hostname, int port, const char * request, char * result, int maxlen)
{
    int rc = 0;
    if (SendRequest(hostname, port, request))
        rc = ReadData(result, maxlen);
    Close();
	
    return rc;
}



void
Socket::Close()
{
    if (data->sd == -1)
        return;
    close(data->sd);
    data->sd = -1;
}



//
//	ServerSocketData
//

class ServerSocketData
	{
	public:
		int				portno;
		int 			sockfd;						// Listen on sock_fd,
		int				new_fd;						// New connection on new_fd
		int				block_flags;				// Original flags for blocking I/O
		int             request_allocated_size;
		char *		 	request;
//		char			argument[10][256];
//		int				argcnt;
		
		char			filepath[PATH_MAX];
		
		struct sockaddr_in	my_addr; 					// My address information
		struct sockaddr_in	their_addr;					// Connector's address information
		
		ServerSocketData() { request_allocated_size = 1024; request = (char *)malloc(sizeof(char)*request_allocated_size); }
	};



//
//	ServerSocket
//

ServerSocket::ServerSocket(int port)
{
    data = new ServerSocketData();
	
    data->portno = port;
    int yes = 1;
//    data->argcnt = 0;
    if ((data->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw SocketException(strerror(errno), 1);
	
    data->my_addr.sin_family = AF_INET;                     // host byte order
    data->my_addr.sin_port = htons(port);					// short, network byte order
    data->my_addr.sin_addr.s_addr = htonl(INADDR_ANY);		// automatically fill with my IP
    memset(&(data->my_addr.sin_zero), '\0', 8);				// zero the rest of the struct
	
    if (setsockopt(data->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)	// To avoid (most) socket already in use errors
        throw SocketException(strerror(errno), 2);
	
    // Call to avoid SIGPIPEs on OS X
    // May not work on other platforms like Linux. In that case #ifdefs should be added around it
#ifdef MAC_OS_X
    if(setsockopt(data->sockfd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(int)) == -1)  
        throw SocketException(strerror(errno), 3);
#endif
    
    if (bind(data->sockfd, (struct sockaddr *)&(data->my_addr), sizeof(struct sockaddr)) == -1)
        throw SocketException(strerror(errno), 4);			// Socket already used
	
    if (listen(data->sockfd, BACKLOG) == -1)
        throw SocketException(strerror(errno), 5);
    
    signal(SIGPIPE, SIG_IGN);							// ignore SIGPIPE so that the program does not exit; catch errors when send() returns -1 instead
}



ServerSocket::~ServerSocket()
{
    close(data->sockfd);
    delete data;
}



bool
ServerSocket::Poll(bool block)
{
    int sin_size = sizeof(struct sockaddr_in);
    if(!block)
        fcntl(data->sockfd, F_SETFL, O_NONBLOCK);				// temporariliy make the socket non-blocking
    data->new_fd = accept(data->sockfd, (struct sockaddr *)&(data->their_addr), (socklen_t*) &sin_size);
    if(!block)
        fcntl(data->sockfd, F_SETFL, 0);						// make the socket blocking again
    return (data->new_fd != -1);
}



long
ServerSocket::Read(char * buffer, int maxSize)
{
    if (data->new_fd == -1)
        return 0;
	
    ssize_t n = 0;
    bzero(buffer, maxSize);
	
    for (int t = 0; t < TIMEOUT; t++)
    {
        if ((n = recv(data->new_fd, buffer, maxSize-1, 0)) >= 0)
            return n;
		
        usleep(TIMEOUTDEL);
    }
	
    return n;
}



// strip strips non-printing characters in the beginning and end of a string
// the tsring is modified in place and a pointer to the new beginning is returned

static char *
strip(char * s)
{
	ssize_t t = strlen(s)-1;
	while(s[t] <= ' ')
		s[t--] = '\0';
	while(*s <= ' ')
		s++;
	return s;
}

static char *
recode(char * s)	    // Replace %20 and others with a space
{
    int j = 0;
    for (int i=0; s[i]; i++)
        if (s[j] != '%')
            s[i] = s[j++];
        else
        {
            s[i] = ' ';
            j+=3;
        }
    return s;
}



bool
ServerSocket::GetRequest(bool block)
{
    if (!Poll(block))
        return false;
	
	// Loop until CRLF CRLF is found which marks the end of the HTTP request
	
	char * d = data->request;
    data->request[0] = '\0';
	int read_count = 0;
	while(!strstr(d, "\r\n\r\n")) // check border condition ***
	{
		long rr = Read(&d[read_count], data->request_allocated_size-read_count);
        if(rr == 0)         // the connections has been closed since the beginning of the request
            return false;   // this can occur when a view is changed; return and ignore the request
        read_count += rr;
        data->request[read_count] = '\0';
		if(read_count >= data->request_allocated_size-1)
			data->request = (char *)realloc(data->request, data->request_allocated_size += 1024);
	}
	
	// Parse request and header [TODO: handle that arguments in theory can span several lines accoding to the HTTP specification; although it never happens]
	
	char * p = data->request;	
	if(!p)
	{
		printf("ERROR: Empty HTTP request.");
		return false;
	}
	
	header.Clear();
    
	header.Set("Method", strsep(&p, " "));
	header.Set("URI", recode(strsep(&p, " ")));
	header.Set("HTTP-Version", strsep(&p, "\r"));
	strsep(&p, "\n");
	while(p && *p != '\r')
	{
        char * k = strip(strsep(&p, ":")); // Order is important!
        char * v = strip(strsep(&p, "\r"));
		header.Set(k, v);
		strsep(&p, "\n");
	}
	
//	if(char * x = strpbrk(header.Get("URI"), "?#")) *x = '\0';	// Remove timestamp in request TODO: keep this later
	
    return true;
}



bool
ServerSocket::SendHTTPHeader(Dictionary * d, const char * response) // Content length from where?
{
    if(!response)
		Send("HTTP/1.1 200 OK\r\n");
    else
		Send("HTTP/1.1 %s\r\n", response);

    d->Set("Server", "Ikaros/1.2");
    d->Set("Connection", "Close");
	
    time_t rawtime;
    time(&rawtime);
    char tb[32];
    strftime(tb, sizeof(tb), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime)); // RFC 1123 really uses numeric time zones rather than GMT

    d->Set("Date", tb);
    d->Set("Expires", tb);
    
    for(Dictionary::Iterator i = Dictionary::First(d); i.kv; ++i)
		Send("%s: %s\r\n", d->GetKey(i), d->Get(i));
	
    Send("\r\n");
	
    return true;
}



bool
ServerSocket::SendData(const char * buffer, long size)
{
    if (data->new_fd == -1)
    {   
//        printf("### connection closed unexpectedly\n");
        return false;	// Connection closed - ignore send
    }
    
    long total = 0;
    long bytesleft = size;
    long n = 0;

     while (total < size)
    {
        n = send(data->new_fd, buffer+total, bytesleft, MSG_NOSIGNAL);  // Write "handle SIGPIPE nostop print pass" in gdb to avoid break during debugging
//        if (n == 0) printf("No data sent - SIGPIPEING....\n");
        if (n == -1) 
        {
//            printf("### send returned -1\n");
            break;
        }
        total += n;
        bytesleft -= n;
    }
	
    if (n == -1 || bytesleft > 0) // We failed to send all data
    {
//        printf("### failed to send all data\n");
        Close();	// Close the socket and ignore further data
        return false;
    }
	
    return true;
}



bool
ServerSocket::Send(const char * format, ...)
{
    if (data->new_fd == -1)
        return false;	// Connection closed - ignore send
    
    char buffer[1024];
    va_list 	args;
    va_start(args, format);
	
    if (vsnprintf(buffer, 1024, format, args) == -1)
    {
        va_end(args);
        return false;
    }
    va_end(args);
	
    return SendData(buffer, strlen(buffer));
}



bool
ServerSocket::Send(const float * d, int sizex, int sizey)
{
    float sx = float(sizex);
    float sy = float(sizey);
	
    if (!SendData((char *)(&sx), sizeof(float)))
        return false;
	
    if (!SendData((char *)(&sy), sizeof(float)))
        return false;
	
    if (!SendData((char *)d, sizex*sizey*sizeof(float)))
        return false;
	
    return true;
}



bool
ServerSocket::SendFile(const char * filename, const char * path, Dictionary * hdr)
{
    if (filename == NULL) return false;
	
    copy_string(data->filepath, path, PATH_MAX);
    append_string(data->filepath, filename, PATH_MAX);
    FILE * f = fopen(data->filepath, "r+b");
	
    if (f == NULL) return false;
	
    fseek(f, 0, SEEK_END);
    size_t len  = ftell(f);
    fseek(f, 0, SEEK_SET);
	
    Dictionary * h = (hdr ? hdr : new Dictionary());
	
    h->Set("Connection",  "Close"); // TODO: check if socket uses persistent connections
    char length[256];
    h->Set("Content-Length", int_to_string(length, (int)len, 256));
    h->Set("Server", "Ikaros/1.2");
	
    if(strend(filename, ".html"))
		h->Set("Content-Type", "text/html");
    else if(strend(filename, ".css"))
		h->Set("Content-Type", "text/css");
    else if(strend(filename, ".js"))
		h->Set("Content-Type", "text/javascript");
    else if(strend(filename, ".jpg"))
		h->Set("Content-Type", "image/jpeg");
    else if(strend(filename, ".jpeg"))
		h->Set("Content-Type", "image/jpeg");
    else if(strend(filename, ".gif"))
		h->Set("Content-Type", "image/gif");
    else if(strend(filename, ".png"))
		h->Set("Content-Type", "image/png");
    else if(strend(filename, ".svg"))
		h->Set("Content-Type", "image/svg+xml");
    else if(strend(filename, ".xml"))
		h->Set("Content-Type", "text/xml");
    else if(strend(filename, ".ico"))
		h->Set("Content-Type", "image/ico");	// TODO: Check this
	
    SendHTTPHeader(h);
	
    char * s = new char[len];
    fread(s, len, 1, f);
    SendData(s, len);
    fclose(f);
    delete [] s;
	
    if(!hdr)
		delete h;
	
    return true;
}



bool
ServerSocket::Close()
{
    if (data->new_fd == -1) return true;	// Connection already closed
	
    if (close(data->new_fd) == -1)
    {
        errcode = errno;
        return false;
    }
    data->new_fd = -1;
	
    return true;
}



int
ServerSocket::Port()
{
    return data->portno;
}



const char *
ServerSocket::Host()
{
    static char hostname[256];
    if (gethostname(hostname, 256) == 0)	// everything ok
        return hostname;
    else
        return "unkown-host";
}

#endif

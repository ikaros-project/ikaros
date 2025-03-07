
//    socket_bsd.cc		Socket utilities for the IKAROS project

#include "socket.h"

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

#include <string>

#define BACKLOG		10 		// how many pending connections queue will hold

using namespace ikaros;


static char *
	int_to_string(char * s, int i, int n)
	{
		snprintf(s, n, "%d", i);
		return s;
	}
    
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
Socket::SendRequest(const char * hostname, int port, const char * request, const long size)
{
    // Freely after https://beej.us/guide/bgnet/html/multi/getaddrinfoman.html
    
    struct addrinfo hints, *servinfo, *p;
    int rv;

    data->sd = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, std::to_string(port).c_str(), &hints, &servinfo)) != 0)
        return false; //  gai_strerror(rv));

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((data->sd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            // perror("socket");
            continue;
        }

        if (connect(data->sd, p->ai_addr, p->ai_addrlen) == -1) {
            // perror("connect");
            close(data->sd);
            continue;
        }

        break; // if we get here, we must have connected successfully
    }

    if (p == NULL) {
        // looped off the end of the list with no connection
        // fprintf(stderr, "failed to connect\n");
        return false;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if(size == -1) // default to use size of string
    {
        if (write(data->sd, request, strlen(request)) <0)
            return false; // cannot send data
    }
    else
    {
        if (write(data->sd, request, size) <0)
            return false; // cannot send data
    }

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



std::string
Socket::HTTPGet(const std::string & url) // Very temporary implementation that only handles incomplete urls for now with only site + path
{
    long max_size = 100000;
    char buffer[100000];
    auto parts = split(url, "/", 1);
    auto site = parts.at(0);
    auto path = parts.at(1);
    std::string request = "GET /"+path+" HTTP/1.1\r\nHost: " + site+"\r\nConnection: close\r\n\r\n";
    Get(site.c_str(), 80, request.c_str(), buffer, 100000);
    auto buff = std::string(buffer);
    auto header_and_data = split(buff, "\r\n\r\n", 1);
    Close();

    return header_and_data.at(1); 
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

    //int xxx = data->sockfd;
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





#include <unistd.h>
#include <cstring>  // For bzero
#include <errno.h>
#include <algorithm> // For std::min

size_t ServerSocket::Read(char *buffer, int maxSize, bool fill) 
{
    if (data->new_fd == -1) {
        return 0; // Invalid socket
    }

    size_t total_read = 0;  // Total bytes read
    ssize_t n = 0;          // Bytes read in a single recv() call

    if (fill) {
        // Fill the buffer completely if requested
        while (total_read < maxSize) {
            n = recv(data->new_fd, buffer + total_read, maxSize - total_read, 0);

            if (n > 0) {
                total_read += n;
                //std::cout << "Total read: " << total_read << std::endl;
            } else if (n == 0) {
                // Connection closed by the client
                break;
            } else if (n == -1) {
                if (errno == EINTR) {
                    // Interrupted system call, retry
                    continue;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No data available, break out of the loop
                    break;
                } else {
                    // Fatal error
                    return -1;
                }
            }
        }
    } else {
        // Single call to recv, do not enforce filling the buffer
        n = recv(data->new_fd, buffer, maxSize, 0);

        if (n > 0) {
            total_read = n;
        } else if (n == -1) {
            if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                // Fatal error
                return -1;
            }
        }
    }

    return 
    total_read;
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

/*
static char *
recode(char * s)	    // Replace %20 and others with a space - no longer used - replaced with UriDecode
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
*/


// https://www.codeguru.com/cpp/cpp/algorithms/strings/article.php/c12759/URI-Encoding-and-Decoding.htm

const signed char HEX2DEC[256] =
{
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
    
    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    
    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    
    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};


static std::string UriDecode(const std::string & sSrc)
{
   // Note from RFC1630: "Sequences which start with a percent
   // sign but are not followed by two hexadecimal characters
   // (0-9, A-F) are reserved for future extension"
 
   const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
   const long SRC_LEN = sSrc.length();
   const unsigned char * const SRC_END = pSrc + SRC_LEN;
   // last decodable '%'
   const unsigned char * const SRC_LAST_DEC = SRC_END - 2;
 
   char * const pStart = new char[SRC_LEN];
   char * pEnd = pStart;
 
   while (pSrc < SRC_LAST_DEC)
   {
      if (*pSrc == '%')
      {
         char dec1, dec2;
         if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
            && -1 != (dec2 = HEX2DEC[*(pSrc + 2)]))
         {
            *pEnd++ = (dec1 << 4) + dec2;
            pSrc += 3;
            continue;
         }
      }
 
      *pEnd++ = *pSrc++;
   }
 
   // the last 2- chars
   while (pSrc < SRC_END)
      *pEnd++ = *pSrc++;
 
   std::string sResult(pStart, pEnd);
   delete [] pStart;
   return sResult;
}



bool
ServerSocket::GetRequest(bool block)
{
    if (!Poll(block))
        return false;
	
	// Loop until CRLF CRLF is found which marks the end of the HTTP request
	
    data->request[0] = '\0';
	int read_count = 0;
	while(!strstr(data->request, "\r\n\r\n")) // check border condition ***
	{
		long rr = Read(&(data->request[read_count]), data->request_allocated_size-read_count);
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
	header.Set("URI", UriDecode(strsep(&p, " ")).c_str());
	header.Set("HTTP-Version", strsep(&p, "\r"));
	strsep(&p, "\n");
	while(p && *p != '\r')
	{
        char * k = strip(strsep(&p, ":")); // Order is important!
        char * v = strip(strsep(&p, "\r"));
		header.Set(k, v);
		strsep(&p, "\n");
	}
    if(*p=='\r')
        p++;
    if(*p=='\n')
        p++;
	
    if (equal_strings(header.Get("Method"), "PUT"))
    {
        const char* content_length_str = header.Get("Content-Length");
        if (content_length_str)
        {
            size_t content_length = std::stoull(content_length_str);
            //std::cout << "Socket: Content length " << content_length << std::endl;

            if (content_length > 0)
            {   
                char buffer[content_length];
                size_t n = strlen(p);
                strncpy(buffer, p, sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';

                long rr = n+Read(buffer+n, content_length-n, true);
                body = std::string(buffer);
            }
        }
    }

    return true;
}



bool
ServerSocket::SendHTTPHeader(Dictionary * d, const char * response) // Content length from where?
{
    if(!response)
		Send("HTTP/1.1 200 OK\r\n");
    else
		Send("HTTP/1.1 %s\r\n", response);

    d->Set("Server", "Ikaros/3.0");
    d->Set("Connection", "Close");
	
    time_t rawtime;
    time(&rawtime);
    char tb[32];
    strftime(tb, sizeof(tb), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime)); // RFC 1123 really uses numeric time zones rather than GMT

    d->Set("Date", tb);
    d->Set("Expires", tb);
    
    for(Dictionary::Iterator i = Dictionary::First(d); i.kv; ++i)
    {
		Send("%s: %s\r\n", d->GetKey(i), d->GetString(i));
    }
	
    Send("\r\n");
	
    return true;
}



bool
ServerSocket::SendData(const char * buffer, long size)
{
    if (data->new_fd == -1)
    {   
        return false;	// Connection closed - ignore send
    }
    
    long total = 0;
    long bytesleft = size;
    long n = 0;

     while (total < size)
    {
        n = send(data->new_fd, buffer+total, bytesleft, MSG_NOSIGNAL);  // Write "handle SIGPIPE nostop print pass" in gdb to avoid break during debugging
        if (n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }
	
    if (n == -1 || bytesleft > 0) // We failed to send all data
    {
        Close();	// Close the socket and ignore further data
        return false;
    }
	
    return true;
}



bool
ServerSocket::Send(std::string data)
{
    return SendData(data.c_str(), data.size());
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

/*
void ServerSocket::fillBuffer(std::string s)
{
    buffer.append(s);
}

bool ServerSocket::SendBuffer()
{
    return SendData(buffer.c_str(),buffer.length());
}

void ServerSocket::clearBuffer() // FIXME: what is this?
{
    buffer.clear();
    if (buffer.empty())
        buffer.reserve(5000000); // 5Mb
}
*/

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
    h->Set("Server", "Ikaros/3.0");
	
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
		h->Set("Content-Type", "image/vnd.microsoft.icon");
	
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
ServerSocket::SendFile(const std::string & filename, const std::string & path)
{
    return SendFile(filename.c_str(), path.c_str());
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


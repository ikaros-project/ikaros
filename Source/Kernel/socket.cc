
//    socket.cc		Socket utilities for the IKAROS project

#include "socket.h"
#include "exceptions.h"

#include <unistd.h>
#include <cstring>  // For bzero
#include <errno.h>
#include <algorithm> // For std::min


#define BACKLOG		10 		// how many pending connections queue will hold

using namespace ikaros;


static char *
	int_to_string(char * s, int i, int n)
	{
		snprintf(s, n, "%d", i);
		return s;
	}
    
//
//	Socket - functions that gets data from a HTTP server
//

Socket::~Socket()
{
    if(sd != -1) 
        close(sd);
}



bool
Socket::SendRequest(const char * hostname, int port, const char * request, const long size)
{
    struct addrinfo hints, *servinfo, *p;
    int rv;

    sd = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    if((rv = getaddrinfo(hostname, std::to_string(port).c_str(), &hints, &servinfo)) != 0)
        return false; //  gai_strerror(rv));

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != nullptr; p = p->ai_next) {
        if((sd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            continue;
        }

        if(connect(sd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sd);
            continue;
        }

        break; // if we get here, we must have connected successfully
    }

    if(p == nullptr) 
    {
        freeaddrinfo(servinfo);
        return false;
    }
    freeaddrinfo(servinfo); // all done with this structure

    if(size == -1) // default to use size of string
    {
        if(write(sd, request, strlen(request)) <0)
            return false; // cannot send data
    }
    else
    {
        if(write(sd, request, size) <0)
            return false; // cannot send data
    }

    return sd != -1;
}



bool
Socket::Poll()
{
    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
	
    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);
	
    int result = select(sd+1, &readfds, nullptr, nullptr, &tv);
    if(result == -1)
    {
        //perror("select"); // FIXME: throw exception?
        return false;
    }
	
    if(FD_ISSET(sd, &readfds))
        return true;
    else
        return false;
}



int
Socket::ReadData(char * result, int maxlen, bool fill)
{
    if(sd == -1)
        return 0;
	
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
	
    int dst=0;
    ssize_t rc = 0;
    do
    {
        ssize_t read_size = (maxlen-dst < BUFFER_SIZE ? maxlen-dst : BUFFER_SIZE);
        rc = read(sd, buffer, read_size);

        rc = read(sd, buffer, read_size);
        if(rc == -1)
        {
            //perror("read");
            return -1;
        }
    
        if(rc == 0 && fill)
            break; // End of file

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
    if(SendRequest(hostname, port, request))
        rc = ReadData(result, maxlen);
    Close();
	
    return rc;
}



void
Socket::Close()
{
    if(sd == -1)
        return;
    close(sd);
    sd = -1;
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



//	ServerSocket
//
ServerSocket::ServerSocket(int port) 
{
    portno = port;
    int yes = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
        throw std::system_error(errno, std::system_category(), "Failed to create socket");

    // Set address properties
    my_addr.sin_family = AF_INET;                     // host byte order
    my_addr.sin_port = htons(port);                   // short, network byte order
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);       // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8);             // zero the rest of the struct

    // Set SO_REUSEADDR to prevent "socket already in use" errors
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        throw std::system_error(errno, std::system_category(), "Failed to set SO_REUSEADDR");

    // Handle SIGPIPE errors on macOS
    if(setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(int)) == -1)
        throw std::system_error(errno, std::system_category(), "Failed to set SO_NOSIGPIPE");

    // Bind the socket
    if(bind(sockfd, (struct sockaddr *)&(my_addr), sizeof(struct sockaddr)) == -1) 
        throw std::system_error(errno, std::system_category(), "Failed to bind socket");

    // Listen for incoming connections
    if(listen(sockfd, BACKLOG) == -1)
        throw std::system_error(errno, std::system_category(), "Failed to listen on socket");

    // Ignore SIGPIPE so the program does not exit unexpectedly
    signal(SIGPIPE, SIG_IGN);
}



ServerSocket::~ServerSocket()
{
    close(sockfd);
}



bool
ServerSocket::Poll(bool block)
{
    int sin_size = sizeof(struct sockaddr_in);
    if(!block)
        fcntl(sockfd, F_SETFL, O_NONBLOCK);				// temporariliy make the socket non-blocking
    new_fd = accept(sockfd, (struct sockaddr *)&(their_addr), (socklen_t*) &sin_size);
    if(!block)
        fcntl(sockfd, F_SETFL, 0);						// make the socket blocking again
    return (new_fd != -1);
}




size_t 
ServerSocket::Read(char *buffer, int maxSize, bool fill) 
{
    if(new_fd == -1) {
        return 0; // Invalid socket
    }

    size_t total_read = 0;  // Total bytes read
    ssize_t n = 0;          // Bytes read in a single recv() call

    if(fill) {
        // Fill the buffer completely if requested
        while (total_read < maxSize) {
            n = recv(new_fd, buffer + total_read, maxSize - total_read, 0);

            if(n > 0) 
            {
                total_read += n;
            } 
            else if(n == 0) 
            {
                
                break; // Connection closed by the client
            } 
            else if(n == -1) 
            {
                if(errno == EINTR) 
                {
                    
                    continue; // Interrupted system call, retry
                } 
                else if(errno == EAGAIN || errno == EWOULDBLOCK) 
                {
                    break;  // No data available, break out of the loop
                } 
                else
                     throw std::system_error(errno, std::system_category(), "recv failed");
            }
        }
    } 
    else 
    {
        n = recv(new_fd, buffer, maxSize, 0);   // Single call to recv, do not enforce filling the buffer

        if(n > 0) 
        {
            total_read = n;
        } 
        else if(n == -1) 
        {
            if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) 
            {
                // Fatal error
                throw std::system_error(errno, std::system_category(), "recv failed");
                // return -1;
            }
        }
    }

    return total_read;
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
      if(*pSrc == '%')
      {
         char dec1, dec2;
         if(-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
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
    if(!Poll(block))
        return false;
	
	// Loop until CRLF CRLF is found which marks the end of the HTTP request
	
    request[0] = '\0';
    int read_count = 0;
    while(!strstr(request, "\r\n\r\n")) // check border condition ***
    {
        long rr = Read(&(request[read_count]), request_allocated_size-read_count);
        if(rr == 0)         // the connections has been closed since the beginning of the request
            return false;   // this can occur when a view is changed; return and ignore the request
        read_count += rr;
        request[read_count] = '\0';
        if(read_count >= request_allocated_size-1) {
            char *new_request = (char *)realloc(request, request_allocated_size + 1024);
            if(new_request == nullptr) 
                throw std::system_error(errno, std::system_category(), "Failed to allocate memory");

            request = new_request;
            request_allocated_size += 1024;
        }
    }
    	
	// Parse request and header [TODO: handle that arguments in theory can span several lines accoding to the HTTP specification; although it never happens]
	
	char * p = request;	
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
	
    if(equal_strings(header.Get("Method"), "PUT"))
    {
        const char* content_length_str = header.Get("Content-Length");
        if(content_length_str)
        {
            size_t content_length = std::stoull(content_length_str);
            if(content_length > 0)
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
    if(new_fd == -1)
        return false;	// Connection closed - ignore send
    
    long total = 0;
    long bytesleft = size;
    long n = 0;

     while (total < size)
    {
        n = send(new_fd, buffer+total, bytesleft, MSG_NOSIGNAL);
        if(n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }
	
    if(n == -1 || bytesleft > 0) // We failed to send all data
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
    if(new_fd == -1)
        return false;	// Connection closed - ignore send
    
    char buffer[1024];
    va_list 	args;
    va_start(args, format);
	
    if(vsnprintf(buffer, 1024, format, args) == -1)
    {
        va_end(args);
        return false;
    }
    va_end(args);
	
    return SendData(buffer, strlen(buffer));
}



bool
ServerSocket::SendFile(const char * filename, const char * path, Dictionary * hdr)
{
    if(filename == nullptr) return false;
	
    copy_string(filepath, path, PATH_MAX);
    append_string(filepath, filename, PATH_MAX);
    FILE * f = fopen(filepath, "r+b");
	
    if(f == nullptr) return false;
	
    fseek(f, 0, SEEK_END);
    size_t len  = ftell(f);
    fseek(f, 0, SEEK_SET);
	
    Dictionary * h = (hdr ? hdr : new Dictionary());
	
    h->Set("Connection",  "Close"); // TODO: check if socket uses persistent connections

    std::string length = std::to_string((size_t)len);
    h->Set("Content-Length", length.c_str());
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
ServerSocket::SendFile(const std::filesystem::path & filename, const std::string & path)
{
    return SendFile(filename.c_str(), path.c_str());
}



bool
ServerSocket::Close()
{
    if(new_fd == -1) return true;	// Connection already closed
	
    if(close(new_fd) == -1)
    {
        errcode = errno;
        return false;
    }
    new_fd = -1;
	
    return true;
}



int
ServerSocket::Port()
{
    return portno;
}



const char *
ServerSocket::Host()
{
    static char hostname[256];
    if(gethostname(hostname, 256) == 0)	// everything ok
        return hostname;
    else
        return "unkown-host";
}


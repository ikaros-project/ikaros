//    socket.cc		Socket utilities for the IKAROS project

#include "socket.h"
#include "exceptions.h"

#include <unistd.h>
#include <errno.h>
#include <algorithm> // For std::min
#include <string_view>


#define BACKLOG		10 		// how many pending connections queue will hold

using namespace ikaros;

namespace
{
    constexpr int MAX_HTTP_HEADER_SIZE = 64 * 1024;
    constexpr size_t MAX_HTTP_BODY_SIZE = 10 * 1024 * 1024;
}

//
//	Socket - functions that gets data from a HTTP server
//

Socket::~Socket()
{
    if(sd != -1) 
        close(sd);
        sd = -1;
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
        return false;
	
    if(FD_ISSET(sd, &readfds))
        return true;
    else
        return false;
}



int
Socket::ReadData(char * result, ssize_t maxlen, bool fill)
{
    if(sd == -1)
        return 0;
	
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
	
    ssize_t dst=0;
    ssize_t rc = 0;
    do
    {
        ssize_t read_size = (maxlen-dst < BUFFER_SIZE ? maxlen-dst : BUFFER_SIZE);
        rc = read(sd, buffer, read_size);

        // rc = read(sd, buffer, read_size); // Do not read twice; this discards the first chunk.
        if(rc == -1)
            return -1;
    
        if(rc == 0 && fill)
            break; // End of file

        for (int i = 0; i < std::min(rc, maxlen - dst); i++)
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
    constexpr size_t max_response_size = 100 * 1024;
    std::vector<char> buffer(max_response_size);
    auto parts = split(url, "/", 1);
    if(parts.size() < 2 || parts.at(0).empty())
        return "";

    auto site = parts.at(0);
    auto path = parts.at(1);
    std::string request = "GET /"+path+" HTTP/1.1\r\nHost: " + site+"\r\nConnection: close\r\n\r\n";
    int bytes_read = Get(site.c_str(), 80, request.c_str(), buffer.data(), buffer.size());
    if(bytes_read <= 0)
        return "";

    auto buff = std::string(buffer.data(), bytes_read);
    auto header_and_data = split(buff, "\r\n\r\n", 1);
    Close();

    if(header_and_data.size() < 2)
        return "";

    return header_and_data.at(1); 
}


//
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
    StopListening();
    Close();
}



bool
ServerSocket::Poll(bool block)
{
    int sin_size = sizeof(struct sockaddr_in);
    if(!block)
        fcntl(sockfd, F_SETFL, O_NONBLOCK);				// temporariliy make the socket non-blocking
    new_fd = accept(sockfd, (struct sockaddr *)&(their_addr), (socklen_t*) &sin_size);
    if(new_fd != -1)
        output_buffer.clear();
    if(!block)
        fcntl(sockfd, F_SETFL, 0);						// make the socket blocking again
/*
    // --- Add this block to set a 5 second receive timeout ---
    if (new_fd != -1) {
        struct timeval timeout;
        timeout.tv_sec = 5;   // 5 seconds timeout
        timeout.tv_usec = 0;
        setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    }
*/
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
    if(s == nullptr)
        return nullptr;

    while(*s != '\0' && *s <= ' ')
        s++;

    if(*s == '\0')
        return s;

    char * t = s + strlen(s) - 1;
    while(t >= s && *t <= ' ')
        *t-- = '\0';

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

static std::string UriDecode(const std::string &sSrc)
{
    const unsigned char *pSrc = (const unsigned char *)sSrc.c_str();
    const size_t SRC_LEN = sSrc.length();
    std::string result;
    result.reserve(SRC_LEN);

    for (size_t i = 0; i < SRC_LEN; ++i)
    {
        if (pSrc[i] == '%' && i + 2 < SRC_LEN)
        {
            char dec1 = HEX2DEC[pSrc[i + 1]];
            char dec2 = HEX2DEC[pSrc[i + 2]];
            if (dec1 != -1 && dec2 != -1)
            {
                result += (dec1 << 4) + dec2;
                i += 2;
                continue;
            }
        }
        result += pSrc[i];
    }
    return result;
}


/* OLD VERSION

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
*/



bool
ServerSocket::GetRequest(bool block)
{
    if(!Poll(block))
        return false;
	
    body.clear();

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
            if(request_allocated_size >= MAX_HTTP_HEADER_SIZE)
                return false;

            int new_request_size = std::min(request_allocated_size + 1024, MAX_HTTP_HEADER_SIZE);
            char *new_request = (char *)realloc(request, new_request_size);
            if(new_request == nullptr) 
                throw std::system_error(errno, std::system_category(), "Failed to allocate memory");

            request = new_request;
            request_allocated_size = new_request_size;
        }
    }
    	
	// Parse request and header [TODO: handle that arguments in theory can span several lines accoding to the HTTP specification; although it never happens]
	
	char * p = request;	
	if(!p)
	{
		printf("ERROR: Empty HTTP request.");
		return false;
	}
	
	header = dictionary();
    char * method = strsep(&p, " ");
    char * uri = strsep(&p, " ");
    char * http_version = strsep(&p, "\r");
    if(method == nullptr || uri == nullptr || http_version == nullptr || p == nullptr)
        return false;

	header["Method"] = method;
	header["URI"] = UriDecode(uri);
	header["HTTP-Version"] = http_version;
	strsep(&p, "\n");
	while(p && *p != '\r')
	{
        char * key = strsep(&p, ":");
        char * value = strsep(&p, "\r");
        if(key == nullptr || value == nullptr)
            return false;

        char * k = strip(key);
        char * v = strip(value);
        if(k == nullptr || v == nullptr || *k == '\0')
            return false;

		header[k] = v;
		strsep(&p, "\n");
	}
    if(p == nullptr)
        return false;
    if(*p=='\r')
        p++;
    if(*p=='\n')
        p++;
	
    if(header.contains_non_null("Method") && std::string(header["Method"]) == "PUT")
    {
        if(header.contains_non_null("Content-Length")) {
            size_t content_length = 0;
            try {
                content_length = std::stoull(std::string(header["Content-Length"]));
            } catch(const std::exception &) {
                return false;
            }

            if(content_length > MAX_HTTP_BODY_SIZE)
                return false;

            if (content_length > 0) {
                std::vector<char> buffer(content_length);
                size_t buffered_size = 0;
                if(p >= request && p <= request + read_count)
                    buffered_size = std::min(content_length, static_cast<size_t>(request + read_count - p));

                std::copy_n(p, buffered_size, buffer.data());

                size_t bytes_read = buffered_size + Read(buffer.data() + buffered_size, content_length - buffered_size, true);
                body = std::string(buffer.data(), bytes_read);
            }
        }
    }
    return true;
}



bool
ServerSocket::SendHTTPHeader(dictionary & d, const char * response) // Content length from where?
{
    if(!response)
		Append("HTTP/1.1 200 OK\r\n");
    else
		Append("HTTP/1.1 %s\r\n", response);

    d["Server"] = "Ikaros/3.0";
    d["Connection"] = "Close";
	
    time_t rawtime;
    time(&rawtime);
    char tb[32];
    strftime(tb, sizeof(tb), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime)); // RFC 1123 really uses numeric time zones rather than GMT

    d["Date"] = tb;
    d["Expires"] = tb;
    
    for(const auto & [key, value] : d)
    {
		Append("%s: %s\r\n", key.c_str(), std::string(value).c_str());
    }
	
    Append("\r\n");
	
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
ServerSocket::Append(const std::string & data)
{
    if(new_fd == -1)
        return false;	// Connection closed - ignore send

    output_buffer += data;
    return true;
}



bool
ServerSocket::Append(const char * format, ...)
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
	
    output_buffer += buffer;
    return true;
}



bool
ServerSocket::Flush()
{
    if(new_fd == -1)
        return false;

    if(output_buffer.empty())
        return true;

    if(!SendData(output_buffer.c_str(), output_buffer.size()))
        return false;

    output_buffer.clear();
    return true;
}



bool
ServerSocket::Send(const std::string & data)
{
    return Append(data);
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

    return Append(buffer);
}



bool
ServerSocket::SendFile(const std::filesystem::path & filename)
{
    dictionary header;
    return SendFile(filename, header);
}



bool
ServerSocket::SendFile(const std::filesystem::path & filename, dictionary & hdr)
{
    if(filename.empty()) return false;

    std::filesystem::path resolved_filename = filename;
    FILE * f = fopen(resolved_filename.c_str(), "r+b");
	
    if(f == nullptr) return false;
	
    if(fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return false;
    }
    long file_length = ftell(f);
    if(file_length < 0)
    {
        fclose(f);
        return false;
    }
    size_t len  = static_cast<size_t>(file_length);
    if(fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return false;
    }
	
    hdr["Connection"] = "Close"; // TODO: check if socket uses persistent connections

    std::string length = std::to_string((size_t)len);
    hdr["Content-Length"] = length;
    hdr["Server"] = "Ikaros/3.0";
	
    const std::string extension = resolved_filename.extension().string();
    if(extension == ".html")
		hdr["Content-Type"] = "text/html";
    else if(extension == ".css")
		hdr["Content-Type"] = "text/css";
    else if(extension == ".js")
		hdr["Content-Type"] = "text/javascript";
    else if(extension == ".jpg")
		hdr["Content-Type"] = "image/jpeg";
    else if(extension == ".jpeg")
		hdr["Content-Type"] = "image/jpeg";
    else if(extension == ".gif")
		hdr["Content-Type"] = "image/gif";
    else if(extension == ".png")
		hdr["Content-Type"] = "image/png";
    else if(extension == ".svg")
		hdr["Content-Type"] = "image/svg+xml";
    else if(extension == ".xml")
		hdr["Content-Type"] = "text/xml";
    else if(extension == ".ico")
		hdr["Content-Type"] = "image/vnd.microsoft.icon";
	
    SendHTTPHeader(hdr);
    if(!Flush())
    {
        fclose(f);
        return false;
    }
	
    char * s = new char[len];
    fread(s, len, 1, f);
    SendData(s, len);
    fclose(f);
    delete [] s;
	
    return true;
}



bool
ServerSocket::Close()
{
    if(new_fd == -1) return true;	// Connection already closed

    if(!output_buffer.empty() && !Flush())
        return false;
	
    if(close(new_fd) == -1)
    {
        errcode = errno;
        return false;
    }
    new_fd = -1;
    output_buffer.clear();
	
    return true;
}


void
ServerSocket::StopListening()
{
    if(sockfd == -1)
        return;

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    sockfd = -1;
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
        return "unknown-host";
}

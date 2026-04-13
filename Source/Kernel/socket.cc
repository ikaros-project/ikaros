//    socket.cc		Socket utilities for the IKAROS project

#include "socket.h"
#include "exceptions.h"

#include <unistd.h>
#include <errno.h>
#include <algorithm> // For std::min
#include <string_view>
#include <sstream>
#include <vector>


#define BACKLOG		10 		// how many pending connections queue will hold

using namespace ikaros;

namespace
{
    constexpr int MAX_HTTP_HEADER_SIZE = 64 * 1024;
    constexpr size_t MAX_HTTP_BODY_SIZE = 10 * 1024 * 1024;
    constexpr auto KEEP_ALIVE_IDLE_TIMEOUT = std::chrono::seconds(10);

    std::string
    to_lower_copy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }
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
    int accepted_fd = accept(sockfd, (struct sockaddr *)&(their_addr), (socklen_t*) &sin_size);
    if(!block)
        fcntl(sockfd, F_SETFL, 0);						// make the socket blocking again
/*
    // --- Add this block to set a 5 second receive timeout ---
    if (accepted_fd != -1) {
        struct timeval timeout;
        timeout.tv_sec = 5;   // 5 seconds timeout
        timeout.tv_usec = 0;
        setsockopt(accepted_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    }
*/
    if(accepted_fd == -1)
        return false;

    connections.emplace(next_connection_id++, ConnectionState{accepted_fd, {}, {}, std::chrono::steady_clock::now()});
    return true;
}




size_t 
ServerSocket::Read(char *buffer, int maxSize, bool fill) 
{
    ConnectionState * connection = ConnectionFor(current_read_connection_id);
    if(connection == nullptr) {
        return 0; // Invalid socket
    }

    size_t total_read = 0;  // Total bytes read
    ssize_t n = 0;          // Bytes read in a single recv() call

    if(fill) {
        // Fill the buffer completely if requested
        while (total_read < maxSize) {
            n = recv(connection->fd, buffer + total_read, maxSize - total_read, 0);

            if(n > 0) 
            {
                total_read += n;
                connection->last_activity = std::chrono::steady_clock::now();
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
        n = recv(connection->fd, buffer, maxSize, 0);   // Single call to recv, do not enforce filling the buffer

        if(n > 0) 
        {
            total_read = n;
            connection->last_activity = std::chrono::steady_clock::now();
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
    QueuedRequest queued_request;
    if(!QueueRequest(block))
        return false;

    if(!PopRequest(queued_request, false))
        return false;

    ActivateRequest(queued_request);
    return true;
}



bool
ServerSocket::QueueRequest(bool block)
{
    QueuedRequest queued_request;
    while(true)
    {
        int connection_id = 0;
        if(!WaitForReadyConnection(block, connection_id))
            return false;

        current_read_connection_id = connection_id;
        bool read_ok = false;
        auto parse_start = std::chrono::steady_clock::now();
        try
        {
            read_ok = ReadCurrentRequest(queued_request);
        }
        catch(...)
        {
            current_read_connection_id = 0;
            throw;
        }
        queued_request.parse_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - parse_start).count();
        current_read_connection_id = 0;

        if(!read_ok)
        {
            if(!block)
                return false;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(pending_requests_mutex);
            pending_requests.push(std::move(queued_request));
        }
        pending_requests_cv.notify_one();

        return true;
    }
}



bool
ServerSocket::PopRequest(QueuedRequest & queued_request, bool block)
{
    std::unique_lock<std::mutex> lock(pending_requests_mutex);

    if(block)
        pending_requests_cv.wait(lock, [this] { return !pending_requests.empty(); });
    else if(pending_requests.empty())
        return false;

    queued_request = std::move(pending_requests.front());
    pending_requests.pop();
    return true;
}



void
ServerSocket::ActivateRequest(const QueuedRequest & queued_request)
{
    header = queued_request.header;
    body = queued_request.body;
    active_connection_id = queued_request.connection_id;
    active_close_after_response = queued_request.close_after_response;
}



bool
ServerSocket::FinishActiveRequest()
{
    int connection_id = active_connection_id;
    active_connection_id = 0;

    if(active_close_after_response)
        return CloseConnection(connection_id);

    return true;
}



bool
ServerSocket::ReadCurrentRequest(QueuedRequest & queued_request)
{
    body.clear();
    ConnectionState * connection = ConnectionFor(current_read_connection_id);
    if(connection == nullptr)
        return false;

    constexpr size_t read_chunk_size = 4096;
    while(connection->input_buffer.find("\r\n\r\n") == std::string::npos)
    {
        char buffer[read_chunk_size];
        long rr = Read(buffer, sizeof(buffer));
        if(rr == 0)
        {
            CloseConnection(current_read_connection_id);
            return false;
        }

        connection->input_buffer.append(buffer, rr);
        if(connection->input_buffer.size() > MAX_HTTP_HEADER_SIZE)
        {
            CloseConnection(current_read_connection_id);
            return false;
        }
    }

    size_t header_end = connection->input_buffer.find("\r\n\r\n");
    if(header_end == std::string::npos)
        return false;

    std::string header_text = connection->input_buffer.substr(0, header_end);
    std::istringstream request_stream(header_text);
    std::string request_line;
    if(!std::getline(request_stream, request_line))
        return false;
    if(!request_line.empty() && request_line.back() == '\r')
        request_line.pop_back();

    auto request_line_parts = split(request_line, " ");
    if(request_line_parts.size() < 3)
        return false;

    queued_request.header = dictionary();
    queued_request.body.clear();
    queued_request.close_after_response = true;
    queued_request.connection_id = current_read_connection_id;

    queued_request.header["method"] = request_line_parts[0];
	queued_request.header["uri"] = UriDecode(request_line_parts[1]);
	queued_request.header["http-version"] = request_line_parts[2];

    for(std::string line; std::getline(request_stream, line);)
    {
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        if(line.empty())
            continue;

        auto separator = line.find(':');
        if(separator == std::string::npos)
            return false;

        std::string key = trim(line.substr(0, separator));
        std::string value = trim(line.substr(separator + 1));
        if(key.empty())
            return false;

        queued_request.header[to_lower_copy(key)] = value;
    }

    std::string http_version_string = std::string(queued_request.header["http-version"]);
    bool close_after_response = (http_version_string != "HTTP/1.1");
    if(queued_request.header.contains_non_null("connection"))
    {
        std::string connection_value = to_lower_copy(std::string(queued_request.header["connection"]));
        if(connection_value == "close")
            close_after_response = true;
        else if(connection_value == "keep-alive")
            close_after_response = false;
    }
    queued_request.close_after_response = close_after_response;
		
    if(queued_request.header.contains_non_null("method") && std::string(queued_request.header["method"]) == "PUT")
    {
        size_t body_start = header_end + 4;
        if(queued_request.header.contains_non_null("content-length")) {
            size_t content_length = 0;
            try {
                content_length = std::stoull(std::string(queued_request.header["content-length"]));
            } catch(const std::exception &) {
                return false;
            }

            if(content_length > MAX_HTTP_BODY_SIZE)
                return false;

            while(connection->input_buffer.size() < body_start + content_length)
            {
                char buffer[read_chunk_size];
                long rr = Read(buffer, sizeof(buffer));
                if(rr == 0)
                {
                    CloseConnection(current_read_connection_id);
                    return false;
                }
                connection->input_buffer.append(buffer, rr);
                if(connection->input_buffer.size() - body_start > MAX_HTTP_BODY_SIZE)
                {
                    CloseConnection(current_read_connection_id);
                    return false;
                }
            }

            if(content_length > 0)
                queued_request.body = connection->input_buffer.substr(body_start, content_length);
        }
    }

    size_t total_consumed = header_end + 4 + queued_request.body.size();
    connection->input_buffer.erase(0, total_consumed);
    return true;
}



bool
ServerSocket::SendHTTPHeader(dictionary & d, const char * response) // Content length from where?
{
    bool can_keep_alive = active_close_after_response == false &&
        (d.contains_non_null("Content-Length") || d.contains_non_null("Transfer-Encoding"));

    if(!can_keep_alive)
        active_close_after_response = true;

    if(!response)
			Append("HTTP/1.1 200 OK\r\n");
    else
			Append("HTTP/1.1 %s\r\n", response);

    d["Server"] = "Ikaros/3.0";
    d["Connection"] = active_close_after_response ? "Close" : "keep-alive";
	
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
    ConnectionState * connection = ConnectionFor(active_connection_id);
    if(connection == nullptr)
        return false;	// Connection closed - ignore send
    
    long total = 0;
    long bytesleft = size;
    long n = 0;

     while (total < size)
    {
        n = send(connection->fd, buffer+total, bytesleft, MSG_NOSIGNAL);
        if(n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }
	
    if(n == -1 || bytesleft > 0) // We failed to send all data
    {
        CloseConnection(active_connection_id);	// Close the socket and ignore further data
        return false;
    }

    connection->last_activity = std::chrono::steady_clock::now();
	
    return true;
}



bool
ServerSocket::Append(const std::string & data)
{
    ConnectionState * connection = ConnectionFor(active_connection_id);
    if(connection == nullptr)
        return false;	// Connection closed - ignore send

    connection->output_buffer += data;
    return true;
}



bool
ServerSocket::Append(const char * format, ...)
{
    if(ConnectionFor(active_connection_id) == nullptr)
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
	
    ConnectionFor(active_connection_id)->output_buffer += buffer;
    return true;
}



bool
ServerSocket::Flush()
{
    ConnectionState * connection = ConnectionFor(active_connection_id);
    if(connection == nullptr)
        return false;

    if(connection->output_buffer.empty())
        return true;

    if(!SendData(connection->output_buffer.c_str(), connection->output_buffer.size()))
        return false;

    connection->output_buffer.clear();
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
    if(ConnectionFor(active_connection_id) == nullptr)
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
	
    hdr["Connection"] = active_close_after_response ? "Close" : "keep-alive";

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
    if(active_connection_id != 0)
    {
        if(ConnectionFor(active_connection_id) != nullptr &&
           !ConnectionFor(active_connection_id)->output_buffer.empty() && !Flush())
            return false;
        return CloseConnection(active_connection_id);
    }
    if(current_read_connection_id != 0)
        return CloseConnection(current_read_connection_id);
    return true;	// Connection already closed
}


void
ServerSocket::StopListening()
{
    for(auto & [connection_id, connection] : connections)
    {
        if(connection.fd != -1)
            close(connection.fd);
    }
    connections.clear();

    if(sockfd == -1)
        return;

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    sockfd = -1;
}



bool
ServerSocket::WaitForReadyConnection(bool block, int & connection_id)
{
    connection_id = 0;

    while(true)
    {
        CloseIdleConnections();

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        int max_fd = sockfd;

        for(const auto & [id, connection] : connections)
        {
            if(connection.fd != -1)
            {
                FD_SET(connection.fd, &readfds);
                max_fd = std::max(max_fd, connection.fd);
            }
        }

        struct timeval timeout;
        struct timeval * timeout_ptr = nullptr;
        if(!block)
        {
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            timeout_ptr = &timeout;
        }

        int ready = select(max_fd + 1, &readfds, nullptr, nullptr, timeout_ptr);
        if(ready == -1)
        {
            if(errno == EINTR)
                continue;
            throw std::system_error(errno, std::system_category(), "select failed");
        }
        if(ready == 0)
            return false;

        if(FD_ISSET(sockfd, &readfds))
        {
            Poll(false);
            if(--ready <= 0)
            {
                if(!block)
                    return false;
                continue;
            }
        }

        for(const auto & [id, connection] : connections)
        {
            if(connection.fd != -1 && FD_ISSET(connection.fd, &readfds))
            {
                connection_id = id;
                return true;
            }
        }
    }
}



void
ServerSocket::CloseIdleConnections()
{
    auto now = std::chrono::steady_clock::now();
    std::vector<int> idle_connections;
    for(const auto & [id, connection] : connections)
    {
        if(connection.fd == -1)
            continue;
        if(active_connection_id == id || current_read_connection_id == id)
            continue;
        if(now - connection.last_activity > KEEP_ALIVE_IDLE_TIMEOUT)
            idle_connections.push_back(id);
    }

    for(int connection_id : idle_connections)
        CloseConnection(connection_id);
}



bool
ServerSocket::CloseConnection(int connection_id)
{
    auto it = connections.find(connection_id);
    if(it == connections.end())
        return true;

    if(close(it->second.fd) == -1)
    {
        errcode = errno;
        return false;
    }

    connections.erase(it);
    if(active_connection_id == connection_id)
        active_connection_id = 0;
    if(current_read_connection_id == connection_id)
        current_read_connection_id = 0;
    return true;
}



ServerSocket::ConnectionState *
ServerSocket::ConnectionFor(int connection_id)
{
    auto it = connections.find(connection_id);
    if(it == connections.end())
        return nullptr;
    return &it->second;
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

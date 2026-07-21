//    socket.cc		Socket utilities for the IKAROS project

#include "socket.h"
#include "exceptions.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm> // For std::min
#include <array>
#include <charconv>
#include <future>
#include <memory>
#include <optional>
#include <string_view>
#include <sstream>
#include <thread>
#include <vector>


#define BACKLOG		10 		// how many pending connections queue will hold

using namespace ikaros;

namespace
{
    constexpr int MAX_HTTP_HEADER_SIZE = 64 * 1024;
    constexpr size_t MAX_HTTP_BODY_SIZE = 10 * 1024 * 1024;
    constexpr auto KEEP_ALIVE_IDLE_TIMEOUT = std::chrono::seconds(10);

    struct AddressInfoDeleter
    {
        void operator()(addrinfo * addresses) const
        {
            if(addresses != nullptr)
                freeaddrinfo(addresses);
        }
    };

    using AddressInfoPtr = std::unique_ptr<addrinfo, AddressInfoDeleter>;

    struct ResolutionResult
    {
        int status;
        AddressInfoPtr addresses;
    };

    std::optional<ResolutionResult>
    resolve_with_timeout(const std::string & hostname, int port,
                         std::chrono::milliseconds timeout)
    {
        std::packaged_task<ResolutionResult()> resolver([hostname, port]()
        {
            addrinfo hints{};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            addrinfo * addresses = nullptr;
            int status = getaddrinfo(hostname.c_str(), std::to_string(port).c_str(),
                                     &hints, &addresses);
            return ResolutionResult{status, AddressInfoPtr(addresses)};
        });
        std::future<ResolutionResult> result = resolver.get_future();

        try
        {
            std::thread(std::move(resolver)).detach();
        }
        catch(const std::system_error &)
        {
            return std::nullopt;
        }

        if(result.wait_for(timeout) != std::future_status::ready)
            return std::nullopt;

        try
        {
            return result.get();
        }
        catch(...)
        {
            return std::nullopt;
        }
    }

    bool
    wait_for_socket(int fd, bool read_ready,
                    std::chrono::steady_clock::time_point deadline)
    {
        if(fd < 0 || fd >= FD_SETSIZE)
            return false;

        while(true)
        {
            auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
                deadline - std::chrono::steady_clock::now());
            if(remaining <= std::chrono::microseconds::zero())
                return false;

            timeval timeout{};
            timeout.tv_sec = static_cast<time_t>(remaining.count() / 1000000);
            timeout.tv_usec = static_cast<suseconds_t>(remaining.count() % 1000000);

            fd_set read_fds;
            fd_set write_fds;
            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);
            if(read_ready)
                FD_SET(fd, &read_fds);
            else
                FD_SET(fd, &write_fds);

            int result = select(fd + 1,
                                read_ready ? &read_fds : nullptr,
                                read_ready ? nullptr : &write_fds,
                                nullptr, &timeout);
            if(result > 0)
                return true;
            if(result == 0)
                return false;
            if(errno != EINTR)
                return false;
        }
    }

    bool
    set_socket_timeout(int fd, int option, std::chrono::milliseconds timeout)
    {
        timeval value{};
        value.tv_sec = static_cast<time_t>(timeout.count() / 1000);
        value.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);
        return setsockopt(fd, SOL_SOCKET, option, &value, sizeof(value)) == 0;
    }

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

Socket::Socket(std::chrono::milliseconds timeout): timeout_(timeout)
{
    if(timeout_ <= std::chrono::milliseconds::zero())
        throw std::invalid_argument("Socket timeout must be positive");
}



Socket::~Socket()
{
    Close();
}



bool
Socket::SendRequest(const char * hostname, int port, const char * request, const long size)
{
    Close();
    if(hostname == nullptr || *hostname == '\0' || request == nullptr ||
       port < 1 || port > 65535 || size < -1)
        return false;

    size_t request_size = size == -1 ? strlen(request) : static_cast<size_t>(size);

    auto resolution = resolve_with_timeout(hostname, port, timeout_);
    if(!resolution || resolution->status != 0 || !resolution->addresses)
        return false;

    // loop through all the results and connect to the first we can
    addrinfo * connected_address = nullptr;
    for(addrinfo * address = resolution->addresses.get(); address != nullptr;
        address = address->ai_next)
    {
        sd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if(sd == -1)
            continue;

        int flags = fcntl(sd, F_GETFL, 0);
        if(flags == -1 || fcntl(sd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            close(sd);
            sd = -1;
            continue;
        }

        int result = connect(sd, address->ai_addr, address->ai_addrlen);
        if(result == -1 && errno == EINPROGRESS)
        {
            auto deadline = std::chrono::steady_clock::now() + timeout_;
            if(wait_for_socket(sd, false, deadline))
            {
                int connect_error = 0;
                socklen_t connect_error_size = sizeof(connect_error);
                if(getsockopt(sd, SOL_SOCKET, SO_ERROR, &connect_error,
                              &connect_error_size) == 0 && connect_error == 0)
                    result = 0;
            }
        }

        if(result == 0 && fcntl(sd, F_SETFL, flags) == 0 &&
           set_socket_timeout(sd, SO_RCVTIMEO, timeout_) &&
           set_socket_timeout(sd, SO_SNDTIMEO, timeout_))
        {
            connected_address = address;
            break;
        }

        close(sd);
        sd = -1;
    }

    if(connected_address == nullptr)
        return false;

    auto write_deadline = std::chrono::steady_clock::now() + timeout_;
    size_t total_sent = 0;
    while(total_sent < request_size)
    {
        if(!wait_for_socket(sd, false, write_deadline))
        {
            Close();
            return false;
        }

        ssize_t sent = send(sd, request + total_sent, request_size - total_sent,
                            MSG_NOSIGNAL);
        if(sent > 0)
        {
            total_sent += static_cast<size_t>(sent);
            continue;
        }
        if(sent == -1 && errno == EINTR)
            continue;
        if(sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            continue;

        Close();
        return false;
    }

    return true;
}



bool
Socket::Poll()
{
    if(sd == -1)
        return false;

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
    if(sd == -1 || maxlen < 0 || (result == nullptr && maxlen > 0))
        return -1;
    if(maxlen == 0)
        return 0;
	
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
	
    ssize_t dst=0;
    ssize_t rc = 0;
    auto deadline = std::chrono::steady_clock::now() + timeout_;
    do
    {
        if(!wait_for_socket(sd, true, deadline))
            return -1;

        ssize_t read_size = (maxlen-dst < BUFFER_SIZE ? maxlen-dst : BUFFER_SIZE);
        rc = read(sd, buffer, read_size);

        // rc = read(sd, buffer, read_size); // Do not read twice; this discards the first chunk.
        if(rc == -1)
        {
            if(errno == EINTR)
                continue;
            return -1;
        }
    
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
ServerSocket::ServerSocket(int port, const std::string & bind_address) 
{
    portno = port;
    int yes = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
        throw std::system_error(errno, std::system_category(), "Failed to create socket");
    if(sockfd >= FD_SETSIZE)
    {
        close(sockfd);
        sockfd = -1;
        throw std::runtime_error("Server listener descriptor exceeds select() capacity");
    }

    // Set address properties
    my_addr.sin_family = AF_INET;                     // host byte order
    my_addr.sin_port = htons(port);                   // short, network byte order
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);      // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8);             // zero the rest of the struct

    if(!bind_address.empty())
    {
        if(inet_pton(AF_INET, bind_address.c_str(), &(my_addr.sin_addr)) != 1)
            throw std::invalid_argument("Invalid IPv4 bind address: " + bind_address);
    }

    // Set SO_REUSEADDR to prevent "socket already in use" errors
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        throw std::system_error(errno, std::system_category(), "Failed to set SO_REUSEADDR");

#if defined(SO_NOSIGPIPE)
    if(setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(int)) == -1)
        throw std::system_error(errno, std::system_category(), "Failed to set SO_NOSIGPIPE");
#endif

    // Bind the socket
    if(bind(sockfd, (struct sockaddr *)&(my_addr), sizeof(struct sockaddr)) == -1) 
        throw std::system_error(errno, std::system_category(), "Failed to bind socket");

    // Listen for incoming connections
    if(listen(sockfd, BACKLOG) == -1)
        throw std::system_error(errno, std::system_category(), "Failed to listen on socket");

    int wakeup_pipe[2];
    if(pipe(wakeup_pipe) == -1)
    {
        int error = errno;
        close(sockfd);
        sockfd = -1;
        throw std::system_error(error, std::system_category(), "Failed to create server wake-up pipe");
    }
    wakeup_read_fd = wakeup_pipe[0];
    wakeup_write_fd = wakeup_pipe[1];
    if(wakeup_read_fd >= FD_SETSIZE || wakeup_write_fd >= FD_SETSIZE)
    {
        close(wakeup_read_fd);
        close(wakeup_write_fd);
        close(sockfd);
        wakeup_read_fd = -1;
        wakeup_write_fd = -1;
        sockfd = -1;
        throw std::runtime_error("Server wake-up descriptor exceeds select() capacity");
    }

    // Ignore SIGPIPE so the program does not exit unexpectedly
    signal(SIGPIPE, SIG_IGN);
}



ServerSocket::~ServerSocket()
{
    StopListening();
    CloseSockets();
}



bool
ServerSocket::Poll(bool block)
{
    if(stop_requested.load(std::memory_order_acquire))
        return false;

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
    if(accepted_fd >= FD_SETSIZE)
    {
        close(accepted_fd);
        return false;
    }

    int accepted_flags = fcntl(accepted_fd, F_GETFL, 0);
    if(accepted_flags == -1 || fcntl(accepted_fd, F_SETFL, accepted_flags | O_NONBLOCK) == -1)
    {
        int error = errno;
        close(accepted_fd);
        throw std::system_error(error, std::system_category(), "Failed to make accepted socket nonblocking");
    }

    connections.emplace(next_connection_id++,
                        ConnectionState{accepted_fd, {}, {}, std::chrono::steady_clock::now(), false});
    return true;
}




ssize_t
ServerSocket::Read(char * buffer, size_t max_size)
{
    ConnectionState * connection = ConnectionFor(current_read_connection_id);
    if(connection == nullptr)
        return 0;

    while(true)
    {
        ssize_t bytes_read = recv(connection->fd, buffer, max_size, 0);

        if(bytes_read > 0)
        {
            connection->last_activity = std::chrono::steady_clock::now();
            return bytes_read;
        }

        if(bytes_read == 0)
            return 0;

        if(errno == EINTR)
            continue;
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;

        throw std::system_error(errno, std::system_category(), "recv failed");
    }
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
        RequestReadResult read_result = RequestReadResult::incomplete;
        auto parse_start = std::chrono::steady_clock::now();
        try
        {
            read_result = ReadCurrentRequest(queued_request);
        }
        catch(...)
        {
            current_read_connection_id = 0;
            throw;
        }
        queued_request.parse_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - parse_start).count();
        current_read_connection_id = 0;

        if(read_result != RequestReadResult::complete)
        {
            if(read_result == RequestReadResult::closed)
                CloseConnection(connection_id);
            else if(read_result != RequestReadResult::incomplete)
            {
                SendRequestError(connection_id, read_result);
                CloseConnection(connection_id);
            }
            else if(ConnectionState * connection = ConnectionFor(connection_id))
                connection->awaiting_more_input = true;
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



ServerSocket::RequestReadResult
ServerSocket::ReadCurrentRequest(QueuedRequest & queued_request)
{
    body.clear();
    ConnectionState * connection = ConnectionFor(current_read_connection_id);
    if(connection == nullptr)
        return RequestReadResult::closed;

    constexpr size_t read_chunk_size = 4096;
    while(connection->input_buffer.find("\r\n\r\n") == std::string::npos)
    {
        char buffer[read_chunk_size];
        ssize_t rr = Read(buffer, sizeof(buffer));
        if(rr == 0)
            return RequestReadResult::closed;
        if(rr < 0)
            return RequestReadResult::incomplete;

        connection->input_buffer.append(buffer, rr);
        if(connection->input_buffer.size() > MAX_HTTP_HEADER_SIZE)
            return RequestReadResult::request_header_too_large;
    }

    size_t header_end = connection->input_buffer.find("\r\n\r\n");
    if(header_end == std::string::npos)
        return RequestReadResult::incomplete;

    std::string header_text = connection->input_buffer.substr(0, header_end);
    std::istringstream request_stream(header_text);
    std::string request_line;
    if(!std::getline(request_stream, request_line))
        return RequestReadResult::bad_request;
    if(!request_line.empty() && request_line.back() == '\r')
        request_line.pop_back();

    std::istringstream request_line_stream(request_line);
    std::string method;
    std::string uri;
    std::string http_version;
    std::string extra_request_line_part;
    if(!(request_line_stream >> method >> uri >> http_version) ||
       request_line_stream >> extra_request_line_part)
        return RequestReadResult::bad_request;

    if(method != "GET" && method != "PUT")
        return RequestReadResult::method_not_allowed;
    if(http_version != "HTTP/1.1" && http_version != "HTTP/1.0")
        return RequestReadResult::http_version_unsupported;

    queued_request.header = dictionary();
    queued_request.body.clear();
    queued_request.close_after_response = true;
    queued_request.connection_id = current_read_connection_id;

    queued_request.header["method"] = method;
		queued_request.header["uri"] = uri;
		queued_request.header["http-version"] = http_version;

    std::optional<size_t> content_length;
    bool has_transfer_encoding = false;

    for(std::string line; std::getline(request_stream, line);)
    {
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        if(line.empty())
            continue;

        auto separator = line.find(':');
        if(separator == std::string::npos)
            return RequestReadResult::bad_request;

        std::string key = trim(line.substr(0, separator));
        std::string value = trim(line.substr(separator + 1));
        if(key.empty())
            return RequestReadResult::bad_request;

        std::string normalized_key = to_lower_copy(key);
        if(normalized_key == "content-length")
        {
            if(content_length)
                return RequestReadResult::bad_request;

            size_t parsed_length = 0;
            auto [end, error] = std::from_chars(value.data(), value.data() + value.size(),
                                                parsed_length);
            if(error != std::errc() || end != value.data() + value.size())
                return RequestReadResult::bad_request;
            if(parsed_length > MAX_HTTP_BODY_SIZE)
                return RequestReadResult::payload_too_large;
            content_length = parsed_length;
        }
        else if(normalized_key == "transfer-encoding" && !value.empty())
        {
            has_transfer_encoding = true;
        }

        queued_request.header[normalized_key] = value;
    }

    if(has_transfer_encoding)
        return RequestReadResult::transfer_encoding_unsupported;

    if(http_version == "HTTP/1.1" &&
       !queued_request.header.contains_non_null("host"))
        return RequestReadResult::bad_request;

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
    if(method == "PUT" && !content_length)
        return RequestReadResult::length_required;

    size_t body_start = header_end + 4;
    size_t expected_body_size = content_length.value_or(0);
    while(connection->input_buffer.size() < body_start + expected_body_size)
    {
        char buffer[read_chunk_size];
        ssize_t rr = Read(buffer, sizeof(buffer));
        if(rr == 0)
            return RequestReadResult::closed;
        if(rr < 0)
            return RequestReadResult::incomplete;
        connection->input_buffer.append(buffer, rr);
        if(connection->input_buffer.size() - body_start > MAX_HTTP_BODY_SIZE)
            return RequestReadResult::payload_too_large;
    }

    if(expected_body_size > 0)
        queued_request.body = connection->input_buffer.substr(body_start, expected_body_size);

    size_t total_consumed = body_start + expected_body_size;
    connection->input_buffer.erase(0, total_consumed);
    return RequestReadResult::complete;
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
        if(n > 0)
        {
            total += n;
            bytesleft -= n;
            continue;
        }

        if(n == -1 && errno == EINTR)
            continue;

        if(n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(connection->fd, &write_fds);
            if(select(connection->fd + 1, nullptr, &write_fds, nullptr, nullptr) > 0)
                continue;
        }

        break;
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
    std::unique_ptr<FILE, decltype(&fclose)> file(
        fopen(resolved_filename.c_str(), "rb"), &fclose);
    if(!file)
        return false;

    struct stat file_info{};
    if(fstat(fileno(file.get()), &file_info) == -1 || file_info.st_size < 0 ||
       !S_ISREG(file_info.st_mode))
        return false;

    uintmax_t file_length = static_cast<uintmax_t>(file_info.st_size);

    hdr["Connection"] = active_close_after_response ? "Close" : "keep-alive";

    hdr["Content-Length"] = std::to_string(file_length);
    hdr["Server"] = "Ikaros/3.0";
    hdr["Cache-Control"] = "no-cache, no-store";
    hdr["Pragma"] = "no-cache";
	
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

    if(!SendHTTPHeader(hdr))
        return false;
    if(!Flush())
        return false;

    std::array<char, 64 * 1024> buffer;
    uintmax_t bytes_remaining = file_length;
    while(bytes_remaining > 0)
    {
        size_t requested = static_cast<size_t>(
            std::min<uintmax_t>(buffer.size(), bytes_remaining));
        size_t bytes_read = fread(buffer.data(), 1, requested, file.get());
        if(bytes_read == 0 || !SendData(buffer.data(), static_cast<long>(bytes_read)))
        {
            CloseConnection(active_connection_id);
            return false;
        }

        bytes_remaining -= bytes_read;
        if(bytes_read != requested)
        {
            CloseConnection(active_connection_id);
            return false;
        }
    }

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
    if(stop_requested.exchange(true, std::memory_order_acq_rel))
        return;

    if(wakeup_write_fd == -1)
        return;

    const char wakeup = 1;
    while(write(wakeup_write_fd, &wakeup, sizeof(wakeup)) == -1 && errno == EINTR)
        ;
}



bool
ServerSocket::WaitForReadyConnection(bool block, int & connection_id)
{
    connection_id = 0;

    while(true)
    {
        if(stop_requested.load(std::memory_order_acquire))
            return false;

        CloseIdleConnections();

        for(const auto & [id, connection] : connections)
        {
            if(!connection.awaiting_more_input &&
               connection.input_buffer.find("\r\n\r\n") != std::string::npos)
            {
                connection_id = id;
                return true;
            }
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        int max_fd = sockfd;

        if(wakeup_read_fd != -1)
        {
            FD_SET(wakeup_read_fd, &readfds);
            max_fd = std::max(max_fd, wakeup_read_fd);
        }

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
        else if(!connections.empty())
        {
            auto now = std::chrono::steady_clock::now();
            auto next_timeout = std::chrono::duration_cast<std::chrono::microseconds>(
                KEEP_ALIVE_IDLE_TIMEOUT);
            for(const auto & [id, connection] : connections)
            {
                if(connection.fd == -1 || active_connection_id == id ||
                   current_read_connection_id == id)
                    continue;

                auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
                    connection.last_activity + KEEP_ALIVE_IDLE_TIMEOUT - now);
                if(remaining < next_timeout)
                    next_timeout = remaining;
            }

            auto timeout_us = std::max<std::chrono::microseconds>(
                std::chrono::microseconds(1),
                std::chrono::duration_cast<std::chrono::microseconds>(next_timeout));
            timeout.tv_sec = static_cast<time_t>(timeout_us.count() / 1000000);
            timeout.tv_usec = static_cast<suseconds_t>(timeout_us.count() % 1000000);
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

        if(wakeup_read_fd != -1 && FD_ISSET(wakeup_read_fd, &readfds))
        {
            char wakeup;
            while(read(wakeup_read_fd, &wakeup, sizeof(wakeup)) == -1 && errno == EINTR)
                ;
            return false;
        }

        if(stop_requested.load(std::memory_order_acquire))
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

        for(auto & [id, connection] : connections)
        {
            if(connection.fd != -1 && FD_ISSET(connection.fd, &readfds))
            {
                connection.awaiting_more_input = false;
                connection_id = id;
                return true;
            }
        }
    }
}



void
ServerSocket::CloseSockets()
{
    for(auto & [connection_id, connection] : connections)
    {
        if(connection.fd != -1)
            close(connection.fd);
    }
    connections.clear();

    if(sockfd != -1)
    {
        close(sockfd);
        sockfd = -1;
    }

    if(wakeup_read_fd != -1)
    {
        close(wakeup_read_fd);
        wakeup_read_fd = -1;
    }

    if(wakeup_write_fd != -1)
    {
        close(wakeup_write_fd);
        wakeup_write_fd = -1;
    }
}



void
ServerSocket::SendRequestError(int connection_id, RequestReadResult error)
{
    const char * status = "400 Bad Request";
    switch(error)
    {
        case RequestReadResult::method_not_allowed:
            status = "405 Method Not Allowed";
            break;
        case RequestReadResult::length_required:
            status = "411 Length Required";
            break;
        case RequestReadResult::payload_too_large:
            status = "413 Payload Too Large";
            break;
        case RequestReadResult::request_header_too_large:
            status = "431 Request Header Fields Too Large";
            break;
        case RequestReadResult::transfer_encoding_unsupported:
            status = "501 Not Implemented";
            break;
        case RequestReadResult::http_version_unsupported:
            status = "505 HTTP Version Not Supported";
            break;
        default:
            break;
    }

    active_connection_id = connection_id;
    active_close_after_response = true;
    std::string response_body = std::string(status) + "\n";
    dictionary response_header({
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(response_body.size())},
    });
    SendHTTPHeader(response_header, status);
    Append(response_body);
    Flush();
    active_connection_id = 0;
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

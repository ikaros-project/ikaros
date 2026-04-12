/ Experimental server

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <chrono>

using namespace std::chrono_literals;



class Request
{
public:
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    std::chrono::steady_clock::time_point created_at;

    void 
    reply(const std::string& data, const std::string& content_type = "text/plain")
    {
        send_response(200, data, content_type);
    }

    void 
    error(int status, const std::string& message = "") 
    {
        std::string body = message.empty() ? default_error_message(status) : message;
        send_response(status, body, "text/plain");
    }

    bool
    is_timed_out(std::chrono::steady_clock::duration timeout) const 
    {
        return (std::chrono::steady_clock::now() - created_at) > timeout;
    }

//private:
    int client_fd = -1;
    bool responded = false;
    bool keep_alive = false;

    std::string 
    default_error_message(int status) 
    {
        switch (status) 
        {
            case 400: return "Bad Request";
            case 408: return "Request Timeout";
            case 413: return "Payload Too Large";
            case 500: return "Internal Server Error";
            default:  return "Error " + std::to_string(status);
        }
    }

    void 
    send_response(int status, const std::string& data, const std::string& content_type) 
    {
        if (responded || client_fd < 0) 
            return;
        responded = true;

        std::string status_text = (status == 200) ? "OK" :
                                  (status == 400) ? "Bad Request" :
                                  (status == 408) ? "Request Timeout" :
                                  (status == 413) ? "Payload Too Large" :
                                  (status == 500) ? "Internal Server Error" : "Unknown";

        std::string response =
            "HTTP/1.1 " + std::to_string(status) + " " + status_text + "\r\n"
            "Content-Type: " + content_type + "\r\n"
            "Content-Length: " + std::to_string(data.size()) + "\r\n"
            "Connection: " + (keep_alive ? "keep-alive" : "close") + "\r\n"
            "\r\n" + data;

        send(client_fd, response.c_str(), response.size(), 0);

        if (!keep_alive) 
        {
            close(client_fd);
            client_fd = -1;
        }
    }

    friend class Server;
};



class Server 
{
public:
    Server(int port,
           std::chrono::seconds header_timeout = 15s,
           std::chrono::seconds total_timeout = 60s,
           std::chrono::seconds keep_alive_timeout = 30s)
        : running(true),
          header_timeout_(header_timeout),
          total_timeout_(total_timeout),
          keep_alive_timeout_(keep_alive_timeout)
    {
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));

        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
            throw std::runtime_error(std::string("bind failed: ") + std::strerror(errno));

        if (listen(listen_fd, SOMAXCONN) < 0)
            throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));

        acceptor_thread = std::thread(&Server::acceptor, this);
        watchdog_thread = std::thread(&Server::watchdog, this);
    }

    ~Server() 
    {
        running = false;
        if (listen_fd >= 0) close(listen_fd);
        if (acceptor_thread.joinable()) acceptor_thread.join();
        if (watchdog_thread.joinable()) watchdog_thread.join();
    }

    std::shared_ptr<Request> get_next_request() 
    {
        std::unique_lock<std::mutex> lk(queue_mutex);
        queue_cv.wait(lk, [this] { return !request_queue.empty(); });
        auto req = std::move(request_queue.front());
        request_queue.pop();
        return req;
    }

    bool 
    has_pending_requests() const 
    {
        std::lock_guard<std::mutex> lk(queue_mutex);
        return !request_queue.empty();
    }

    size_t 
    pending_request_count() const 
    {
        std::lock_guard<std::mutex> lk(queue_mutex);
        return request_queue.size();
    }

private:
    int listen_fd = -1;
    std::thread acceptor_thread;
    std::thread watchdog_thread;
    std::atomic<bool> running{true};

    std::chrono::seconds header_timeout_;
    std::chrono::seconds total_timeout_;
    std::chrono::seconds keep_alive_timeout_;

    std::queue<std::shared_ptr<Request>> request_queue;
    mutable std::mutex queue_mutex;
    std::condition_variable queue_cv;


    static std::string 
    to_lower(std::string s)  
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    void 
    send_error(int fd, int status, const std::string& msg = "") 
    {
        Request dummy;
        dummy.client_fd = fd;
        dummy.keep_alive = false;
        dummy.error(status, msg);
    }

    void 
    acceptor() 
    {
        while (running) 
        {
            sockaddr_in client{};
            socklen_t len = sizeof(client);
            int fd = accept(listen_fd, (sockaddr*)&client, &len);
            if (fd < 0) 
            {
                if (!running) break;
                continue;
            }
            std::thread(&Server::handle_connection, this, fd).detach();
        }
    }

    void 
    handle_connection(int client) 
    {
        auto last_activity = std::chrono::steady_clock::now();

        while (running) 
        {
            // Check idle timeout for keep-alive
            if (std::chrono::steady_clock::now() - last_activity > keep_alive_timeout_) 
            {
                close(client);
                return;
            }

            std::string buffer;
            size_t header_end = std::string::npos;

            // Read until headers or timeout
            auto start = std::chrono::steady_clock::now();
            while (true) 
            {
                if (std::chrono::steady_clock::now() - start > header_timeout_) 
                {
                    send_error(client, 408, "Timeout waiting for request");
                    close(client);
                    return;
                }

                char chunk[2048];
                ssize_t n = recv(client, chunk, sizeof(chunk)-1, 0);
                if (n == 0) 
                { // client closed connection
                    close(client);
                    return;
                }
                if (n < 0) 
                {
                    close(client);
                    return;
                }
                buffer.append(chunk, n);

                header_end = buffer.find("\r\n\r\n");
                if (header_end != std::string::npos) break;

                if (buffer.size() > 16384) 
                {
                    send_error(client, 413, "Headers too large");
                    close(client);
                    return;
                }
            }

            std::string headers_part = buffer.substr(0, header_end);
            std::string body_part = buffer.substr(header_end + 4);

            // Parse request line
            std::istringstream iss(headers_part);
            std::string line;
            if (!std::getline(iss, line)) 
            {
                send_error(client, 400);
                close(client);
                return;
            }

            std::string method, path, version;
            std::istringstream req_line(line);
            if (!(req_line >> method >> path >> version)) 
            {
                send_error(client, 400);
                close(client);
                return;
            }

            if (method != "GET" && method != "PUT") 
            {
                send_error(client, 405);
                close(client);
                return;
            }

            // Parse headers
            std::map<std::string, std::string> headers;
            while(std::getline(iss, line)) 
            {
                if (line.size() <= 2) break;
                size_t colon = line.find(':');
                if (colon == std::string::npos) continue;
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                val.erase(0, val.find_first_not_of(" \t"));
                val.erase(val.find_last_not_of("\r\t ") + 1);
                headers[to_lower(key)] = val;
            }

            bool keep_alive = true;
            auto conn_it = headers.find("connection");
            if (conn_it != headers.end()) 
            {
                std::string conn_val = to_lower(conn_it->second);
                if (conn_val == "close") keep_alive = false;
                else if (conn_val == "keep-alive") keep_alive = true;
            } else 
            {
                // HTTP/1.1 default is keep-alive, HTTP/1.0 default is close
                keep_alive = (version == "HTTP/1.1");
            }

            // Handle body
            size_t content_length = 0;
            auto cl_it = headers.find("content-length");
            if (cl_it != headers.end()) 
            {
                try 
                {
                    content_length = std::stoul(cl_it->second);
                    if (content_length > 10 * 1024 * 1024) 
                    {
                        send_error(client, 413);
                        close(client);
                        return;
                    }
                } catch (...) 
                {
                    send_error(client, 400);
                    close(client);
                    return;
                }
            }

            std::string body = std::move(body_part);
            while (body.size() < content_length) 
            {
                char chunk[4096];
                ssize_t n = recv(client, chunk, sizeof(chunk), 0);
                if (n <= 0) break;
                body.append(chunk, n);
            }
            if (body.size() > content_length) body.resize(content_length);

            // Create request
            auto req = std::make_shared<Request>();
            req->method = std::move(method);
            req->path = std::move(path);
            req->headers = std::move(headers);
            req->body = std::move(body);
            req->client_fd = client;
            req->keep_alive = keep_alive;
            req->created_at = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lk(queue_mutex);
                request_queue.push(req);
            }
            queue_cv.notify_one();

            // Update last activity
            last_activity = std::chrono::steady_clock::now();

            // Wait for reply (simplified: we rely on reply() closing socket if !keep_alive)
            // In production you might want a timeout or condition variable per connection
        }
    }

    void 
    watchdog() 
    {
        while(running) 
        {
            std::this_thread::sleep_for(5s);
            // Could add cleanup of stale requests here if needed
        }
    }
};




int 
main() 
{
    try 
    {
        Server server(8080, 10s, 60s, 30s);
        std::cout << "Server listening on http://localhost:8080/ (keep-alive enabled)\n";

        while (server.pending_request_count() < 5) 
        {
            std::cout << "Waiting for requests..." << server.pending_request_count() << "\n";
            std::this_thread::sleep_for(1s);

        }

        while (true) 
        {
            auto req = server.get_next_request();

            std::cout << "→ " << req->method << " " << req->path << (req->keep_alive ? " [keep-alive]" : " [close]") << "\n";

            if (req->path == "/") 
            {
                req->reply("Hello! You can keep sending requests.\n");
            } 
            else if (req->path == "/close") 
            {
                req->reply("This response will close the connection.\n", "text/plain");
                // Force close even if client asked for keep-alive
                req->keep_alive = false;
                req->reply("Goodbye.\n"); // second reply to test behavior
            } 
            else 
            {
                req->error(404);
            }
        }
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}


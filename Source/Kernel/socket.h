//	socket.h		Socket utilities for the IKAROS project

#pragma once

#include <sys/types.h>

#include <string>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <queue>
#include <map>

#include "dictionary.h"

inline constexpr int PORTNO = 8000; // the default port clients will be connecting to

//
//	Socket used to request data from a HTTP server in a simple way
//

class Socket
	{
	public:
			explicit Socket(std::chrono::milliseconds timeout = std::chrono::seconds(5));
		Socket(const Socket &) = delete;
		Socket & operator=(const Socket &) = delete;
		Socket(Socket &&) = delete;
		Socket & operator=(Socket &&) = delete;
		~Socket();
		
		std::string 	HTTPGet(const std::string & url);
		bool			Poll(); // return true if data is waiting
		void			Close();

		bool			SendRequest(const char * hostname, int port, const char * request, const long size=-1);
		int				ReadData(char * result, ssize_t maxlen, bool fill=false);
		int				Get(const char * hostname, int port, const char * request, char * result, int maxlen);
			
	private:
			int sd = -1;
			std::chrono::milliseconds timeout_;
		};

//
//	The class ServerSocket implements a simple HTTP server
//

class ServerSocket
		{
	public:
		struct QueuedRequest
		{
			ikaros::dictionary	header;
			std::string			body;
			bool				close_after_response = true;
			int					connection_id = 0;
		};

		explicit ServerSocket(int port=PORTNO, const std::string & bind_address="");		// Create server socket on port
		ServerSocket(const ServerSocket &) = delete;
		ServerSocket & operator=(const ServerSocket &) = delete;
		ServerSocket(ServerSocket &&) = delete;
		ServerSocket & operator=(ServerSocket &&) = delete;
		~ServerSocket();
		
		bool				QueueRequest(bool block=false);
		bool				PopRequest(QueuedRequest & request);
		void				ActivateRequest(QueuedRequest request);
		bool				FinishActiveRequest();
		const ikaros::dictionary & RequestHeader() const;
		const std::string & RequestBody() const;
		
		bool				SendHTTPHeader(ikaros::dictionary d, const char * response=nullptr);
		bool				Append(const char * format, ...); // Maximum 1023 characters
		bool				Append(const std::string & data);
		bool				Flush();
		bool				SendData(const char * buffer, long size);
		bool				SendFile(const std::filesystem::path & filename); // FIXME: add header automatically based on file type
		void				StopListening();

		bool				Close();								// Close reply socket
		private:
			struct ConnectionState
			{
				int					fd = -1;
				std::string			input_buffer;
				std::string			output_buffer;
				std::chrono::steady_clock::time_point last_activity = std::chrono::steady_clock::now();
				bool				awaiting_more_input = false;
			};

			enum class RequestReadResult
		{
			complete,
			incomplete,
			closed,
			bad_request,
			method_not_allowed,
			length_required,
			payload_too_large,
			request_header_too_large,
			transfer_encoding_unsupported,
			http_version_unsupported,
		};

		int 			sockfd = -1;				// Listen on sock_fd,
		int				wakeup_read_fd = -1;
		int				wakeup_write_fd = -1;
		std::atomic<bool>	stop_requested{false};
		std::map<int, ConnectionState> connections;
		std::queue<QueuedRequest> pending_requests;
		int				next_connection_id = 1;
		int				current_read_connection_id = 0;
		int				active_connection_id = 0;
		bool			active_close_after_response = true;
		ikaros::dictionary active_header;
		std::string		active_body;
		bool				Poll();                                 // Accept a new connection if available
		bool				WaitForReadyConnection(bool block, int & connection_id);
		void				CloseIdleConnections();
		bool				CloseConnection(int connection_id);
		void				CloseSockets();
		void				SendRequestError(int connection_id, RequestReadResult error);
		ConnectionState *	ConnectionFor(int connection_id);
		RequestReadResult	ReadCurrentRequest(QueuedRequest & request);
		ssize_t				Read(char * buffer, size_t max_size);
		bool				SendFile(const std::filesystem::path & filename, ikaros::dictionary header);
		};

//	socket.h		Socket utilities for the IKAROS project

#pragma once

#include <sys/types.h>
#include <netinet/in.h>

#include <string>
#include <filesystem>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <map>

#include "dictionary.h"

inline constexpr int PORTNO = 8000; // the default port clients will be connecting to

//
//	Socket used to request data from a HTTP server in a simple way
//

class Socket
	{
	public:
		Socket() = default;
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
			long long			parse_duration_us = 0;
		};

		struct ConnectionState
		{
			int					fd = -1;
			std::string			input_buffer;
			std::string			output_buffer;
			std::chrono::steady_clock::time_point last_activity = std::chrono::steady_clock::now();
		};

		explicit ServerSocket(int port=PORTNO);		// Create server socket on port
		ServerSocket(const ServerSocket &) = delete;
		ServerSocket & operator=(const ServerSocket &) = delete;
		ServerSocket(ServerSocket &&) = delete;
		ServerSocket & operator=(ServerSocket &&) = delete;
		~ServerSocket();
		
		ikaros::dictionary	header;									// The current HTTP request header
		bool				GetRequest(bool block=false);           // Check for request and read if available
		bool				QueueRequest(bool block=false);
		bool				PopRequest(QueuedRequest & request, bool block=true);
		void				ActivateRequest(const QueuedRequest & request);
		bool				FinishActiveRequest();
		
		bool				SendHTTPHeader(ikaros::dictionary & d, const char * response=nullptr);
		bool				Append(const char * format, ...); // Maximum 1023 characters
		bool				Append(const std::string & data);
		bool				Flush();
		bool				SendData(const char * buffer, long size);
		bool				Send(const char * format, ...); // Compatibility wrapper around Append
		bool				Send(const std::string & data); // Compatibility wrapper around Append
		bool				SendFile(const std::filesystem::path & filename); // FIXME: add header automatically based on file type
		bool				SendFile(const std::filesystem::path & filename, ikaros::dictionary & header);
		bool				SendBuffer();
		void				StopListening();

		bool				Close();								// Close reply socket
		
		int					Port();									// Get port no
		const char *		Host();									// Get host name
		
		int					errcode;								// Last error
		std::string 		body;									// Body of PUT request
		private:
		int				portno;
		int 			sockfd = -1;				// Listen on sock_fd,
		std::map<int, ConnectionState> connections;
		std::queue<QueuedRequest> pending_requests;
		std::mutex		pending_requests_mutex;
		std::condition_variable pending_requests_cv;
		int				next_connection_id = 1;
		int				current_read_connection_id = 0;
		int				active_connection_id = 0;
		bool			active_close_after_response = true;
		int				block_flags;				// Original flags for blocking I/O
		struct sockaddr_in	my_addr; 					// My address information
		struct sockaddr_in	their_addr;					// Connector's address information
		bool				Poll(bool block=false);                 // Accept a new connection if available
		bool				WaitForReadyConnection(bool block, int & connection_id);
		void				CloseIdleConnections();
		bool				CloseConnection(int connection_id);
		ConnectionState *	ConnectionFor(int connection_id);
		bool				ReadCurrentRequest(QueuedRequest & request);
		size_t				Read(char * buffer, int maxSize, bool fill=false);	// Read, fill means fill with maxSize, return read size
		};

/*
Copyright (C) 2014 by Joachim Meyer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include "ConnectionHandler.h"


Connection::Connection(String address){
	createTCPConnection(address);
}


Connection::~Connection(){
	disconnect();
}

void Connection::createTCPConnection(String address) {
	int pathIndex;
	String host, port;
	this->address = address;
	
	int protocolIndex = address.find_first_of("://");
	if (protocolIndex != 0){
		protocolIndex += strlen("://");
		pathIndex = address.find_first_of("/", protocolIndex);

		if (pathIndex != 0){
			host = address.substr(protocolIndex, pathIndex - strlen("/"));
		} else {
			host = address.substr(protocolIndex, address.length());
		}
	} else {
		pathIndex = address.find_first_of(":");

		if (pathIndex != 0){
			host = address.substr(0, pathIndex);
			port = address.substr(pathIndex + 1);
		} else {
			host = address;
		}
	}

	struct sockaddr_in server;

	addrinfo *result = NULL;
	addrinfo hints;

	//Create a socket
#if PLATFORM == PLATFORM_WINDOWS
	char ipstringbuffer[46];
	unsigned long ipbufferlength = 46;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		Logger::log("We haven't been able to create a socket: %d\n", WSAGetLastError());
#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
	char* ipstringbuffer;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Logger::log("We haven't been able to create a socket: %s\n", strerror(errno));
#endif
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &result) != 0) {
#if PLATFORM == PLATFORM_WINDOWS
		Logger::log("Address resolve error: %d\n", WSAGetLastError());
#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
		Logger::log("Address resolve error: %s\n", strerror(errno));
#endif
		return;
	}

#if PLATFORM == PLATFORM_WINDOWS
	if (WSAAddressToStringA(result->ai_addr, (unsigned long)result->ai_addrlen, NULL, ipstringbuffer, &ipbufferlength) != 0) {
		Logger::log("Address to String convert error: %d\n", WSAGetLastError());
		return;
	}
#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
	in_addr addr;
	addr = ((sockaddr_in*)result->ai_addr)->sin_addr;
	ipstringbuffer = inet_ntoa(addr);
#endif

	String ipString = ipstringbuffer;
	ipString = ipString.substr(0, ipString.find_first_of(":"));

	server.sin_addr.s_addr = inet_addr(ipString.c_str());
	server.sin_family = AF_INET;
	server.sin_port = htons(port.toInteger());

	if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
#if PLATFORM == PLATFORM_WINDOWS
		Logger::log("connect error code: %d\n", WSAGetLastError());
#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
		Logger::log("connect error code: %s\n", strerror(errno));
#endif
		return;
	}
}

void Connection::createSSLConnetion(){
	sslHandle = NULL;
	sslContext = NULL;

	if (s)	{
		// Register the error strings for libcrypto & libssl
		SSL_load_error_strings();
		// Register the available ciphers and digests
		SSL_library_init();

		// New context saying we are a client, and using SSL 2 or 3
		sslContext = SSL_CTX_new(SSLv23_client_method());
		if (sslContext == NULL)
			ERR_print_errors_fp(stderr);

		// Create an SSL struct for the connection
		sslHandle = SSL_new(sslContext);
		if (sslHandle == NULL)
			ERR_print_errors_fp(stderr);

		// Connect the SSL struct to our connection
		if (!SSL_set_fd(sslHandle, s))
			ERR_print_errors_fp(stderr);

		// Initiate SSL handshake
		if (SSL_connect(sslHandle) != 1)
			ERR_print_errors_fp(stderr);
	} else
	{
		Logger::log("Connect failed");
	}
}

void Connection::disconnect(){
	if (s){
#ifdef _WINDOWS
		closesocket(s);
#else
		close(s);
#endif
	}
	if (sslHandle)
	{
		SSL_shutdown(sslHandle);
		SSL_free(sslHandle);
	}
	if (sslContext)
		SSL_CTX_free(sslContext);
}

char *Connection::sslRead(){
	const int readSize = 1024;
	char *rc = NULL;
	int received, count = 0;
	char buffer[1024];

	if (s){
		while (true){
			if (!rc)
				rc = (char*)malloc(readSize * sizeof(char) + 1);
			else
				rc = (char*)realloc(rc, (count + 1) *
				readSize * sizeof(char) + 1);

			received = SSL_read(sslHandle, buffer, readSize);
			buffer[received] = '\0';

			if (received > 0)
				strcat(rc, buffer);

			if (received < readSize)
				break;
			count++;
		}
	}

	return rc;
}

void Connection::sslWrite(char *text){
	if (s)
		SSL_write(sslHandle, text, strlen(text));
}

String Connection::getAddress(){
	return address;
}

ConnectionHandler::ConnectionHandler(){}

ConnectionHandler::~ConnectionHandler(){
	disconnectAll();
}

void ConnectionHandler::newConnection(String address){
	Connection* newCon = new Connection(address);
	connections.push_back(newCon);
}

Connection *ConnectionHandler::getConnection(String address){
	for (int i = 0; i < connections.size(); i++){
		if (connections[0]->getAddress() == address)
			return connections[i];
	}
	return NULL;
}

void ConnectionHandler::disconnect(String address){
	for (int i = 0; i < connections.size(); i++){
		if (connections[0]->getAddress() == address){
			delete connections[i];
			connections.erase(connections.begin() + i);
		}
	}
}

void ConnectionHandler::disconnectAll(){
	for (int i = 0; i < connections.size(); i++){
		delete connections[i];
	}
	connections.clear();
}

std::vector<String> ConnectionHandler::getAddresses(){
	std::vector<String> retVec;
	for (int i = 0; i < connections.size(); i++){
		retVec.push_back(connections[i]->getAddress());
	}
	return retVec;
}
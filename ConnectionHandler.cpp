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
#include "ChatFrame.h"
#include "openssl/crypto.h"

Connection::Connection(int clServ){
	//timer = new Timer(true, 500);
	this->clServ = clServ;

	sslMutex = Services()->getCore()->createMutex();
}

void Connection::disconnect(){
	if (s){
#ifdef _WINDOWS
		closesocket(s);
#else
		close(s);
#endif
	}
	if (sslHandle){
		SSL_shutdown(sslHandle);
		SSL_free(sslHandle);
	}
	if (sslContext)
		SSL_CTX_free(sslContext);
}

String Connection::sslRead(){
	const int readSize = 1024;
	String rc = String();
	int received, count = 0;
	char buffer[1024];

	if (s){
		while (true){
			received = SSL_read(sslHandle, buffer, readSize);
			
			if (SSL_get_error(sslHandle, received) == SSL_ERROR_ZERO_RETURN){
				received = 0;
				Logger::log("SSL_ERROR_ZERO_RETURN\n");
			} else if (SSL_get_error(sslHandle, received) == SSL_ERROR_SYSCALL){
				received = 0;
				Logger::log("SSL_ERROR_SYSCALL\n");
				
				ConnectionEvent* connE = new ConnectionEvent();
				connE->connection = this;
				dispatchEvent(connE, ConnectionEvent::DISCONNECT_EVENT);
				
				killThread();
				return String();
			}
			
			buffer[received] = '\0';

			if (received > 0){
				for (int i = 0; i < received; i++){
					rc.append(buffer[i]);
				}
			}

			if (received < readSize)
				break;
			count++;
		}
		dispatchEvent(new ChatEvent(address, rc), ChatEvent::EVENT_RECEIVE_CHAT_MSG);
	}

	return rc;
}

void Connection::sslWrite(String text){
	if (s)
		SSL_write(sslHandle, text.c_str(), text.length());
}

void Connection::runThread(){
	while (threadRunning) {
		updateThread();
	}
}

ConnectionClient::ConnectionClient(String address) : Connection(Connection::CONN_CLIENT){
	this->core = Services()->getCore();
	this->eventMutex = Services()->getCore()->getEventMutex();
	
	createTCPConnection(address);
	
	if (allSuccess)
		createSSLConnetion();

	if (allSuccess)
		core->createThread(this);
}

ConnectionClient::ConnectionClient(int socket, SSL *handle, String address) : Connection(Connection::CONN_CLIENT){
	this->core = Services()->getCore();
	this->eventMutex = Services()->getCore()->getEventMutex();

	s = socket;
	this->address = address;
	sslHandle = handle;

	core->createThread(this);
}

ConnectionClient::~ConnectionClient(){
	disconnect();
}

void ConnectionClient::createTCPConnection(String address) {
	allSuccess = false;
	int pathIndex;
	String host, port;
	this->address = address;
	
	pathIndex = address.find_first_of(":");

	if (pathIndex != 0){
		host = address.substr(0, pathIndex);
		port = address.substr(pathIndex + 1);
	} else {
		host = address;
		port = "5534";
	}

	struct sockaddr_in server;

	addrinfo *result = NULL;
	addrinfo hints;

	//Create a socket
#if PLATFORM == PLATFORM_WINDOWS
	char ipstringbuffer[46];
	unsigned long ipbufferlength = sizeof(ipstringbuffer);

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
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
	allSuccess = true;
}

void ConnectionClient::createSSLConnetion(){
	allSuccess = false;
	sslHandle = NULL;
	sslContext = NULL;

	if (s)	{
		// Register the error strings for libcrypto & libssl
		SSL_load_error_strings();
		// Register the available ciphers and digests
		SSL_library_init();

		// New context saying we are a client, and using SSL 2 or 3
		sslContext = SSL_CTX_new(SSLv23_client_method());
		if (sslContext == NULL){
			ERR_print_errors_fp(stderr);
			return;
		}

		// Create an SSL struct for the connection
		sslHandle = SSL_new(sslContext);
		if (sslHandle == NULL){
			ERR_print_errors_fp(stderr);
			return;
		}

		// Connect the SSL struct to our connection
		if (!SSL_set_fd(sslHandle, s)){
			ERR_print_errors_fp(stderr);
			return;
		}

		// Initiate SSL handshake
		if (SSL_connect(sslHandle) != 1){
			ERR_print_errors_fp(stderr);
			return;
		}
	} else {
		Logger::log("Connect failed");
	}
	allSuccess = true;
}

void ConnectionClient::updateThread(){
	Services()->getCore()->lockMutex(sslMutex);
	sslRead();
	Services()->getCore()->unlockMutex(sslMutex);
#ifdef _WINDOWS
	Sleep(500);
#else
	usleep(500 * 1000);
#endif
}

ConnectionServer::ConnectionServer() : Connection(ConnectionClient::CONN_SERVER) {
	certFile = "Assets/certFile.pem";
	this->core = Services()->getCore();
	this->eventMutex = Services()->getCore()->getEventMutex();

	sslHandle = NULL;

	openListener(5534);

	if (allSuccess)
		createSSLContext();

	if (allSuccess)
		loadCerts();

	if (allSuccess){
		core->createThread(this);
	}
}

ConnectionServer::~ConnectionServer(){
	disconnect();
}

void ConnectionServer::openListener(int port) {
	struct sockaddr_in addr;
	allSuccess = false;

#ifdef _WINDOWS
	if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET){
		Logger::log("Error creating server socket: %d\n", WSAGetLastError());
#else
	if(sd = socket(PF_INET, SOCK_STREAM, 0) == -1){
		Logger::log("Error creating server socket: %s\n", strerror(errno));
#endif
	return;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) != 0)	{
#ifdef _WINDOWS
		Logger::log("Server can't bind port: %d\n", WSAGetLastError());
#else
		Logger::log("Server can't bind port: %s\n", strerror(errno));
#endif
		return;
	}

	if (listen(s, SOMAXCONN) != 0){
#ifdef _WINDOWS
		Logger::log("Can't configure listening port: %d\n", WSAGetLastError());
#else
		Logger::log("Can't configure listening port: %s\n", strerror(errno));
#endif
		return;
	}
	allSuccess = true;
}

void ConnectionServer::createSSLContext(){
	allSuccess = false;
	if (s){
		OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
		SSL_load_error_strings();   /* load all error messages */
		SSL_library_init();
		sslContext = SSL_CTX_new(SSLv3_server_method());   /* create new context from method */
		if (sslContext == NULL){
			ERR_print_errors_fp(stderr);
			return;
		}
		allSuccess = true;
	}
}

void ConnectionServer::loadCerts(){
	allSuccess = false;
	if (!OSBasics::fileExists(certFile)){
		Services()->getCore()->executeExternalCommand("set OPENSSL_CONF=openssl.cfg && openssl", "req -x509 -nodes -days 365 -newkey rsa:1024 -keyout " + certFile.substr(certFile.find_last_of("/") + String("/").length()) + " -out " + certFile.substr(certFile.find_last_of("/") + String("/").length()) + " -batch", "Assets");
	}

	/* set the local certificate from CertFile */
	if (SSL_CTX_use_certificate_file(sslContext, certFile.c_str(), SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		return;
	}
	/* set the private key from KeyFile (may be the same as CertFile) */
	if (SSL_CTX_use_PrivateKey_file(sslContext, certFile.c_str(), SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		return;
	}
	/* verify private key */
	if (!SSL_CTX_check_private_key(sslContext))
	{
		fprintf(stderr, "Private key does not match the public certificate\n");
		return;
	}
	allSuccess = true;
}

void ConnectionServer::updateThread(){
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	int client = accept(s, (struct sockaddr*)&addr, &len); /* accept connection as usual */
	address = String(inet_ntoa(addr.sin_addr)) + String(":") + String(ntohs(addr.sin_port));
	Logger::log("Connection: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	sslHandle = SSL_new(sslContext);  /* get new SSL state with context */
	SSL_set_fd(sslHandle, client); /* set connection socket to SSL state */
	SSL_accept(sslHandle);

	ConnectionClient *newClient = new ConnectionClient(client, sslHandle, address);
	ConnectionEvent *connE = new ConnectionEvent();
	connE->connection = newClient;
	dispatchEvent(connE, ConnectionEvent::CONNECT_EVENT);
}

String Connection::getAddress(){
	return address;
}

ConnectionHandler::ConnectionHandler(){
	server = new ConnectionServer();
	server->addEventListener(this, ConnectionEvent::CONNECT_EVENT);

	this->addEventListener(this, ConnectionEvent::CONNECT_EVENT);
	this->addEventListener(this, ChatEvent::EVENT_RECEIVE_CHAT_MSG);
}

ConnectionHandler::~ConnectionHandler(){
	disconnectAll();
}

bool ConnectionHandler::newConnection(String address){
	Connection* newCon = new ConnectionClient(address);
	if (newCon->getSuccess()){
		newCon->addEventListener(this, ChatEvent::EVENT_RECEIVE_CHAT_MSG);
		newCon->addEventListener(this, ConnectionEvent::DISCONNECT_EVENT);
		connections.push_back(newCon);
		return true;
	}
	return false;
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
			Services()->getCore()->removeThread(connections[i]);
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

void ConnectionHandler::handleEvent(Event* e){
	if (e->getEventType() == "ChatEvent"){
		ChatEvent* event = (ChatEvent*)e;
		if (event->getEventCode() == ChatEvent::EVENT_SEND_CHAT_MSG){
			getConnection(event->address)->sslWrite(event->message);
		}
	}
	if (e->getEventType() == "ConnEvent" && e->getDispatcher() == this && e->getEventCode() == ConnectionEvent::CONNECT_EVENT){
		ConnectionEvent *event = (ConnectionEvent*)e;
		event->connection->addEventListener(this, ChatEvent::EVENT_RECEIVE_CHAT_MSG);
		connections.push_back(event->connection);
	}
}

void ConnectionHandler::Update(){
	Core* core = Services()->getCore();
	core->lockMutex(server->eventMutex);
	for (int i = server->eventQueue.size() -1; i >= 0; i--){
		dispatchEvent(server->eventQueue[i], server->eventQueue[i]->getEventCode());
		server->eventQueue.erase(server->eventQueue.begin() + i);
	}
	core->unlockMutex(server->eventMutex);

	for (int c = 0; c < connections.size(); c++){
		Connection *connection = connections[c];
		core->lockMutex(connection->eventMutex);
		Event *event = new Event();
		for (int e = connection->eventQueue.size() - 1; e >= 0; e--){
			event = connection->eventQueue[e];
			connection->eventQueue.erase(connection->eventQueue.begin() + e);
			dispatchEventNoDelete(event, event->getEventCode());
		}
		if (event->getEventCode() != ConnectionEvent::DISCONNECT_EVENT)
			core->unlockMutex(connection->eventMutex);
	}
}
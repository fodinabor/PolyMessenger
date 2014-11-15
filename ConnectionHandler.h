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
#pragma once
#include <Polycode.h>

#include <string.h>
#ifdef _WINDOWS
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace Polycode;

class Connection : public Threaded {
public:
	Connection(int clServ);

	void disconnect();

	String sslRead();
	void sslWrite(String text);

	String getAddress();

	//void handleEvent(Event *e);

	const static int CONN_SERVER = 0;
	const static int CONN_CLIENT = 0;

protected:
	String address;
	int s;
	SSL *sslHandle;
	SSL_CTX *sslContext;

	int clServ;

	//Timer *timer;

	bool allSuccess;
};

class ConnectionClient : public Connection {
public:
	ConnectionClient(String address);
	ConnectionClient(int socket, SSL *handle, String address);
	~ConnectionClient();

	void createTCPConnection(String address);
	void createSSLConnetion();

	void updateThread();
};

class ConnectionServer : public Connection {
public:
	ConnectionServer();
	~ConnectionServer();

	void openListener(int port);
	void acceptConnection();

	void createSSLContext();

	void updateThread();

	void loadCerts();

	//void handleEvent(Event *e);

private:
	String certFile;

	//SSL *clSocket;
};

class ConnectionEvent : public Event {
public:
	ConnectionEvent(){ eventType = "ConnEvent"; }
	~ConnectionEvent(){}

	Connection *connection;

	static const int CONNECT_EVENT = EVENTBASE_NONPOLYCODE + 250;
	static const int DISCONNECT_EVENT = EVENTBASE_NONPOLYCODE + 251;
	static const int DO_CONNECT_EVENT = EVENTBASE_NONPOLYCODE + 252;
};

class ConnectionHandler : public EventDispatcher {
public:
	ConnectionHandler();
	~ConnectionHandler();

	void newConnection(String address);

	void Update();

	void disconnectAll();
	void disconnect(String address);

	void handleEvent(Event *e);

	Connection* getConnection(String address);

	std::vector<String> getAddresses();

private:
	ConnectionServer *server;
	std::vector<Connection*> connections;

};
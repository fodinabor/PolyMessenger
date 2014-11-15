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
#include "ConnectionWindow.h"
#include "MessengerFrame.h"
#include "ChatFrame.h"

extern MessengerFrame *globalFrame;

ConnectionWindow::ConnectionWindow() : UIWindow("Connections",480,320){
	closeOnEscape = true;

	connections = new ConnectionHandler();
	connections->addEventListener(this, ConnectionEvent::CONNECT_EVENT);
	connections->addEventListener(this, ConnectionEvent::DISCONNECT_EVENT);

	connList = new UITreeContainer("Assets/internet.png", "Open Connections", 350, 300);
	connList->getRootNode()->addEventListener(this, UITreeEvent::SELECTED_EVENT);
	connList->getRootNode()->addEventListener(this, UITreeEvent::EXECUTED_EVENT);
	addChild(connList);
	connList->setPosition(10, 10);

	newConnBtn = new UIButton("New Connection", 100);
	newConnBtn->addEventListener(this, UIEvent::CLICK_EVENT);
	addChild(newConnBtn);
	newConnBtn->setPosition(375, 20);

	addrInput = new UITextInput(false, 100, 25);
	addChild(addrInput);
	addrInput->setPosition(375, 50);
}

ConnectionWindow::~ConnectionWindow(){

}

void ConnectionWindow::newConnection(){
	if (connections->newConnection(addrInput->getText())){
		globalFrame->newChat(addrInput->getText());
		addrInput->setText("");
	}
	refreshList();
}

void ConnectionWindow::refreshList(){
	connList->getRootNode()->clearTree();
	std::vector<String> addresses = connections->getAddresses();
	for (int i = 0; i < addresses.size(); i++){
		connList->getRootNode()->addTreeChild("Assets/globe.png", addresses[i]);
	}
	if (connList->getRootNode()->isCollapsed())
		connList->getRootNode()->toggleCollapsed();
}

String ConnectionWindow::getSelected(){
	return selection;
}

void ConnectionWindow::handleEvent(Event *e){
	if (e->getDispatcher() == connList->getRootNode()){
		if (e->getEventCode() == UITreeEvent::EXECUTED_EVENT){
			this->selection = ((UITreeEvent*)e)->selection->getLabelText();
			globalFrame->showChat(selection);
			dispatchEvent(new UIEvent(), UIEvent::CLOSE_EVENT);
		}
	}

	if (e->getDispatcher() == newConnBtn){
		newConnection();
	}

	if (e->getEventType() == "ConnEvent"){
		ConnectionEvent *connE = (ConnectionEvent*)e;
		if (e->getDispatcher() == connections){
			if (e->getEventCode() == ConnectionEvent::CONNECT_EVENT){
				globalFrame->showModal(this);
				refreshList();
				globalFrame->newChat(addrInput->getText());
			}
			if (e->getEventCode() == ConnectionEvent::DISCONNECT_EVENT){
				connections->disconnect(connE->connection->getAddress());
				refreshList();
				globalFrame->getChatForAddress(connE->connection->getAddress())->newMessage("Partner disconnected..", ChatMessage::SYSTEM_MESSAGE);
			}
		}
	}

	UIWindow::handleEvent(e);
}
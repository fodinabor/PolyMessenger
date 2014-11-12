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

extern MessengerFrame *globalFrame;

ConnectionWindow::ConnectionWindow() : UIWindow("Connections",480,320){
	closeOnEscape = true;

	connections = new ConnectionHandler();

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

void ConnectionWindow::Update(){
	
}

void ConnectionWindow::newConnection(){
	connections->newConnection(addrInput->getText());
	addrInput->setText("");
	refreshList();
}

void ConnectionWindow::refreshList(){
	connList->getRootNode()->clearTree();
	for (int i = 0; i < connections->getAddresses().size(); i++){
		connList->getRootNode()->addTreeChild("Assets/globe.png", connections->getAddresses()[i]);
	}
}

String ConnectionWindow::getSelected(){
	return selection;
}

void ConnectionWindow::handleEvent(Event *e){
	if (e->getDispatcher() == connList->getRootNode()){
		if (e->getEventCode() == UITreeEvent::EXECUTED_EVENT){
			this->selection = ((UITreeEvent*)e)->selection->getLabelText();
			dispatchEvent(new UIEvent(), UIEvent::OK_EVENT);
		}
	}

	if (e->getDispatcher() == newConnBtn){
		newConnection();
	}

	UIWindow::handleEvent(e);
}
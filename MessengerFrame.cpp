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
#include "MessengerFrame.h"
#include "ChatFrame.h"
#include "ConnectionWindow.h"

MessengerFrame::MessengerFrame(){
	connBtn = new UIButton("Connections", 100);
	connBtn->addEventListener(this, UIEvent::CLICK_EVENT);
	addChild(connBtn);
	connBtn->setPosition(50,50);

	connWin = new ConnectionWindow();
	connWin->setPosition(getHeight()/2 - connWin->getHeight()/2, getWidth()/2 - connWin->getWidth()/2);
	connWin->connections->addEventListener(this, ChatEvent::EVENT_RECEIVE_CHAT_MSG);

	modalBlocker = new UIRect(10, 10);
	modalBlocker->setBlendingMode(Renderer::BLEND_MODE_NORMAL);
	modalBlocker->setColor(0, 0, 0, 0.4);
	modalBlocker->setAnchorPoint(-1.0, -1.0, 0.0);
	modalBlocker->enabled = false;
	modalBlocker->blockMouseInput = true;
	modalBlocker->processInputEvents = true;
	addChild(modalBlocker);

	modalRoot = new UIElement();
	addChild(modalRoot);

	modalChild = NULL;

	visibleChat = NULL;

	Resize(Services()->getCore()->getXRes(), Services()->getCore()->getYRes());
}

MessengerFrame::~MessengerFrame(){}

void MessengerFrame::showChat(String address){
	if (visibleChat){
		removeChild(visibleChat);
		visibleChat->removeAllHandlers();
	}

	for (int i = 0; i < chatFrames.size(); i++){
		if (address == chatFrames[i]->getAddress()){
			visibleChat = chatFrames[i];
			visibleChat->addEventListener(this, ChatEvent::EVENT_SEND_CHAT_MSG);
			addChild(visibleChat);
			return;
		}
	}

	ChatFrame *chat = new ChatFrame(address);
	chatFrames.push_back(chat);
	visibleChat = chat;
	visibleChat->addEventListener(this, ChatEvent::EVENT_SEND_CHAT_MSG);
	addChild(visibleChat);
}

void MessengerFrame::showModal(UIWindow* modalChild){
	modalBlocker->enabled = true;

	//focusChild(NULL);

	this->modalChild = modalChild;
	modalRoot->addChild(modalChild);
	modalChild->showWindow();
	modalChild->addEventListener(this, UIEvent::CLOSE_EVENT);
	Resize(getWidth(), getHeight());

	CoreServices::getInstance()->getCore()->setCursor(Core::CURSOR_ARROW);
}

void MessengerFrame::hideModal() {
	if (modalChild) {
		modalRoot->removeChild(modalChild);
		//assetBrowser->removeAllHandlers();
		modalChild->hideWindow();
		modalChild = NULL;
	}
	modalBlocker->enabled = false;
}

TextInputPopup* MessengerFrame::showTextInput(String caption, String action, String value){
	textPopup->action = action;
	textPopup->setCaption(caption);
	textPopup->setValue(value);
	showModal(textPopup);
	return textPopup;
}

void MessengerFrame::Resize(int width, int height){
	setHeight(height);
	setWidth(width);

	modalBlocker->Resize(width, height);

	connWin->setPosition(getHeight() / 2 - connWin->getHeight() / 2, getWidth() / 2 - connWin->getWidth() / 2);
}

void MessengerFrame::handleEvent(Event *e){
	if (e->getEventType() == "UIEvent"){
		if (e->getDispatcher() == modalChild){
			if (e->getEventCode() == UIEvent::CLOSE_EVENT){
				hideModal();
			}
		}
		if (e->getDispatcher() == connBtn){
			showModal(connWin);
		}
	}
	if (e->getEventType() == "ChatEvent"){
		ChatEvent* cEvent = (ChatEvent*)e;
		if (cEvent->getEventCode() == ChatEvent::EVENT_SEND_CHAT_MSG){
			connWin->connections->getConnection(cEvent->address)->sslWrite(const_cast<char*>(cEvent->message.c_str()));
		}
		if (cEvent->getEventCode() == ChatEvent::EVENT_RECEIVE_CHAT_MSG){
			getChatForAddress(cEvent->address)->newMessage(cEvent->message, ChatMessage::PARTNER_MESSAGE);
		}
	}
}

ConnectionHandler* MessengerFrame::getConnections(){
	return connWin->connections;
}

ChatFrame* MessengerFrame::getChatForAddress(String address){
	for (int i = 0; i < chatFrames.size(); i++){
		if (chatFrames[i]->getAddress() == address){
			return chatFrames[i];
		}
	}
}
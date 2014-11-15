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
#include "ChatFrame.h"
#include "MessengerFrame.h"
#include "ToolWindows.h"

extern UIGlobalMenu *globalMenu;
extern MessengerFrame *globalFrame;

ChatMessage::ChatMessage(String content, int who){
	message = new UIMultilineLabel(content, 12, 4, "sans");

	messageBg = new UIRect(200, message->getHeight() + 5);
	
	if (who == ChatMessage::OWN_MESSAGE)
		messageBg->setColor(0.2, 0.5, 0.5, 0.4);
	else
		messageBg->setColor(0.5, 0.5, 0.2, 0.4);

	messageBg->setBlendingMode(Renderer::BLEND_MODE_NORMAL);
	messageBg->addEventListener(this, InputEvent::EVENT_MOUSEDOWN);
	messageBg->processInputEvents = true;
	addChild(messageBg);

	addChild(message);

	updateSize();
}

ChatMessage::~ChatMessage(){}

void ChatMessage::updateSize(){
	setHeight(message->getHeight() + 10);
	messageBg->setHeight(message->getHeight() + 5);
}

void ChatMessage::handleEvent(Event *e){
	if (e->getDispatcher() == menu) {
		if (menu->getSelectedItem()->_id == "delete_message") {
			dispatchEvent(new UIEvent(), UIEvent::REMOVE_EVENT);
		} else if (menu->getSelectedItem()->_id == "edit_message") {
			textPopup = globalFrame->showTextInput("Edit Message.", "edit_message", message->getText());
			textPopup->addEventListener(this, UIEvent::OK_EVENT);
		}
	}

	if (e->getDispatcher() == messageBg){
		if (e->getEventCode() == InputEvent::EVENT_MOUSEDOWN){
			dispatchEvent(new Event(), Event::SELECT_EVENT);
			InputEvent *inputEvent = (InputEvent*)e;
			if (inputEvent->mouseButton == CoreInput::MOUSE_BUTTON2){
				menu = globalMenu->showMenuAtMouse(120);
				menu->addOption("Copy", "copy_message");
				menu->addDivider();
				menu->addOption("Edit", "edit_message");
				menu->addOption("Delete", "delete_message");
				menu->addEventListener(this, UIEvent::OK_EVENT);
			}
		}
	}
	UIElement::handleEvent(e);
}

ChatFrame::ChatFrame(String address) : UIElement(200, Services()->getCore()->getYRes()){
	this->address = address;
	setPosition(Services()->getCore()->getXRes() / 2 - getWidth() / 2, 0);

	chatInput = new UITextInput(true, 200, 50);
	addChild(chatInput);
	chatInput->setPositionY(getHeight() - chatInput->getHeight());

	sendBtn = new UIButton("Send.", 50);
	sendBtn->addEventListener(this, UIEvent::CLICK_EVENT);
	chatInput->addChild(sendBtn);
	sendBtn->setPositionX(chatInput->getWidth() + 30);
}

ChatFrame::~ChatFrame(){}

void ChatFrame::newMessage(String message, int who){
	ChatMessage *newMsg;
	if (message.find_last_of("\n") < message.length() - String("\n").length()){
		newMsg = new ChatMessage(message, who);
	} else {
		newMsg = new ChatMessage(message.substr(0, message.find_last_of("\n")), who);
	}
	messages.push_back(newMsg);
	addChild(newMsg);
	Resize(getHeight(), getWidth());
}

String ChatFrame::getAddress(){
	return address;
}

void ChatFrame::Resize(int width, int height){
	setWidth(width);
	setHeight(height);

	int offsetY = 0;
	for (int i = 0; i < messages.size(); i++){
		messages[i]->updateSize();
		messages[i]->setPositionY(offsetY);
		offsetY += messages[i]->getHeight();
	}

	chatInput->Resize(200, 50);
	chatInput->setPositionY(getHeight() - chatInput->getHeight());
}

void ChatFrame::handleEvent(Event* e){
	if (e->getDispatcher() == sendBtn && e->getEventType() == "UIEvent"){
		newMessage(chatInput->getText(), ChatMessage::OWN_MESSAGE);
		dispatchEvent(new ChatEvent(address, chatInput->getText()), ChatEvent::EVENT_SEND_CHAT_MSG);
		chatInput->setText("", false);
	}
}

ChatEvent::ChatEvent(String address, String message) : Event(){
	this->address = address;
	this->message = message;

	eventType = "ChatEvent";
}
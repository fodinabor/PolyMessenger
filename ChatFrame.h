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
#include <PolycodeUI.h>

class TextInputPopup;

class ChatMessage : public UIElement {
public:
	ChatMessage(String content, int who);
	~ChatMessage();

	void updateSize();
	void handleEvent(Event *e);

	static const int OWN_MESSAGE = 0;
	static const int PARTNER_MESSAGE = 1;

private:
	UIMultilineLabel *message;
	UIRect *messageBg;

	UIMenu *menu;
	TextInputPopup *textPopup;
};

class ChatFrame : public UIElement {
public:
	ChatFrame(String address);
	~ChatFrame();

	void newMessage(String message, int who);

	String getAddress();

	void Resize(int width, int height);
	void handleEvent(Event* e);
private:
	UITextInput *chatInput;
	UIButton *sendBtn;

	String address;

	std::vector<ChatMessage*> messages;
};

class ChatEvent : public Event {
public:
	ChatEvent(String Address, String message);
	~ChatEvent(){}

	String address;
	String message;
	int msgID;

	static const int EVENT_SEND_CHAT_MSG = EVENTBASE_NONPOLYCODE + 325;
	static const int EVENT_DELETE_CHAT_MSG = EVENTBASE_NONPOLYCODE + 326;
	static const int EVENT_CHANGE_CHAT_MSG = EVENTBASE_NONPOLYCODE + 327;
	static const int EVENT_RECEIVE_CHAT_MSG = EVENTBASE_NONPOLYCODE + 328;
};
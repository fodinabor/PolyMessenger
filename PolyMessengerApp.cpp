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
#include "PolyMessengerApp.h"
#include "ConnectionWindow.h"

Scene* globalScene;
UIGlobalMenu* globalMenu;
MessengerFrame* globalFrame;

PolyMessengerApp::PolyMessengerApp(PolycodeView *view) {
	Services()->getLogger()->setLogToFile(true);
	core = new Win32Core(view, 1000, 500, false, false, 0, 0, 15);
	core->addEventListener(this, Core::EVENT_CORE_RESIZE);

	CoreServices::getInstance()->getResourceManager()->addArchive("default.pak");
	CoreServices::getInstance()->getResourceManager()->addDirResource("default", false);
	CoreServices::getInstance()->getResourceManager()->addArchive("Assets/UIThemes.pak");
	CoreServices::getInstance()->getResourceManager()->addDirResource("UIThemes", true);
	CoreServices::getInstance()->getConfig()->loadConfig("Polycode", "UIThemes/dark/theme.xml");
	
	SceneLabel::defaultAnchor = Vector3(-1.0, -1.0, 0.0);
	SceneLabel::defaultPositionAtBaseline = true;
	SceneLabel::defaultSnapToPixels = true;
	SceneLabel::createMipmapsForLabels = false;

	Entity::defaultBlendingMode = Renderer::BLEND_MODE_NONE;
	CoreServices::getInstance()->getRenderer()->setTextureFilteringMode(Renderer::TEX_FILTERING_NEAREST);

	Scene *screen = new Scene(Scene::SCENE_2D_TOPLEFT);

	screen->doVisibilityChecking(false);
	screen->getDefaultCamera()->frustumCulling = false;
	screen->rootEntity.processInputEvents = true;

	screen->clearColor.setColorHexFromString(CoreServices::getInstance()->getConfig()->getStringValue("Polycode", "uiBgColor"));
	screen->useClearColor = true;

	globalScene = screen;

	globalMenu = new UIGlobalMenu();
	UITextInput::setMenuSingleton(globalMenu);

	screen->addChild(globalMenu);

	mainFrame = new MessengerFrame();
	screen->addChild(mainFrame);
	globalFrame = mainFrame;
}

PolyMessengerApp::~PolyMessengerApp() {
    
}

bool PolyMessengerApp::Update() {
	if ((Services()->getInput()->getKeyState(KEY_LALT) || Services()->getInput()->getKeyState(KEY_RALT)) && Services()->getInput()->getKeyState(KEY_F4))
		core->Shutdown();

	mainFrame->getConnections()->Update();

	return core->updateAndRender();
}

void PolyMessengerApp::handleEvent(Event* e){
	if (e->getDispatcher() == core){
		if (e->getEventCode() == Core::EVENT_CORE_RESIZE){
			mainFrame->Resize(core->getXRes(), core->getYRes());
		}
	}
}
#include "OrganicMainComponent.h"

ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

OrganicMainContentComponent::OrganicMainContentComponent()
{
	setSize(800, 600);

	Engine::mainEngine->addEngineListener(this);
	
	lookAndFeelOO = new LookAndFeelOO();
	LookAndFeel::setDefaultLookAndFeel(lookAndFeelOO);
	
	(&getCommandManager())->registerAllCommandsForTarget(this);
	(&getCommandManager())->setFirstCommandTarget(this);
	
	(&getCommandManager())->getKeyMappings()->resetToDefaultMappings();
	addKeyListener((&getCommandManager())->getKeyMappings());
	

#if JUCE_MAC
	setMacMainMenu(this, nullptr, "");
#else
	//done in Main.cpp as it's a method of DocumentWindow
#endif



#if JUCE_OPENGL
	openGLContext.setComponentPaintingEnabled(true);
	openGLContext.attachTo(*this);
#endif

}

OrganicMainContentComponent::~OrganicMainContentComponent()
{
#if JUCE_MAC
	setMacMainMenu(nullptr, nullptr, "");
#endif

	if (Engine::mainEngine != nullptr) Engine::mainEngine->removeEngineListener(this);
	ShapeShifterManager::deleteInstance();
}

void OrganicMainContentComponent::init()
{
	String lastVersion = getAppProperties().getCommonSettings(true)->getValue("lastVersion", "0");
	if (lastVersion != getAppVersion())
	{
		ShapeShifterManager::getInstance()->loadDefaultLayoutFile(true);
	} else
	{
		ShapeShifterManager::getInstance()->loadLastSessionLayoutFile();
	}


	addAndMakeVisible(&ShapeShifterManager::getInstance()->mainContainer);
	grabKeyboardFocus();
}

void OrganicMainContentComponent::clear()
{

#if JUCE_OPENGL
	openGLContext.detach();
	openGLContext.setRenderer(nullptr);
#endif
}


void OrganicMainContentComponent::paint(Graphics& g)
{
	g.fillAll(BG_COLOR.darker());
}

void OrganicMainContentComponent::resized()
{
	juce::Rectangle<int> r = getLocalBounds();
	ShapeShifterManager::getInstance()->mainContainer.setBounds(r);
}


void OrganicMainContentComponent::startLoadFile()
{

	if (fileProgressWindow != nullptr)
	{
		removeChildComponent(fileProgressWindow);
		fileProgressWindow = nullptr;
	}

	fileProgressWindow = new ProgressWindow("Loading File...", Engine::mainEngine);
	addAndMakeVisible(fileProgressWindow);
	fileProgressWindow->setSize(getWidth(), getHeight());
}

void OrganicMainContentComponent::fileProgress(float percent, int state)
{
	if (fileProgressWindow != nullptr)
	{
		fileProgressWindow->setProgress(percent);
	} else
	{
		DBG("Window is null but still got progress");
	}
}

void OrganicMainContentComponent::endLoadFile()
{
	if (fileProgressWindow != nullptr)
	{
		removeChildComponent(fileProgressWindow);
		fileProgressWindow = nullptr;
	}
}

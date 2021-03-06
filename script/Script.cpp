#include "Script.h"
/*
==============================================================================

Script.cpp
Created: 20 Feb 2017 5:01:11pm
Author:  Ben

==============================================================================
*/

Script::Script(ScriptTarget * _parentTarget, bool canBeDisabled) :
	BaseItem("Script", canBeDisabled, false),
	scriptParamsContainer("params"),
	parentTarget(_parentTarget),
	scriptAsyncNotifier(10)
{
	isSelectable = false;

	filePath = new FileParameter("File Path", "Path to the script file", "");
	addParameter(filePath);

	logParam = addBoolParameter("Log", "Utility parameter to easily activate/deactivate logging from the script", false);
	logParam->setCustomShortName("enableLog");
	logParam->hideInEditor = true;

	reload = addTrigger("Reload", "Reload the script");
	reload->hideInEditor = true;

	scriptObject.setMethod("log", Script::logFromScript);
	scriptObject.setMethod("addBoolParameter", Script::addBoolParameterFromScript);
	scriptObject.setMethod("addIntParameter", Script::addIntParameterFromScript);
	scriptObject.setMethod("addFloatParameter", Script::addFloatParameterFromScript);
	scriptObject.setMethod("addEnumParameter", Script::addEnumParameterFromScript);
	scriptObject.setMethod("addTargetParameter", Script::addTargetParameterFromScript);
	scriptObject.setMethod("addColorParameter", Script::addColorParameterFromScript);
	scriptObject.setMethod("addTrigger", Script::addTriggerFromScript);

	scriptParamsContainer.hideEditorHeader = true;
	addChildControllableContainer(&scriptParamsContainer);

	startTimer(1, 1000);
}

Script::~Script()
{

}

void Script::loadScript()
{
	if (Engine::mainEngine->isLoadingFile)
	{
		Engine::mainEngine->addEngineListener(this);
		//return;
	}


	setState(SCRIPT_LOADING);

	String path = filePath->getAbsolutePath();

	if (path.isEmpty())
	{
		return;
	}

#if JUCE_WINDOWS
	if (path.startsWithChar('/')) //avoid jassertfalse
	{
		NLOG(niceName, "Path " + path + " is not a valid Windows-style path.");
		setState(SCRIPT_ERROR);
		return; //OSX / Linux file
	}
#endif

	File f = filePath->getFile();

	if (!f.exists())
	{
		//check local director
		NLOG("Script", "File not found : " + f.getFileName());
		setState(SCRIPT_EMPTY);
		return;
	}

	fileName = f.getFileName();
	setNiceName(f.getFileNameWithoutExtension());
	fileLastModTime = f.getLastModificationTime();
	String s = f.loadFileAsString();

	stopTimer(0);

	if(paramsContainerData.isVoid()) paramsContainerData = scriptParamsContainer.getJSONData();

	buildEnvironment();
	Result result = scriptEngine->execute(s);


	if (result.getErrorMessage().isEmpty())
	{
		NLOG("Script : " + niceName, "Script loaded succesfully");
		scriptParamsContainer.loadJSONData(paramsContainerData); //keep overriden values
		paramsContainerData = var();
		setState(SCRIPT_LOADED);
	} else
	{
		NLOG("Script : " + niceName, "Script error : " + result.getErrorMessage());
		setState(SCRIPT_ERROR);
		return;
	}

	const NamedValueSet props = scriptEngine->getRootObjectProperties();
	updateEnabled = props.contains(updateIdentifier);
	if (updateEnabled)
	{
		lastUpdateTime = (float)Time::getMillisecondCounter() / 1000.f;
		startTimer(0, 20); //50fps, should be parametrable
	}

	scriptParamsContainer.hideInEditor = scriptParamsContainer.controllables.size() == 0;

}

void Script::buildEnvironment()
{
	//clear phase
	setState(SCRIPT_CLEAR);

	scriptEngine = new JavascriptEngine();
	while (scriptParamsContainer.controllables.size() > 0) scriptParamsContainer.removeControllable(scriptParamsContainer.controllables[0]);
	scriptParamsContainer.clear();

	scriptEngine->registerNativeObject("script", getScriptObject()); //force "script" for this objet
	if (parentTarget != nullptr) scriptEngine->registerNativeObject("local", parentTarget->getScriptObject()); //force "local" for the related object
	if (Engine::mainEngine != nullptr) scriptEngine->registerNativeObject(Engine::mainEngine->scriptTargetName, Engine::mainEngine->getScriptObject());
	if (ScriptUtil::getInstanceWithoutCreating() != nullptr) scriptEngine->registerNativeObject(ScriptUtil::getInstance()->scriptTargetName, ScriptUtil::getInstance()->getScriptObject());
}

void Script::setState(ScriptState newState)
{
	state = newState;
	scriptAsyncNotifier.addMessage(new ScriptEvent(ScriptEvent::STATE_CHANGE));
}

var Script::callFunction(const Identifier & function, const Array<var> args, Result  * result)
{
	if (canBeDisabled && !enabled->boolValue()) return false;

	if (scriptEngine == nullptr) return false;

	Result tmpResult = Result::ok();
	if (result == nullptr) result = &tmpResult;
	var returnData = scriptEngine->callFunction(function, var::NativeFunctionArgs(var::undefined(), (const var *)args.begin(), args.size()), result);
	if (result->getErrorMessage().isNotEmpty())
	{
		NLOG(niceName, "Script Error :\n" + result->getErrorMessage());
		return var();
	}

	return returnData;
}


void Script::onContainerParameterChangedInternal(Parameter * p)
{
	if (p == filePath) loadScript();
}

void Script::onContainerTriggerTriggered(Trigger * t)
{
	if (t == reload)
	{
		loadScript();
	}
}

void Script::onControllableFeedbackUpdateInternal(ControllableContainer * cc, Controllable * c)
{
	if (cc == &scriptParamsContainer)
	{
		Array<var> args;
		args.add(c->getScriptObject());

		callFunction("scriptParamChanged", args);
	}
}

var Script::getJSONData()
{
	var data = BaseItem::getJSONData();
	var pData = scriptParamsContainer.getJSONData();
	if (!pData.isVoid()) data.getDynamicObject()->setProperty("scriptParams", pData);
	return data;
}

void Script::loadJSONDataInternal(var data)
{
	BaseItem::loadJSONDataInternal(data);
	paramsContainerData = data.getProperty("scriptParams", var());
}

void Script::endLoadFile()
{
	Engine::mainEngine->removeEngineListener(this);
	loadScript();
}

InspectableEditor * Script::getEditor(bool isRoot)
{
	return new ScriptEditor(this, isRoot);
}

void Script::timerCallback(int timerID)
{
	switch (timerID) // update timer
	{
	case 0:
	{
		float curTime = (float)Time::getMillisecondCounter() / 1000.f;
		float deltaTime = curTime - lastUpdateTime;
		lastUpdateTime = curTime;

		Array<var> args;
		args.add(deltaTime);

		Result r = Result::ok();
		callFunction(updateIdentifier, args, &r);
		if (r != Result::ok()) stopTimer(0);
	}
	break;

	case 1:
	{
		File f = filePath->getFile();
		if (f.getLastModificationTime() != fileLastModTime)
		{
			loadScript();
		}
	}
	break;
	}
}

var Script::logFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = ScriptTarget::getObjectFromJS<Script>(args);
	if (!s->logParam->boolValue()) return var();

	for (int i = 0; i < args.numArguments; i++)
	{
		if (i == 0) NLOG("Script : " + s->niceName, args.arguments[i].toString());
		else NLOG("", args.arguments[i].toString());
	}

	return var();
}

var Script::addBoolParameterFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = getObjectFromJS<Script>(args);
	if (!checkNumArgs(s->niceName, args, 3)) return var();
	return s->scriptParamsContainer.addBoolParameter(args.arguments[0], args.arguments[1], (bool)args.arguments[2])->getScriptObject();
}

var Script::addIntParameterFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = getObjectFromJS<Script>(args);
	if (!checkNumArgs(s->niceName, args, 5)) return var();
	return s->scriptParamsContainer.addIntParameter(args.arguments[0], args.arguments[1], (int)args.arguments[2], (int)args.arguments[3], (int)args.arguments[4])->getScriptObject();
}

var Script::addFloatParameterFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = getObjectFromJS<Script>(args);
	if (!checkNumArgs(s->niceName, args, 5)) return var();
	return s->scriptParamsContainer.addFloatParameter(args.arguments[0], args.arguments[1], (float)args.arguments[2], (float)args.arguments[3], (float)args.arguments[4])->getScriptObject();
}

var Script::addStringParameterFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = getObjectFromJS<Script>(args);
	if (!checkNumArgs(s->niceName, args, 3)) return var();
	return s->scriptParamsContainer.addStringParameter(args.arguments[0], args.arguments[1], args.arguments[2])->getScriptObject();
}

var Script::addEnumParameterFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = getObjectFromJS<Script>(args);
	if (!checkNumArgs(s->niceName, args, 2)) return var();
	return s->scriptParamsContainer.addEnumParameter(args.arguments[0], args.arguments[1])->getScriptObject();
}

var Script::addTargetParameterFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = getObjectFromJS<Script>(args);
	if (!checkNumArgs(s->niceName, args, 2)) return var();
	return s->scriptParamsContainer.addTargetParameter(args.arguments[0], args.arguments[1])->getScriptObject();
}

var Script::addColorParameterFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = getObjectFromJS<Script>(args);
	if (!checkNumArgs(s->niceName, args, 3)) return var();
	return s->scriptParamsContainer.addColorParameter(args.arguments[0], args.arguments[1], Colour((int)(args.arguments[2])))->getScriptObject();
}

var Script::addTriggerFromScript(const var::NativeFunctionArgs & args)
{
	Script * s = getObjectFromJS<Script>(args);
	if (!checkNumArgs(s->niceName, args, 2)) return var();
	return s->scriptParamsContainer.addTrigger(args.arguments[0], args.arguments[1])->getScriptObject();
}

bool Script::checkNumArgs(const String &logName, const var::NativeFunctionArgs & args, int expectedArgs)
{
	if (args.numArguments != expectedArgs)
	{
		NLOG(logName, "Error, function takes " + String(expectedArgs) + " arguments, got " + String(args.numArguments));
		if (args.numArguments > 0) NLOG("", "When tying to add : " + args.arguments[0].toString());
		return false;
	}

	return true;
}

#include "FileParameter.h"
/*
  ==============================================================================

    StringParameter.cpp
    Created: 9 Mar 2016 12:29:30am
    Author:  bkupe

  ==============================================================================
*/


FileParameter::FileParameter(const String & niceName, const String &description, const String & initialValue, bool enabled) :
    StringParameter(niceName, description, initialValue, enabled),
	forceRelativePath(false)
{
	defaultUI = FILE; 
	if(Engine::mainEngine != nullptr) Engine::mainEngine->addEngineListener(this);
}

FileParameter::~FileParameter()
{
	if(Engine::mainEngine != nullptr) Engine::mainEngine->removeEngineListener(this);
}

void FileParameter::setValueInternal(var &newVal)
{
	StringParameter::setValueInternal(newVal);

	if (newVal.toString().isNotEmpty())
	{
		if(File::isAbsolutePath(newVal.toString())) absolutePath = newVal.toString();
		else absolutePath = Engine::mainEngine->getFile().getParentDirectory().getChildFile(value.toString()).getFullPathName();

		File f = File::createFileWithoutCheckingPath(absolutePath);
		if (f.existsAsFile() && (isRelativePath(newVal.toString()) || forceRelativePath))
		{
			value = File(absolutePath).getRelativePathFrom(Engine::mainEngine->getFile().getParentDirectory());
		}
	}
}

void FileParameter::setForceRelativePath(bool force)
{
	forceRelativePath = force;
	setValue(absolutePath, false, true);
}

bool FileParameter::isRelativePath(const String & p)
{
	if (p.isEmpty()) return false;
	if (File::isAbsolutePath(p)) return !File(p).getRelativePathFrom(Engine::mainEngine->getFile().getParentDirectory()).contains("..");
	return Engine::mainEngine->getFile().getParentDirectory().getChildFile(p).exists();
}

String FileParameter::getAbsolutePath() const
{
	if (value.toString().isEmpty()) return "";
	if (File::isAbsolutePath(value.toString())) return value.toString();
	return Engine::mainEngine->getFile().getParentDirectory().getChildFile(value.toString()).getFullPathName();
}

File FileParameter::getFile()
{
	String p = getAbsolutePath();
	if (p.isEmpty()) return File();

	return File(p); 
}

var FileParameter::getJSONDataInternal()
{
	var data = StringParameter::getJSONDataInternal();
	if(forceRelativePath) data.getDynamicObject()->setProperty("relative", true);
	return data;
}

void FileParameter::loadJSONDataInternal(var data)
{
	StringParameter::loadJSONDataInternal(data);
	setForceRelativePath(data.getProperty("relative", false));
}

void FileParameter::fileSaved(bool savedAs)
{
	if(savedAs) setValue(absolutePath, false, true); //force re-evaluate relative path if changed
}

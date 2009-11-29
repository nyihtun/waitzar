/*
 * Copyright 2009 by Seth N. Hetu
 *
 * Please refer to the end of the file for licensing information
 */


#include "ConfigManager.h"

using std::vector;
using std::pair;
using std::wstring;
using json_spirit::wValue;
using json_spirit::wObject;
using json_spirit::wPair;
using json_spirit::Value_type;


ConfigManager::ConfigManager(void){}

ConfigManager::~ConfigManager(void){}


/**
 * Load our maing config file:
 *   config/config.txt
 *
 * This file usually only contains SETTINGS (not languages, etc.) but for the
 *   purose of brevity, we can actually load the entire configuration (inc.
 *   languages, keyboards, etc.) here.
 * This is the only config. file that is actually required.
 */
void ConfigManager::initMainConfig(const std::string& configFile)
{
	//Save the file, we will load it later when we need it
	this->mainConfig = configFile;
}


/**
 * Load all config files for a language:
 *   config/Myanmar/
 *                 config.txt
 *                 ZgInput/config.txt
 *                 StdTransformers/config.txt
 *                 Burglish/config.txt
 *   ...etc.
 *
 * These files may not contain general SETTINGS. The first config file (directly in the Language
 *   folder) must contain basic language information (like the id). All other config files must be in
 *   immediate sub-directories; beyond that, the structure is arbitrary. For example, we load "Burglish"
 *   and "Zawgyi Input" in separate folders, but we load a series of "Standard Transformers" all from
 *   one directory.
 * These files are optional, but heavily encouraged in all distributions except the Web Demo.
 */
void ConfigManager::initAddLanguage(const std::string& configFile, const std::vector<std::string>& subConfigFiles)
{
	//Convert std::strings to JsonFiles
	std::vector<JsonFile> cfgs;
	for (size_t i=0; i<subConfigFiles.size(); i++)
		cfgs.push_back(subConfigFiles[i]);

	//Save the file, we will load it later when we need it
	this->langConfigs[JsonFile(configFile)] = cfgs;
}


/**
 * Load the application-maintained settings override file:
 *   %USERPROFILE%\AppData\Local\WaitZar\config.override.txt
 *   (actually, calls SHGetKnownFolderPath(FOLDERID_LocalAppData) \ WaitZar\config.override.txt)
 *
 * This json config file contains one single array of name-value pairs. The names are fully-qualified:
 *   "language.myanmar.defaultdisplayencoding" = "zawgyi-one", for example. These override all 
 *   WaitZar config options where applicable.
 * WaitZar's GUI config window will alter this file. Loading it is optional, but it's generally a good idea
 *  (otherwise, users' settings won't get loaded when they exit and re-load WaitZar).
 */
void ConfigManager::initLocalConfig(const std::string& configFile)
{
	//Save the file, we will load it later when we need it
	this->localConfig = configFile;
}


/**
 * Load the user-maintained settings override file:
 *   %USERPROFILE%\Documents\waitzar.config.txt
 *   (actually, calls SHGetKnownFolderPath(FOLDERID_Documents) \ waitzar.config.txt)
 *
 * This json config file contains one single array of name-value pairs. The names are fully-qualified:
 *   "settings.showballoon" = "false", for example. These override all of WaitZar's config options, 
 *   AND they override the settings in LocalConfig (see initAddLocalConfig). They can contain any
 *   options, but are really only intended only to contain settings overrides (not language overrides, etc.)
 * This is the file that users will tweak on their own. It is HIGHLY recommended to load this file, if it exists.
 */
void ConfigManager::initUserConfig(const std::string& configFile)
{
	//Save the file, we will load it later when we need it
	this->userConfig = configFile;
}


Settings ConfigManager::getSettings() const 
{
	//We need at least one config file to parse.
	if (this->mainConfig == NULL)
		throw std::exception("No main config file defined.");

	//Parse each config file in turn.
	//First: main config
	this->readInConfig(this->mainConfig.root, L"", WRITE_MAIN);

	//TODO:Others
	
}


void ConfigManager::readInConfig(wValue root, wstring context, WRITE_OPTS writeTo) 
{
	//We always operate on maps:
	wObject currPairs = root.get_value<Object>();
	for (std::iterator<Pair> itr=currPairs.begin(); itr!=currPairs.end(); itr++) {
 		//Construct the new context
		wstring newContext = context + "." + sanitize_id(itr->name_);

		//React to this option/category
		if (itr->value_.type()==obj_type) {
			//Inductive case: Continue reading all options under this type
			this->readInConfig(itr->value, newContext, writeTo);
		} else if (itr->value_.type()==str_type) {
			//Base case: the "value" is also a string (set the property)
			this->setSingleOption(newContext, sanitize(itr->value_.get_value(std::wstring));
		} else {
			throw std::exception("ERROR: Config file options should always be string types.");
		}
	}
}


void ConfigManager::setSingleOption(const std::wstring& name, const std::wstring& value)
{
	//Read each "context" setting from left to right. Context settings are separated by periods. 
	//   Note: There are much faster/better ways of doing this, but for now we'll keep all the code
	//   centralized and easy to read.
	try {
		wstring opt = L"settings.";
		if (name.find(opt)==0) {
			if (name.find(L"hotkey.", opt.size())==0)
				options.settings.hotkey = sanitize_id(value);
			else if (name.find(sanitize_id("silence-mywords-errors."), opt.size())==0)
				options.settings.silenceMywordsErrors = read_bool(value);
			else if (name.find(sanitize_id("balloon-start."), opt.size())==0)
				options.settings.balloonStart = read_bool(value);
			else if (name.find(sanitize_id("always-elevate."), opt.size())==0)
				options.settings.alwaysElevate = read_bool(value);
			else if (name.find(sanitize_id("track-caret."), opt.size())==0)
				options.settings.trackCaret = read_bool(value);
			else if (name.find(sanitize_id("lock-windows."), opt.size())==0)
				options.settings.lockWindows = read_bool(value);
			else 
				throw 1;
			return;
		} else if (name.find(L"languages.")==0) {
			//TODO: Options for each language.


		} else
			error = true;
	} catch (int) {
		//Bad option
		throw std::exception("Invalid option: \"" + name + "\", with value: \"" + value + "\"");
	}
}




//Remove leading and trailing whitespace
wstring ConfigManager::sanitize(const wstring& str) 
{
	size_t firstLetter = find_first_not_of(" \t\n", 0);
	size_t lastLetter = find_last_not_of(" \t\n", firstLetter);
	return str.substr(firstLetter, lastLetter-firstLetter+1);
}

//Sanitize, then return in lowercase, with '-', '_', and whitespace removed
wstring ConfigManager::sanitize_id(const wstring& str) 
{
	return loc_to_lower(wstring(str.begin(), std::remove_if(str.begin(), str.end(), is_id_delim)));
}

bool ConfigManager::read_bool(const std::wstring& str)
{
	std::wstring test = loc_to_lower(str);
	if (test == L"yes" or test==L"true")
		return true;
	else if (test==L"no" or test==L"false")
		return false;
	else
		throw std::exception("Bad boolean value: \"" + str + "\"");
}

std::wstring ConfigManager::loc_to_lower(const std::wstring& str)
{
	//Locale-aware "toLower" converter
	std::locale loc(""); //Get native locale
	return std::transform(str.begin(),str.end(),str.begin(),ToLower<wchar_t>(loc));
}


//Not yet defined:
vector<wstring> ConfigManager::getLanguages() const {}
vector<wstring> ConfigManager::getInputManagers() const {}
vector<wstring> ConfigManager::getEncodings() const {}
wstring ConfigManager::getActiveLanguage() const {}
void ConfigManager::changeActiveLanguage(const wstring& newLanguage) {}



/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "debug.h"
#include "inputmanager.h"
#include "utilities.h"

#include <iostream>
#include <fstream>

using namespace std;

InputManager::InputManager() {
	initJoysticks();
}


InputManager::~InputManager() {
	for (uint x=0; x<joysticks.size(); x++)
		if(SDL_JoystickOpened(x))
			SDL_JoystickClose(joysticks[x]);
}


void InputManager::init(const string &conffile) {
	if (!readConfFile(conffile))
		ERROR("InputManager initialization from config file failed.\n");
}


void InputManager::initJoysticks() {
	SDL_JoystickEventState(SDL_IGNORE);

	int numJoy = SDL_NumJoysticks();
	for (int x=0; x<numJoy; x++) {
		SDL_Joystick *joy = SDL_JoystickOpen(x);
		if (joy) {
			INFO("Initialized joystick: '%s'\n", SDL_JoystickName(x));
			joysticks.push_back(joy);
		}
		else WARNING("Failed to initialize joystick: %i\n", x);
	}
}


bool InputManager::readConfFile(const string &conffile) {
	setActionsCount(18);

	if (!fileExists(conffile)) {
		ERROR("File not found: %s\n", conffile.c_str());
		return false;
	}

	ifstream inf(conffile.c_str(), ios_base::in);
	if (!inf.is_open()) {
		ERROR("Could not open %s\n", conffile.c_str());
		return false;
	}

	int action, linenum = 0;
	string line, name, value;
	string::size_type pos;
	vector<string> values;

	while (getline(inf, line, '\n')) {
		linenum++;
		pos = line.find("=");
		name = trim(line.substr(0,pos));
		value = trim(line.substr(pos+1,line.length()));

		if (name=="up")                action = UP;
		else if (name=="down")         action = DOWN;
		else if (name=="left")         action = LEFT;
		else if (name=="right")        action = RIGHT;
		else if (name=="modifier")     action = MODIFIER;
		else if (name=="confirm")      action = CONFIRM;
		else if (name=="cancel")       action = CANCEL;
		else if (name=="manual")       action = MANUAL;
		else if (name=="dec")          action = DEC;
		else if (name=="inc")          action = INC;
		else if (name=="section_prev") action = SECTION_PREV;
		else if (name=="section_next") action = SECTION_NEXT;
		else if (name=="pageup")       action = PAGEUP;
		else if (name=="pagedown")     action = PAGEDOWN;
		else if (name=="settings")     action = SETTINGS;
		else if (name=="menu")         action = MENU;
		else if (name=="volup")        action = VOLUP;
		else if (name=="voldown")      action = VOLDOWN;
		else {
			ERROR("%s:%d Unknown action '%s'.\n", conffile.c_str(), linenum, name.c_str());
			return false;
		}

		split(values, value, ",");
		if (values.size() >= 2) {

			if (values[0] == "joystickbutton" && values.size()==3) {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_BUTTON;
				map.num = atoi(values[1].c_str());
				map.value = atoi(values[2].c_str());
				map.treshold = 0;
				mappings[action].push_back(map);
			} else if (values[0] == "joystickaxys" && values.size()==4) {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_AXYS;
				map.num = atoi(values[1].c_str());
				map.value = atoi(values[2].c_str());
				map.treshold = atoi(values[3].c_str());
				mappings[action].push_back(map);
			} else if (values[0] == "keyboard") {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_KEYPRESS;
				map.value = atoi(values[1].c_str());
				mappings[action].push_back(map);
			} else {
				ERROR("%s:%d Invalid syntax or unsupported mapping type '%s'.\n", conffile.c_str(), linenum, value.c_str());
				return false;
			}

		} else {
			ERROR("%s:%d Every definition must have at least 2 values (%s).\n", conffile.c_str(), linenum, value.c_str());
			return false;
		}
	}
	inf.close();
	return true;
}


void InputManager::setActionsCount(int count) {
	actions.clear();
	actionTick.clear();
	interval.clear();
	for (int x=0; x<count; x++) {
		actions.push_back(false);
		actionTick.push_back(0);
		interval.push_back(0);
		MappingList maplist;
		mappings.push_back(maplist);
	}
}


bool InputManager::update(bool wait) {
	bool anyactions = false;
	SDL_JoystickUpdate();
	
	events.clear();
	SDL_Event event;
	if (wait) {
		SDL_WaitEvent(&event);
		SDL_Event evcopy = event;
		events.push_back(event);
	}
	while (SDL_PollEvent(&event)) {
		SDL_Event evcopy = event;
		events.push_back(evcopy);
	}
	
	Uint32 tick = SDL_GetTicks();
	for (uint x=0; x<actions.size(); x++) {
		actions[x] = false;
		if (isActive(x)) {
			if (tick-actionTick[x]>interval[x]) {
				actions[x] = true;
				actionTick[x] = tick;
				anyactions = true;
			}
		} else {
			actionTick[x] = 0;
		}
	}
	return anyactions;
}


int InputManager::count() {
	return actions.size();
}


void InputManager::setInterval(int ms, int action) {
	if (action<0)
		for (uint x=0; x<interval.size(); x++)
			interval[x] = ms;
	else if ((uint)action < interval.size())
		interval[action] = ms;
}


bool InputManager::operator[](int action) {
	if (action<0 || (uint)action>=actions.size()) return false;
	return actions[action];
}


bool InputManager::isActive(int action) {
	MappingList mapList = mappings[action];
	for (MappingList::const_iterator it = mapList.begin(); it !=mapList.end(); ++it) {
		InputMap map = *it;

		switch (map.type) {
			case InputManager::MAPPING_TYPE_BUTTON:
				if (map.num < joysticks.size() && SDL_JoystickGetButton(joysticks[map.num], map.value))
					return true;
			break;
			case InputManager::MAPPING_TYPE_AXYS:
				if (map.num < joysticks.size()) {
					int axyspos = SDL_JoystickGetAxis(joysticks[map.num], map.value);
					if (map.treshold<0 && axyspos < map.treshold) return true;
					if (map.treshold>0 && axyspos > map.treshold) return true;
				}
			break;
			case InputManager::MAPPING_TYPE_KEYPRESS:
				for (uint ex=0; ex<events.size(); ex++) {
					INFO("KEYPRESS: %d\n", events[ex].key.keysym.sym);
					if (events[ex].type == SDL_KEYDOWN && events[ex].key.keysym.sym == map.value)
						return true;
				}
			break;
		}
	}

	return false;
}

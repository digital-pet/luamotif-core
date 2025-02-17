/*
 * This file is part of the luamotif-core distribution (https://github.com/digital-pet/luamotif-core).
 * Copyright (c) 2022 digital-pet.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */

#include "../include/common.h"
#include "../include/utils.h"
#include "../include/callbacks.h"


#include "GenericWidget.h"
#include "GenericWidget_P.h"


Widget ConstructGenericWidget(lua_State* L, int parentObj, Widget wdgParent, const char* pszWidgetName, MotifWidget WidgetFunction) {
	Arg* aCreationArgs;
	Widget wdgWidget = NULL;
	XmString* axmsValues;
	lua_Integer iArgCount = 0, iXmStringCount = 0, iLuaTableID;
	char* pszKey, * pszValue;
	lua_Integer *piValue, * pbValue;

	struct cb_data* cbdCallback;

	// Gather arguments by looping over the table
	iLuaTableID = lua_gettop(L);
	lua_pushnil(L);


	aCreationArgs = (Arg*)GC_MALLOC(MAXARGS * sizeof(Arg));
	axmsValues = (XmString*)GC_MALLOC(MAXARGS * sizeof(XmString));
	if (axmsValues == NULL) {
		luaL_error(L, "memory allocation failed");
	}

	while (lua_next(L, iLuaTableID) != 0) {

		switch (lua_type(L, -2)) {
		case LUA_TSTRING:
			break;

		default:
			lua_pop(L, 1);
			continue;
		}

		pszKey = gc_strdup(lua_tostring(L, -2));
		if (pszKey == NULL) {
			luaL_error(L, "memory allocation failed");
		}

		switch (lua_type(L, -1)) {
		case LUA_TSTRING:
			pszValue = gc_strdup(lua_tostring(L, -1));
			if (pszValue == NULL) {
				luaL_error(L, "memory allocation failed");
			}


			if (!strcmp(pszKey, "value") || !strcmp(pszKey, "title")) {
				XtSetArg(aCreationArgs[iArgCount], pszKey, pszValue);
			}
			else {
				axmsValues[iXmStringCount] = XmStringCreateLocalized(pszValue);
				XtSetArg(aCreationArgs[iArgCount], pszKey, axmsValues[iXmStringCount]);
				iXmStringCount++;
			}
			iArgCount++;
			break;

		case LUA_TNUMBER:
			piValue = (lua_Integer*)GC_MALLOC(sizeof(lua_Integer));
			*piValue = lua_tointeger(L, -1);

			XtSetArg(aCreationArgs[iArgCount], pszKey, *piValue);
			iArgCount++;
			break;

		case LUA_TBOOLEAN:
			pbValue = (lua_Integer*)GC_MALLOC(sizeof(lua_Integer));
			*pbValue = lua_toboolean(L, -1);

			XtSetArg(aCreationArgs[iArgCount], pszKey, *pbValue);
			iArgCount++;
			break; 
		}
		lua_pop(L, 1);
	}

	// Create widget using function pointer with creation arguments



	wdgWidget = (*WidgetFunction)(wdgParent, (String)pszWidgetName, aCreationArgs, iArgCount);

	// Deallocate XmStrings which require deallocation
	if (iXmStringCount > 0) {
		while (--iXmStringCount >= 0) {
			XmStringFree(axmsValues[iXmStringCount]);
		}
	}


	if (wdgWidget == NULL) {
		luaL_error(L, "failed to create widget");
		// TODO: pass some sort of error up the chain
	}

	lua_pushlightuserdata(L, wdgWidget);
	lua_setfield(L, -2, "__widget");
	luaL_getmetatable(L, WIDGET_METATABLE);
	lua_setmetatable(L, -2);

	// Apply post-creation arguments
	iLuaTableID = lua_gettop(L);
	lua_pushnil(L);

	while (lua_next(L, iLuaTableID) != 0) {
		switch (lua_type(L, -2)) {
		case LUA_TSTRING:
			break;

		default:
			lua_pop(L, 1);
			continue;
		}
		pszKey = gc_strdup(lua_tostring(L, -2));
		// colors would be set here but meh I'll do that later.

		switch (lua_type(L, -1)) {
		case LUA_TFUNCTION:
			cbdCallback = GC_MALLOC(sizeof(struct cb_data));
			if (cbdCallback == NULL) {
				luaL_error(L, "memory allocation failed");
			}

			cbdCallback->L = L;
			lua_pushvalue(L, -1);

			cbdCallback->ref = luaL_ref(L, LUA_REGISTRYINDEX);
			lua_pushvalue(L, iLuaTableID);

			cbdCallback->obj = luaL_ref(L, LUA_REGISTRYINDEX);
			cbdCallback->callback_name = gc_strdup(pszKey);

			XtAddCallback(wdgWidget, pszKey, lm_Callback, cbdCallback);
			XtAddCallback(wdgWidget, XmNdestroyCallback, lm_DestroyCallback, cbdCallback);
			break;
		}
		lua_pop(L, 1);

	}


	// Return widget
	return wdgWidget;
}

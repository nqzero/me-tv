/*
 * Copyright (C) 2011 Michael Lamothe
 *
 * This file is part of Me TV
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __DEVICE_MANAGER_H__
#define __DEVICE_MANAGER_H__

#include <glibmm.h>
#include <giomm.h>
#include "dvb_frontend.h"

typedef std::list<Dvb::Frontend*> FrontendList;

class DeviceManager
{
private:
	FrontendList frontends;	

	static String get_adapter_path(guint adapter);
	static String get_frontend_path(guint adapter, guint frontend);
	static gboolean is_frontend_supported(const Dvb::Frontend& frontend);

public:
	void initialise(const String& devices);
		
	Dvb::Frontend* find_frontend_by_path(const String& path);
	FrontendList& get_frontends() { return frontends; };

	void check_frontend();
};

#endif

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

#ifndef __CONFIGURATION_MANAGER_H__
#define __CONFIGURATION_MANAGER_H__

#include <gconfmm.h>
#include "../common/me-tv-types.h"

class ConfigurationManager
{
private:
	Glib::RefPtr<Gnome::Conf::Client>	gconf_client;

	String get_path(const String& key);

public:
	void initialise();
	
	void set_string_default(const String& key, const String& value);
	void set_int_default(const String& key, gint value);
	void set_boolean_default(const String& key, gboolean value);	
	
	StringList		get_string_list_value(const String& key);
	String	get_string_value(const String& key);
	gint			get_int_value(const String& key);
	gboolean		get_boolean_value(const String& key);

	void set_string_list_value(const String& key, const StringList& value);
	void set_string_value(const String& key, const String& value);
	void set_int_value(const String& key, gint value);
	void set_boolean_value(const String& key, gboolean value);
};

extern ConfigurationManager configuration_manager;

#endif

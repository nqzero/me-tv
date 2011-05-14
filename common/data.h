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

#ifndef __DATA_H__
#define __DATA_H__

#include <list>
#include <libgdamm.h>
#include "me-tv-types.h"

using namespace Gnome::Gda;

class Data
{
public:
	static String get(Glib::RefPtr<DataModelIter>& iter, const String& column)
	{
		return iter->get_value_for_field(column).get_string();
	}
	
	static int get_int(Glib::RefPtr<DataModelIter>& iter, const String& column)
	{
		return iter->get_value_for_field(column).get_int();
	}

	static String get_scalar(const String& table, const String& field, const String& where);
	static String get_scalar(const String& table, const String& field, const String& where_field, const String& where_value);
	
	static Glib::RefPtr<Connection> create_connection();
};

#endif

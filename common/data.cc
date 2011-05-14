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

#include "data.h"
#include "exception.h"
#include "i18n.h"
#include "common.h"
#include <giomm.h>

#define CURRENT_DATABASE_VERSION	9

Glib::RefPtr<Connection> Data::create_connection()
{
	String data_directory = Glib::get_home_dir() + "/.local/share/me-tv";
	make_directory_with_parents (data_directory);

	String database_filename = Glib::build_filename(data_directory, "me-tv.db");

	gboolean database_exists = false;
	Glib::RefPtr<Gio::File> database_file = Gio::File::create_for_path(database_filename);
	if (database_file->query_exists())
	{
		database_exists = true;
		g_debug("Database exists");
	}
	
	g_debug("Opening Me TV database");

	Glib::RefPtr<Connection> connection = Connection::open_from_string("sqlite", String::compose("DB_DIR=%1;DB_NAME=me-tv",
		data_directory));

	if (!database_exists)
	{
		g_debug("Creating database schema");
		connection->statement_execute("CREATE TABLE auto_record (id INTEGER PRIMARY KEY AUTOINCREMENT, title CHAR(200) NOT NULL, priority INTEGER NOT NULL, UNIQUE (title, priority));");
		connection->statement_execute("CREATE TABLE channel ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT, name CHAR(50) NOT NULL, type INTEGER NOT NULL, sort_order INTEGER NOT NULL, mrl CHAR(1024), service_id INTEGER, frequency INTEGER, inversion INTEGER,"
			"bandwidth INTEGER, code_rate_hp INTEGER, code_rate_lp INTEGER, constellation INTEGER, transmission_mode INTEGER,"
            "guard_interval INTEGER, hierarchy_information INTEGER, symbol_rate INTEGER, fec_inner INTEGER, modulation INTEGER,"
		    "polarisation INTEGER, record_extra_before INTEGER, record_extra_after INTEGER, UNIQUE (name));");
		connection->statement_execute("CREATE TABLE configuration (id INTEGER PRIMARY KEY AUTOINCREMENT, name CHAR(200) NOT NULL, value CHAR(1024) NOT NULL, UNIQUE (name));");
		connection->statement_execute("CREATE TABLE epg_event (id INTEGER PRIMARY KEY AUTOINCREMENT, channel_id INTEGER NOT NULL, version_number INTEGER NOT NULL, event_id INTEGER NOT NULL, start_time INTEGER NOT NULL, duration INTEGER NOT NULL, UNIQUE (channel_id, event_id));");
		connection->statement_execute("CREATE TABLE epg_event_text (id INTEGER PRIMARY KEY AUTOINCREMENT, epg_event_id INTEGER NOT NULL, language CHAR(3) NOT NULL, title CHAR(200) NOT NULL, subtitle CHAR(200) NOT NULL, description CHAR(1000) NOT NULL, UNIQUE (epg_event_id, language));");
		connection->statement_execute("CREATE TABLE scheduled_recording (id INTEGER PRIMARY KEY AUTOINCREMENT, description CHAR(200) NOT NULL, recurring_type INTEGER NOT NULL, action_after INTEGER NOT NULL, channel_id INTEGER NOT NULL, start_time INTEGER NOT NULL, duration INTEGER NOT NULL, device CHAR(200) NOT NULL);");
		connection->statement_execute("CREATE TABLE version (value INTEGER NOT NULL);");
		g_debug("Database schema created");

		connection->statement_execute(String::compose("insert into version values (%1);", CURRENT_DATABASE_VERSION));
	}

	Glib::RefPtr<DataModel> version = connection->statement_execute_select("select value from version;");
	Glib::RefPtr<DataModelIter> iter = version->create_iter();
	iter->move_next();
	int version_value = Data::get_int(iter, "value");
	
	g_debug("Required database version: %d", CURRENT_DATABASE_VERSION);
	g_debug("Actual database version: %d", version_value);	

	if (version_value != CURRENT_DATABASE_VERSION)
	{
		throw Exception("Me TV database version does not match");
	}
	
	return connection;
}

String Data::get_scalar(const String& table, const String& field, const String& where_field, const String& where_value)
{
	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
		String::compose("select %1 from %2 where %3 = '%4'", field, table, where_field, where_value));
	Glib::RefPtr<DataModelIter> iter = model->create_iter();

	String result;
	if (iter->move_next())
	{
		result = get(iter, "value");
	}
	
	return result;
}


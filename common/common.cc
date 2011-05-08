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

#include "common.h"
#include "i18n.h"
#include "exception.h"
#include <glib/gprintf.h>

#define BLOCK_SIZE	1024
#define MAX_BUFFER_SIZE	1000 * BLOCK_SIZE

ChannelManager				channel_manager;
ScheduledRecordingManager	scheduled_recording_manager;
DeviceManager				device_manager;
StreamManager				stream_manager;
Glib::RefPtr<Connection>	data_connection;

sigc::signal<void>			signal_update;
sigc::signal<void, String>	signal_error;
sigc::signal<void>			signal_quit;

String		preferred_language;
bool		verbose_logging = false;
bool		disable_epg_thread = false;
String		devices;
gboolean	ignore_teletext = true;
String		recording_directory;
int			read_timeout = 5000;
String		broadcast_address;
String		text_encoding;

void replace_text(String& text, const String& from, const String& to)
{
	String::size_type position = 0;
	while ((position = text.find(from, position)) != String::npos)
	{
		text.replace(position, from.size(), to);
		position += to.size();
	}
}

String get_local_time_text(const gchar* format)
{
	return get_local_time_text(time(NULL), format);
}

String get_local_time_text(time_t t, const gchar* format)
{
	struct tm tp;
	char buffer[100];

	if (localtime_r(&t, &tp) == NULL)
	{
		throw Exception(_("Failed to get time"));
	}
	strftime(buffer, 100, format, &tp);
	
	return buffer;
}

String encode_xml(const String& s)
{
	String result = s;
	
	replace_text(result, "&", "&amp;");
	replace_text(result, "<", "&lt;");
	replace_text(result, ">", "&gt;");
	replace_text(result, "'", "&apos;");
	replace_text(result, "\"", "&quot;");
	replace_text(result, "©", "(C)");

	return result;
}

String trim_string(const String& s)
{
	String result;
	
	glong length = s.bytes();
	if (length > 0)
	{
		gchar buffer[length + 1];
		s.copy(buffer, length);
		buffer[length] = 0;
		result = g_strstrip(buffer);
	}
	return result;
}

void write_string(int fd, const String& data)
{
	gsize length = data.bytes();
	const gchar* buffer = data.c_str();
	guint bytes_written = 0;

	if (length >= MAX_BUFFER_SIZE)
	{
		throw Exception("Refusing to write data: buffer exceeded");
	}

	g_debug("Writing '%s'", buffer);
	while (bytes_written < length)
	{
		guint bytes_to_write = BLOCK_SIZE;
		if (bytes_written + BLOCK_SIZE > length)
		{
			bytes_to_write = length - bytes_written;
		}

		gint result = ::write(fd, buffer + bytes_written, bytes_to_write); 

		if (result < 0)
		{
			throw SystemException("Failed to write data");
		}

		bytes_written += result;
	}	
	g_debug("Data written");
}

String read_string(int fd)
{
	gint read_result;
	guint total_bytes_read = 0;
	gchar buffer[MAX_BUFFER_SIZE];

	g_debug("Reading data");
	while ((read_result = ::read(fd, buffer + total_bytes_read, BLOCK_SIZE)) > 0)
	{
		if (read_result < 0)
		{
			throw SystemException("Failed to read data");
		}
	
		total_bytes_read += read_result;

		if (total_bytes_read + BLOCK_SIZE > MAX_BUFFER_SIZE)
		{
			throw Exception("Buffer exceeded");
		}
	} 
	buffer[total_bytes_read] = 0;
	g_debug("Read '%s'", buffer);

	return buffer;
}

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	if (log_level != G_LOG_LEVEL_DEBUG || verbose_logging)
	{
		String time_text = get_local_time_text("%F %T");
		g_printf("%s: %s\n", time_text.c_str(), message);
	}
}

void on_error(const String& message)
{
	g_message("Error: %s", message.c_str());
}

void handle_error()
{
	try
	{
		throw;
	}
	catch (const Exception& exception)
	{
		g_debug("Signal error '%s'", exception.what().c_str());
		signal_error(exception.what());
	}
	catch (const Glib::Error& exception)
	{
		g_debug("Signal error '%s'", exception.what().c_str());
		signal_error(exception.what());
	}
	catch (...)
	{
		g_message("Unhandled exception");
		signal_error("Unhandled exception");
	}
}

void make_directory_with_parents(const String& path)
{
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
        if (!file->query_exists())
        {
                Glib::RefPtr<Gio::File> parent = file->get_parent();
                if (!parent->query_exists())
                {
                        make_directory_with_parents(parent->get_path());
                }

                g_debug("Creating directory '%s'", path.c_str());
                file->make_directory();
        }
}


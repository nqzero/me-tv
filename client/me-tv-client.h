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

#ifndef __ME_TV_CLIENT_H__
#define __ME_TV_CLIENT_H__

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gtkmm.h>
#include "configuration_manager.h"
#include "../common/client.h"
#include <X11/Xlib.h> // Must be after gtkmm.h and ../common/client.h (due to libxml++)
#include "../common/common.h"
#include "../common/i18n.h"
#include "../common/exception.h"
#include "../common/me-tv-types.h"

extern bool							verbose_logging;
extern bool							safe_mode;
extern bool							minimised_mode;
extern bool							disable_epg_thread;
extern bool							disable_epg;
extern bool							no_screensaver_inhibit;
extern bool							quit_on_close;
extern String						devices;
extern gint							read_timeout;
extern String						engine_type;
extern String						preferred_language;
extern guint						record_extra_before;
extern guint						record_extra_after;
extern Glib::RefPtr<Gtk::UIManager>	ui_manager;
extern Client						client;

void replace_text(String& text, const String& from, const String& to);
String encode_xml(const String& s);
String trim_string(const String& s);

String get_local_time_text(const gchar* format);
String get_local_time_text(time_t t, const gchar* format);

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);

extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_fullscreen;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_mute;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_record_current;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_visibility;

extern Glib::RefPtr<Gtk::Action> action_about;
extern Glib::RefPtr<Gtk::Action> action_auto_record;
extern Glib::RefPtr<Gtk::Action> action_channels;
extern Glib::RefPtr<Gtk::Action> action_change_view_mode;
extern Glib::RefPtr<Gtk::Action> action_decrease_volume;
extern Glib::RefPtr<Gtk::Action> action_epg_event_search;
extern Glib::RefPtr<Gtk::Action> action_increase_volume;
extern Glib::RefPtr<Gtk::Action> action_preferences;
extern Glib::RefPtr<Gtk::Action> action_present;
extern Glib::RefPtr<Gtk::Action> action_quit;
extern Glib::RefPtr<Gtk::Action> action_scheduled_recordings;

extern sigc::signal<void, int> signal_start_rtsp;
extern sigc::signal<void> signal_stop_rtsp;
extern sigc::signal<void, int> signal_add_scheduled_recording;
extern sigc::signal<void, int> signal_remove_scheduled_recording;

// This class exists because I can't get Gtk::ComboBoxText to work properly
// it seems to have 2 columns
class ComboBoxText : public Gtk::ComboBox
{
private:
	class ModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_text);
			add(column_value);
		}

		Gtk::TreeModelColumn<String> column_text;
		Gtk::TreeModelColumn<String> column_value;
	};
	
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
	
public:
	ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml);
	
	void clear_items();
	void append_text(const String& text, const String& value = "");
	void set_active_text(const String& text);
	void set_active_value(const String& value);
	String get_active_text();
	String get_active_value();
};

class IntComboBox : public Gtk::ComboBox
{
private:
	class ModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_int);
		}

		Gtk::TreeModelColumn<guint> column_int;
	};
	
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
		
public:
	IntComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml);
	void set_size(guint size);
	guint get_size();
	guint get_active_value();
};

class ComboBoxEntryText : public Gtk::ComboBoxEntryText
{
public:
	ComboBoxEntryText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml);
};

class GdkLock
{
public:
	GdkLock();
	~GdkLock();
};

class GdkUnlock
{
public:
	GdkUnlock();
	~GdkUnlock();
};

#endif

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

#ifndef __ME_TV_UI_H__
#define __ME_TV_UI_H__

#include <gtkmm.h>
#include "me-tv.h"

extern Glib::RefPtr<Gtk::UIManager>	ui_manager;

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

		Gtk::TreeModelColumn<Glib::ustring> column_text;
		Gtk::TreeModelColumn<Glib::ustring> column_value;
	};
	
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
	
public:
	ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml);
	
	void clear_items();
	void append_text(const Glib::ustring& text, const Glib::ustring& value = "");
	void set_active_text(const Glib::ustring& text);
	void set_active_value(const Glib::ustring& value);
	Glib::ustring get_active_text();
	Glib::ustring get_active_value();
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

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

#include <gtkmm.h>
#include <gdk/gdk.h>

#include "me-tv-ui.h"
#include "../common/i18n.h"
#include "../common/exception.h"

Glib::RefPtr<Gtk::UIManager> ui_manager;
bool quit_on_close;

ComboBoxText::ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBox(cobject)
{
	list_store = Gtk::ListStore::create(columns);
	clear();
	set_model(list_store);
	pack_start(columns.column_text);
	set_active(0);
}

void ComboBoxText::append_text(const Glib::ustring& text, const Glib::ustring& value)
{
	Gtk::TreeModel::Row row = *list_store->append();
	row[columns.column_text] = text;
	row[columns.column_value] = value;
}

void ComboBoxText::set_active_text(const Glib::ustring& text)
{
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); i++)
	{
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_text] == text)
		{
			set_active(i);
		}
	}
}

void ComboBoxText::set_active_value(const Glib::ustring& value)
{
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); i++)
	{
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_value] == value)
		{
			set_active(i);
		}
	}
}

void ComboBoxText::clear_items()
{
	list_store->clear();
}

Glib::ustring ComboBoxText::get_active_text()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_text];
		}
	}
	
	throw Exception(_("Failed to get active text value"));
}

Glib::ustring ComboBoxText::get_active_value()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_value];
		}
	}
	
	throw Exception(_("Failed to get active text value"));
}

ComboBoxEntryText::ComboBoxEntryText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBoxEntryText(cobject)
{
}

GdkLock::GdkLock()
{
	gdk_threads_enter();
}

GdkLock::~GdkLock()
{
	gdk_threads_leave();
}

GdkUnlock::GdkUnlock()
{
	gdk_threads_leave();
}

GdkUnlock::~GdkUnlock()
{
	gdk_threads_enter();
}

IntComboBox::IntComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBox(cobject)
{
	list_store = Gtk::ListStore::create(columns);
	clear();
	set_model(list_store);
	pack_start(columns.column_int);
	set_size(0);
	set_active(0);
}

void IntComboBox::set_size(guint size)
{	
	g_debug("Setting integer combo box size to %d", size);

	list_store->clear();
	for (guint i = 0; i < size; i++)
	{
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_int] = (i+1);
		
		if (list_store->children().size() == 1)
		{
			set_active(0);
		}
	}
	
	if (size == 0)
	{
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_int] = 1;
	}
	
	set_sensitive(size > 1);
}

guint IntComboBox::get_size()
{
	return list_store->children().size();
}

guint IntComboBox::get_active_value()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_int];
		}
	}
	
	throw Exception(_("Failed to get active integer value"));
}

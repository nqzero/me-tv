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

#ifndef __EPG_EVENT_SEARCH_DIALOG_H__
#define __EPG_EVENT_SEARCH_DIALOG_H__

#include "me-tv-ui.h"

class EpgEventSearchDialog : public Gtk::Dialog
{
private:
	class SearchModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		SearchModelColumns()
		{
			add(column_text);
		}

		Gtk::TreeModelColumn<Glib::ustring> column_text;
	};
	
	typedef Glib::RefPtr<Gdk::Pixbuf> PixbufPtr;

	class ResultsModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		ResultsModelColumns()
		{
			add(column_id);
			add(column_title);
			add(column_channel);
			add(column_channel_name);
			add(column_start_time);
			add(column_start_time_text);
			add(column_duration);
			add(column_image);
			add(column_scheduled_recording_id);
		}

		Gtk::TreeModelColumn<guint>			column_id;
		Gtk::TreeModelColumn<Glib::ustring>	column_title;
		Gtk::TreeModelColumn<guint>			column_channel;
		Gtk::TreeModelColumn<Glib::ustring>	column_channel_name;
		Gtk::TreeModelColumn<guint>			column_start_time;
		Gtk::TreeModelColumn<Glib::ustring>	column_start_time_text;
		Gtk::TreeModelColumn<Glib::ustring>	column_duration;
		Gtk::TreeModelColumn<PixbufPtr>		column_image;
		Gtk::TreeModelColumn<gint>		column_scheduled_recording_id;
	};

	SearchModelColumns					search_columns;
	ResultsModelColumns					results_columns;
	Glib::RefPtr<Gtk::ListStore>		list_store_results;
	Glib::RefPtr<Gtk::ListStore>		list_store_search;
	PixbufPtr							pixbuf_record;
	const Glib::RefPtr<Gtk::Builder>	builder;
	Gtk::ComboBoxEntry*					combo_box_entry_search;
	Gtk::TreeView*						tree_view_epg_event_search;
	Gtk::Image							image_record;

	void on_show();
	void on_row_activated(const Gtk::TreeModel::Path& tree_model_path, Gtk::TreeViewColumn* column);
	void on_event_search_button_press_event(GdkEventButton* button);
	void add_completion(const Glib::ustring& text);
	void search();
public:
	EpgEventSearchDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

	static EpgEventSearchDialog& get(Glib::RefPtr<Gtk::Builder> builder);
};

#endif

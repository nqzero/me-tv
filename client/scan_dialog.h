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

#ifndef __SCAN_DIALOG_H__
#define __SCAN_DIALOG_H__

#include <gtkmm.h>
#include "me-tv-ui.h"

#ifndef SCAN_DIRECTORIES
#define SCAN_DIRECTORIES "/usr/share/dvb:/usr/share/doc/dvb-utils/examples/scan:/usr/share/dvb-apps"
#endif

class ScanDialog : public Gtk::Window
{
private:
	const Glib::RefPtr<Gtk::Builder>	builder;
	Gtk::Notebook*						notebook_scan_wizard;
	Gtk::ProgressBar*					progress_bar_scan;
	Gtk::TreeView*						tree_view_scanned_channels;
	StringList							system_files;
	guint								channel_count;
	Glib::ustring						scan_directory_path;

	class ModelColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_id);
			add(column_name);
			add(column_signal_strength);
		}

		Gtk::TreeModelColumn<guint>								column_id;
		Gtk::TreeModelColumn<Glib::ustring>						column_name;
		Gtk::TreeModelColumn<Glib::ustring>							column_signal_strength;
	};

	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
	
	void on_button_scan_wizard_next_clicked();
	void on_button_scan_wizard_cancel_clicked();
	void on_button_scan_wizard_add_clicked();
	void on_button_scan_stop_clicked();
	void on_file_chooser_button_scan_file_set();
	void on_file_chooser_button_import_file_set();
	void on_combo_box_auto_scan_range_changed();
	void on_hide();	
	void stop_scan();
	void update_channel_count();

	void import_channels_conf(const Glib::ustring& channels_conf_path);

	void on_show();

public:
	ScanDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
	~ScanDialog();
		
	static ScanDialog& create(Glib::RefPtr<Gtk::Builder> builder);
};

#endif

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

#include "preferences_dialog.h"
#include "me-tv-ui.h"
#include "configuration_manager.h"
#include "../common/common.h"

PreferencesDialog& PreferencesDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	PreferencesDialog* preferences_dialog = NULL;
	builder->get_widget_derived("dialog_preferences", preferences_dialog);
	return *preferences_dialog;
}

PreferencesDialog::PreferencesDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
	: Gtk::Dialog(cobject), builder(builder)
{
}

void PreferencesDialog::run()
{
	Gtk::FileChooserButton* file_chooser_button_recording_directory = NULL;
	Gtk::SpinButton* spin_button_epg_span_hours = NULL;
	Gtk::SpinButton* spin_button_epg_page_size = NULL;
	Gtk::CheckButton* check_button_keep_above = NULL;
	Gtk::CheckButton* check_button_show_epg_header = NULL;
	Gtk::CheckButton* check_button_show_epg_time = NULL;
	Gtk::CheckButton* check_button_show_epg_tooltips = NULL;
	Gtk::CheckButton* check_button_show_channel_number = NULL;

	builder->get_widget("file_chooser_button_recording_directory", file_chooser_button_recording_directory);
	builder->get_widget("spin_button_epg_span_hours", spin_button_epg_span_hours);
	builder->get_widget("spin_button_epg_page_size", spin_button_epg_page_size);
	builder->get_widget("check_button_keep_above", check_button_keep_above);
	builder->get_widget("check_button_show_epg_header", check_button_show_epg_header);
	builder->get_widget("check_button_show_epg_time", check_button_show_epg_time);
	builder->get_widget("check_button_show_epg_tooltips", check_button_show_epg_tooltips);
	builder->get_widget("check_button_show_channel_number", check_button_show_channel_number);
	
	Client::ConfigurationMap configuration = client.get_configuration();
	file_chooser_button_recording_directory->set_filename(configuration["recording_directory"]);
	spin_button_epg_span_hours->set_value(configuration_manager.get_int_value("epg_span_hours"));
	spin_button_epg_page_size->set_value(configuration_manager.get_int_value("epg_page_size"));
	check_button_keep_above->set_active(configuration_manager.get_boolean_value("keep_above"));
	check_button_show_epg_header->set_active(configuration_manager.get_boolean_value("show_epg_header"));
	check_button_show_epg_time->set_active(configuration_manager.get_boolean_value("show_epg_time"));
	check_button_show_epg_tooltips->set_active(configuration_manager.get_boolean_value("show_epg_tooltips"));
	check_button_show_channel_number->set_active(configuration_manager.get_boolean_value("show_channel_number"));
	
	if (Dialog::run() == Gtk::RESPONSE_OK)
	{
		configuration_manager.set_int_value("epg_span_hours", (int)spin_button_epg_span_hours->get_value());
		configuration_manager.set_int_value("epg_page_size", (int)spin_button_epg_page_size->get_value());
		configuration_manager.set_boolean_value("keep_above", check_button_keep_above->get_active());
		configuration_manager.set_boolean_value("show_epg_header", check_button_show_epg_header->get_active());
		configuration_manager.set_boolean_value("show_epg_time", check_button_show_epg_time->get_active());
		configuration_manager.set_boolean_value("show_epg_tooltips", check_button_show_epg_tooltips->get_active());
		configuration_manager.set_boolean_value("show_channel_number", check_button_show_channel_number->get_active());

		configuration["recording_directory"] = file_chooser_button_recording_directory->get_filename();
		client.set_configuration(configuration);
		
		signal_update();
	}
}

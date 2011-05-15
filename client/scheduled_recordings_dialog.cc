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

#include "me-tv-client.h"
#include "scheduled_recordings_dialog.h"

ScheduledRecordingsDialog& ScheduledRecordingsDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	ScheduledRecordingsDialog* scheduled_recordings_dialog = NULL;
	builder->get_widget_derived("dialog_scheduled_recordings", scheduled_recordings_dialog);
	return *scheduled_recordings_dialog;
}

ScheduledRecordingsDialog::ScheduledRecordingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder)
{
	Gtk::Button* button = NULL;
	
	builder->get_widget("button_scheduled_recordings_delete", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScheduledRecordingsDialog::on_button_scheduled_recordings_delete_clicked));

	builder->get_widget("tree_view_scheduled_recordings", tree_view_scheduled_recordings);
	list_store = Gtk::ListStore::create(columns);
	tree_view_scheduled_recordings->set_model(list_store);
 	tree_view_scheduled_recordings->append_column(_("Description"), columns.column_description);
	tree_view_scheduled_recordings->append_column(_("Channel"), columns.column_channel);
	tree_view_scheduled_recordings->append_column(_("Start Time"), columns.column_start_time);
	tree_view_scheduled_recordings->append_column(_("Duration"), columns.column_duration);
	tree_view_scheduled_recordings->append_column(_("Record"),columns.column_recurring_type);
	tree_view_scheduled_recordings->append_column(_("After"),columns.column_action_after);
	tree_view_scheduled_recordings->append_column(_("Device"), columns.column_device);
	
	list_store->set_sort_column(columns.column_sort, Gtk::SORT_ASCENDING);
}

Client::ScheduledRecording ScheduledRecordingsDialog::get_selected_scheduled_recording()
{
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scheduled_recordings->get_selection();	
	if (selection->count_selected_rows() == 0)
	{
		throw Exception(_("No scheduled recording selected"));
	}
	
	Gtk::TreeModel::Row row = *(selection->get_selected());

	return row[columns.column_scheduled_recording];
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_delete_clicked()
{
	Gtk::MessageDialog dialog(*this,
		"Are you sure that you want to delete this scheduled recording?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
	dialog.set_title(PACKAGE_NAME);
	if (dialog.run() == Gtk::RESPONSE_YES)
	{
		Client::ScheduledRecording scheduled_recording = get_selected_scheduled_recording();
		client.remove_scheduled_recording(scheduled_recording.id);
		update();
	}
}

void ScheduledRecordingsDialog::update()
{
	list_store->clear();
	Client::ChannelList channels = client.get_channels();
	Client::ScheduledRecordingList scheduled_recordings = client.get_scheduled_recordings();
	for (Client::ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		Client::ScheduledRecording& scheduled_recording = *i;
		Gtk::TreeModel::Row row = *(list_store->append());
		row[columns.column_sort]					= scheduled_recording.start_time;
		row[columns.column_scheduled_recording]		= scheduled_recording;
		row[columns.column_description]				= scheduled_recording.description;
		row[columns.column_channel]					= channels.get_by_id(scheduled_recording.channel_id).name;
		row[columns.column_start_time]				= scheduled_recording.get_start_time_text();
		row[columns.column_duration]				= scheduled_recording.get_duration_text();
		row[columns.column_device]					= scheduled_recording.device;
		switch(scheduled_recording.recurring_type)
		{
			case 1 : row[columns.column_recurring_type] = "Every day"; break;
			case 2 : row[columns.column_recurring_type] = "Every week"; break;
			case 3 : row[columns.column_recurring_type] = "Every weekday"; break;
			default: row[columns.column_recurring_type] = "Once"; break;
		}
	}
}

void ScheduledRecordingsDialog::on_show()
{
	update();
	Widget::on_show();
}


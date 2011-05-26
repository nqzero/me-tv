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

#include "epg_event_dialog.h"
#include "me-tv-client.h"

EpgEventDialog& EpgEventDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	EpgEventDialog* dialog_epg_event = NULL;
	builder->get_widget_derived("dialog_epg_event", dialog_epg_event);
	return *dialog_epg_event;
}

EpgEventDialog::EpgEventDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
	: Gtk::Dialog(cobject), builder(builder)
{
}

void EpgEventDialog::show_epg_event(Client::EpgEvent& epg_event)
{
	time_t end_time = epg_event.start_time + epg_event.duration;
	time_t now = time(NULL);
	
	String information = String::compose(
	    	"<b>%1</b>\n<b><i>%2</i></b>\n<i>%4 (%5)</i>\n\n%3",
	    	encode_xml(epg_event.title),
	    	epg_event.description.empty() ? "" : encode_xml(epg_event.subtitle),
	    	epg_event.description.empty() ? encode_xml(epg_event.subtitle) : encode_xml(epg_event.description),
		    epg_event.get_start_time_text() + " - " + get_local_time_text(end_time, "%H:%M"),
	    	epg_event.get_duration_text());
	
	Gtk::Label* label_epg_event_information = NULL;
	builder->get_widget("label_epg_event_information", label_epg_event_information);
	label_epg_event_information->set_label(information);

	gboolean is_scheduled = epg_event.scheduled_recording_id >= 0;
	Gtk::HBox* hbox_epg_event_dialog_scheduled = NULL;
	builder->get_widget("hbox_epg_event_dialog_scheduled", hbox_epg_event_dialog_scheduled);
	hbox_epg_event_dialog_scheduled->property_visible() = is_scheduled;

	Gtk::Button* button_epg_event_dialog_record = NULL;
	builder->get_widget("button_epg_event_dialog_record", button_epg_event_dialog_record);
	button_epg_event_dialog_record->property_visible() = !is_scheduled;

	Gtk::Button* button_epg_event_dialog_watch_now = NULL;
	builder->get_widget("button_epg_event_dialog_watch_now", button_epg_event_dialog_watch_now);
	button_epg_event_dialog_watch_now->property_visible() = epg_event.start_time <= now && now <= end_time;

	Gtk::Button* button_epg_event_dialog_view_schedule = NULL;
	builder->get_widget("button_epg_event_dialog_view_schedule", button_epg_event_dialog_view_schedule);
	button_epg_event_dialog_view_schedule->property_visible() = is_scheduled;

	gint result = run();
	hide();

	switch(result)
	{
	case 0: // Close
		break;
			
	case 1: // Record
		client.add_scheduled_recording(epg_event.id);
		signal_update();
		break;

	case 2: // Record
		action_scheduled_recordings->activate();
		break;
			
	case 3: // Watch Now
		signal_start_rtsp(epg_event.channel_id);
		break;

	default:
		break;
	}	
}


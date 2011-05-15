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

#include "scheduled_recording_manager.h"
#include "exception.h"
#include "i18n.h"
#include "epg_events.h"
#include "common.h"

gboolean ScheduledRecordingManager::is_device_available(const String& device, const ScheduledRecording& scheduled_recording)
{
	Channel channel = ChannelManager::get(scheduled_recording.channel_id);

	ScheduledRecordingList scheduled_recordings = get_all();
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& current = *i;

		Channel current_channel = ChannelManager::get(current.channel_id);

		if (
			current.id != scheduled_recording.id &&
			channel.transponder != current_channel.transponder &&
			scheduled_recording.overlaps(current) &&
			device == current.device
		    )
		{
			g_debug("Frontend '%s' is busy recording '%s'", device.c_str(), scheduled_recording.description.c_str());
			return false;
		}
	}

	g_debug("Found available frontend '%s'", device.c_str());

	return true;
}

void ScheduledRecordingManager::select_device(ScheduledRecording& scheduled_recording)
{
	g_debug("Looking for an available device for scheduled recording");
	
	Channel channel = ChannelManager::get(scheduled_recording.channel_id);

	FrontendList& frontends = device_manager.get_frontends();
	for (FrontendList::iterator j = frontends.begin(); j != frontends.end(); j++)
	{
		Dvb::Frontend* device = (*j);
		const String& device_path = device->get_path();

		if (device->get_frontend_type() != channel.transponder.frontend_type)
		{
			g_debug("Device %s is the wrong type", device_path.c_str());
		}
		else
		{
			if (is_device_available(device_path, scheduled_recording))
			{
				scheduled_recording.device = device_path;
				if (!stream_manager.is_broadcasting(device_path))
				{
					return;
				}
				g_debug("Device '%s' is currently broadcasting, looking for something better", device_path.c_str());
			}
		}
	}
}

void ScheduledRecordingManager::add_scheduled_recording(EpgEvent& epg_event)
{
	ScheduledRecording scheduled_recording;
	Channel channel = ChannelManager::get(epg_event.channel_id);
	
	int before = channel.record_extra_before;
	int after = channel.record_extra_after;
	
	scheduled_recording.channel_id		= epg_event.channel_id;
	scheduled_recording.description		= epg_event.get_title();
	scheduled_recording.recurring_type	= SCHEDULED_RECORDING_RECURRING_TYPE_ONCE;
	scheduled_recording.action_after	= SCHEDULED_RECORDING_ACTION_AFTER_NONE;
	scheduled_recording.start_time		= epg_event.start_time - (before * 60);
 	scheduled_recording.duration		= epg_event.duration + ((before + after) * 60);	
	scheduled_recording.device			= "";

	add_scheduled_recording(scheduled_recording);
}

void ScheduledRecordingManager::add_scheduled_recording(ScheduledRecording& scheduled_recording)
{
	ScheduledRecordingList::iterator iupdated;
	gboolean updated  = false;
	gboolean is_same  = false;
	gboolean conflict = false;

	g_debug("Setting scheduled recording");
	
	Channel channel = ChannelManager::get(scheduled_recording.channel_id);

	if (scheduled_recording.device.empty())
	{
		select_device(scheduled_recording);

		if (scheduled_recording.device.empty())
		{
			String message = String::compose(_(
				"Failed to set scheduled recording for '%1' at %2: There are no devices available at that time"),
				scheduled_recording.description, scheduled_recording.get_start_time_text());
			throw Exception(message);
		}

		g_debug("Device selected: '%s'", scheduled_recording.device.c_str());
	}

	ScheduledRecordingList scheduled_recordings = get_all();
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& current = *i;

		Channel current_channel = ChannelManager::get(current.channel_id);

		// Check for conflict
		if (current.id != scheduled_recording.id &&
		    current_channel.transponder != channel.transponder &&
		    scheduled_recording.device == current.device &&
		    scheduled_recording.overlaps(current))
		{
			conflict = true;
			String message =  String::compose(
				_("Failed to save scheduled recording because it conflicts with another scheduled recording called '%1'."),
				current.description);
			throw Exception(message);
		}

		// Check if it's an existing scheduled recording
		if (scheduled_recording.id != 0 &&
		    scheduled_recording.id == current.id)
		{
			g_debug("Updating scheduled recording");
			updated = true;
			iupdated = i;
		}

		// Check if we are scheduling the same program
		if (scheduled_recording.id == 0 &&
		    current.channel_id 		== scheduled_recording.channel_id &&
		    current.start_time 		== scheduled_recording.start_time &&
		    current.duration 		== scheduled_recording.duration)
		{
			String message =  String::compose(
				_("Failed to save scheduled recording because you have already have a scheduled recording called '%1' which is scheduled for the same time on the same channel."),
				current.description);
			throw Exception(message);
		}

		if (current.id == scheduled_recording.id &&
			current.recurring_type	== scheduled_recording.recurring_type &&
			current.action_after	== scheduled_recording.action_after &&
			current.channel_id 		== scheduled_recording.channel_id &&
			current.start_time 		== scheduled_recording.start_time &&
			current.duration 		== scheduled_recording.duration)
		{
			is_same = true;
		}
	}
	
	// If there is an update an not conflict on scheduled recording, update it.
	if (updated && !conflict && !is_same)
	{
		ScheduledRecording& current = *iupdated;

		current.device = scheduled_recording.device;
		current.recurring_type = scheduled_recording.recurring_type;
		current.action_after = scheduled_recording.action_after;
		current.description = scheduled_recording.description;
		current.channel_id = scheduled_recording.channel_id;
		current.start_time = scheduled_recording.start_time;
		current.duration = scheduled_recording.duration;
	}
	
	// If the scheduled recording is new then add it
	if (scheduled_recording.id == 0)
	{
		g_debug("Adding scheduled recording");

		Glib::RefPtr<SqlBuilder> builder = SqlBuilder::create(SQL_STATEMENT_INSERT);
		builder->set_table("scheduled_recording");
		
		builder->add_field_value("channel_id", scheduled_recording.channel_id);
		builder->add_field_value("description", scheduled_recording.description);
		builder->add_field_value("recurring_type", scheduled_recording.recurring_type);
		builder->add_field_value("action_after", scheduled_recording.action_after);
		builder->add_field_value("start_time", (int)scheduled_recording.start_time);
		builder->add_field_value("duration", scheduled_recording.duration);
		builder->add_field_value("device", scheduled_recording.device);

		data_connection->statement_execute_non_select_builder(builder);
	}
}

void ScheduledRecordingManager::remove_scheduled_recording(guint scheduled_recording_id)
{
	g_debug("Deleting scheduled recording %d", scheduled_recording_id);
	data_connection->statement_execute_non_select(String::compose("delete from scheduled_recording where id = %1", scheduled_recording_id));
	g_debug("Scheduled recording deleted");
}

void ScheduledRecordingManager::remove_scheduled_recording(Channel& channel)
{
	g_debug("Deleting scheduled recordings for channel %d", channel.id);
	data_connection->statement_execute_non_select(String::compose("delete from scheduled_recording where channel_id = %1", channel.id));
	g_debug("Scheduled recordings deleted");
}

void ScheduledRecordingManager::check_scheduled_recordings()
{
	ScheduledRecordingList active_scheduled_recordings;
	
	time_t now = time(NULL);

	g_debug("Removing scheduled recordings older than %u", (guint)now);
	data_connection->statement_execute_non_select(String::compose("delete from scheduled_recording where start_time+duration < %1", now));

	ScheduledRecordingList scheduled_recordings = get_all();
	if (!scheduled_recordings.empty())
	{
		Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
			"select * from scheduled_recording order by start_time;");
		String dump = model->dump_as_string();
		g_debug("\n%s", dump.c_str());

		for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
		{			
			ScheduledRecording& scheduled_recording = *i;
				
			if (scheduled_recording.contains(now))
			{
				active_scheduled_recordings.push_back(scheduled_recording);
			}
		}
	}
		
	for (ScheduledRecordingList::iterator i = active_scheduled_recordings.begin(); i != active_scheduled_recordings.end(); i++)
	{
		const ScheduledRecording& scheduled_recording = *i;
		stream_manager.start_recording(scheduled_recording);
	}

	gboolean check = true;
	while (check)
	{
		check = false;

		FrontendThreadList& frontend_threads = stream_manager.get_frontend_threads();
		for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
		{
			FrontendThread& frontend_thread = **i;
			ChannelStreamList& streams = frontend_thread.get_streams();
			for (ChannelStreamList::iterator j = streams.begin(); j != streams.end(); j++)
			{
				ChannelStream& channel_stream = **j;
				guint scheduled_recording_id = is_recording(channel_stream.channel);
				if (channel_stream.type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING && scheduled_recording_id == -1)
				{
					stream_manager.stop_recording(channel_stream.channel);
					check = true;
					break;
				}
			}
		}
	}
}

void ScheduledRecordingManager::load(Glib::RefPtr<DataModelIter>& iter, ScheduledRecording& scheduled_recording)
{
	scheduled_recording.id				= Data::get_int(iter, "id");
	scheduled_recording.channel_id		= Data::get_int(iter, "channel_id");
	scheduled_recording.description		= Data::get(iter, "description");
	scheduled_recording.recurring_type	= Data::get_int(iter, "recurring_type");
	scheduled_recording.action_after	= Data::get_int(iter, "action_after");
	scheduled_recording.start_time		= Data::get_int(iter, "start_time");
	scheduled_recording.duration		= Data::get_int(iter, "duration");
	scheduled_recording.device			= Data::get(iter, "device");
}

ScheduledRecordingList ScheduledRecordingManager::get_all()
{
	ScheduledRecordingList scheduled_recordings;
	
	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
		String::compose("select * from scheduled_recording where ((start_time + duration) > %1 OR recurring_type != 0)", time(NULL)));
	Glib::RefPtr<DataModelIter> iter = model->create_iter();
	
	while (iter->move_next())
	{
		ScheduledRecording scheduled_recording;
		load(iter, scheduled_recording);
		scheduled_recordings.push_back(scheduled_recording);
	}

	return scheduled_recordings;
}

ScheduledRecording ScheduledRecordingManager::get(guint scheduled_recording_id)
{
	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
		String::compose("select * from scheduled_recording where id = %1", scheduled_recording_id));
	Glib::RefPtr<DataModelIter> iter = model->create_iter();
	
	if (!iter->move_next())
	{
		String message = String::compose(
			_("Scheduled recording '%1' not found"), scheduled_recording_id);
		throw Exception(message);
	}

	ScheduledRecording scheduled_recording;
	load(iter, scheduled_recording);
	
	return scheduled_recording;
}

gint ScheduledRecordingManager::is_recording(const Channel& channel)
{
	ScheduledRecordingList scheduled_recordings = get_all();

	guint now = time(NULL);
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{			
		ScheduledRecording& scheduled_recording = *i;
		if (scheduled_recording.contains(now) && scheduled_recording.channel_id == channel.id)
		{
			return scheduled_recording.id;
		}
	}

	return -1;
}

gint ScheduledRecordingManager::is_recording(const EpgEvent& epg_event)
{
	ScheduledRecordingList scheduled_recordings = get_all();
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& scheduled_recording = *i;
		if (scheduled_recording.channel_id == epg_event.channel_id &&
			scheduled_recording.contains(epg_event.start_time, epg_event.get_end_time()))
		{
			return scheduled_recording.id;
		}
	}
	
	return -1;
}

void ScheduledRecordingManager::check_auto_recordings()
{
	g_debug("Checking auto recordings");

	Glib::RefPtr<DataModel> model_auto_record = data_connection->statement_execute_select(
		"select title from auto_record order by priority");
	Glib::RefPtr<DataModelIter> iter_auto_record = model_auto_record->create_iter();

	while (iter_auto_record->move_next())
	{
		String title = Data::get(iter_auto_record, "title");

		g_debug("Searching for '%s'", title.c_str());

		replace_text(title, "'", "''");
		
		Glib::RefPtr<DataModel> model = data_connection->statement_execute_select (
			String::compose(
				"select * from epg_event ee, epg_event_text eet "
				"where ee.id = eet.epg_event_id "
				"and title like '%%%1%%' order by start_time", title));

		Glib::RefPtr<DataModelIter> iter = model->create_iter();

		while (iter->move_next())
		{
			EpgEvent epg_event;
			EpgEvents::load(iter, epg_event);

			String title = epg_event.get_title();
			g_debug("Checking candidate: %s at %d", title.c_str(), (int)epg_event.start_time);
			gint scheduled_recording_id = is_recording(epg_event);
			if (scheduled_recording_id != -1)
			{
				g_debug("EPG event '%s' at %d is already being recorded",
					title.c_str(), (int)epg_event.start_time);
			}
			else
			{
				try
				{
					g_debug("Trying to auto record '%s' (%d)", title.c_str(), epg_event.event_id);
					add_scheduled_recording(epg_event);
				}
				catch(...)
				{
					handle_error();
				}
			}
		}
	}
	g_debug("Finished checking for auto recordings");
}


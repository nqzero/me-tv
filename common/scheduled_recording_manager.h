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

#ifndef __SCHEDULED_RECORDING_MANAGER_H__
#define __SCHEDULED_RECORDING_MANAGER_H__

#include "scheduled_recording.h"
#include "data.h"
#include "channel.h"
#include "epg_event.h"

typedef std::list<ScheduledRecording> ScheduledRecordingList;

class ScheduledRecordingManager
{
private:
	static gboolean is_device_available(const Glib::ustring& device, const ScheduledRecording& scheduled_recording);
	static void select_device(ScheduledRecording& scheduled_recording);
	static void load(Glib::RefPtr<DataModelIter>& iter, ScheduledRecording& scheduled_recording);

public:

	static void check_auto_recordings();
	static void check_scheduled_recordings();

	static void add_scheduled_recording(EpgEvent& epg_event);
	static void add_scheduled_recording(ScheduledRecording& scheduled_recording);
	static void remove_scheduled_recording(guint scheduled_recording_id);
	static void remove_scheduled_recording(Channel& channel);
	static ScheduledRecording get(guint scheduled_recording_id);
	static ScheduledRecordingList get_all();
	static gint is_recording(const Channel& channel);
	static gint is_recording(const EpgEvent& epg_event);
};

#endif

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

#ifndef __STREAM_MANAGER_H__
#define __STREAM_MANAGER_H__

#include "frontend_thread.h"
#include "scheduled_recording.h"
#include "channel_stream.h"

class StreamManager
{
private:
	FrontendThreadList frontend_threads;
	
public:
	void initialise(const String& text_encoding, guint timeout, gboolean ignore_teletext);
	~StreamManager();
			
	FrontendThreadList& get_frontend_threads() { return frontend_threads; };

	void start_rtsp(Channel& channel, int client_id, GstRTSPServer* rtsp_server);
	void stop_rtsp(int client_id);
	gboolean is_rtsp(const String& device);

	void start_recording(const ScheduledRecording& scheduled_recording);
	void stop_recording(const Channel& channel);

	void start();
	void stop();
};

#endif

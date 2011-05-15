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

#ifndef __FRONTEND_THREAD_H__
#define __FRONTEND_THREAD_H__

#include "epg_thread.h"
#include "channel_stream.h"
#include "dvb_frontend.h"

typedef std::list<ChannelStream*> ChannelStreamList;

class FrontendThread : public Thread
{
private:
	ChannelStreamList	streams;
	EpgThread*			epg_thread;
	int					dvr_fd;
	guint				timeout;
	gboolean			ignore_teletext;
		
	void write(Glib::RefPtr<Glib::IOChannel> channel, guchar* buffer, gsize length);
	void run();
	void setup_dvb(ChannelStream& stream);
	void start_epg_thread();
	void stop_epg_thread();

public:
	FrontendThread(Dvb::Frontend& frontend, const String& encoding, guint timeout, gboolean ignore_teletext);
	~FrontendThread();

	Dvb::Frontend& frontend;
	String text_encoding;

	gboolean is_available(const Channel& channel);

	gboolean is_recording(const Channel& channel);
	void start_recording(Channel& channel, const String& description, gboolean scheduled);
	void stop_recording(const Channel& channel);

	gboolean is_broadcasting();
	void start_broadcasting(Channel& channel, int client_id, const String& interface, const String& address, int port);
	void stop_broadcasting(int client_id);

	void start();
	void stop();
	ChannelStreamList& get_streams() { return streams; }
};

typedef std::list<FrontendThread*> FrontendThreadList;

#endif

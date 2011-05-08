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
 
#ifndef __CHANNEL_STREAM_H__
#define __CHANNEL_STREAM_H__

#include "mpeg_stream.h"
#include "dvb_demuxer.h"
#include "channel.h"
#include <giomm.h>
#include <netinet/in.h>

typedef enum
{
	CHANNEL_STREAM_TYPE_NONE = -1,
	CHANNEL_STREAM_TYPE_BROADCAST = 0,
	CHANNEL_STREAM_TYPE_RECORDING = 1,
	CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING = 2
} ChannelStreamType;

class ChannelStream
{
private:
	Glib::StaticRecMutex			mutex;
	guint					last_insert_time;

	virtual void write_data(guchar* buffer, gsize length) = 0;
	
public:
	ChannelStream(ChannelStreamType t, Channel& channel);
	virtual ~ChannelStream()
	{
		g_debug("Destroying channel stream '%s'", channel.name.c_str());
		clear_demuxers();
	};

	Mpeg::Stream		stream;
	Dvb::DemuxerList	demuxers;
	ChannelStreamType	type;
	Channel				channel;

	Dvb::Demuxer& add_pes_demuxer(const Glib::ustring& demux_path,
		guint pid, dmx_pes_type_t pid_type, const gchar* type_text);
	Dvb::Demuxer& add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id);

	void clear_demuxers();
	void write(guchar* buffer, gsize length);
};


class BroadcastingChannelStream : public ChannelStream
{
private:
	int			sd;
        struct sockaddr_in recvaddr;
	Glib::ustring		address;
	Glib::ustring		interface;
	int					port;
	int					client_id;

	void write_data(guchar* buffer, gsize length);

public:
	BroadcastingChannelStream(Channel& channel, int client_id, const Glib::ustring& interface, const Glib::ustring& address, int port);
	~BroadcastingChannelStream();

	int get_client_id() const { return client_id; }
};

class RecordingChannelStream : public ChannelStream
{
private:
	Glib::ustring					mrl;
	Glib::ustring					description;
	Glib::RefPtr<Glib::IOChannel>	output_channel;

	void write_data(guchar* buffer, gsize length);
public:
	RecordingChannelStream(Channel& channel, gboolean scheduled, const Glib::ustring& mrl, const Glib::ustring& description);
	~RecordingChannelStream();
};

#endif


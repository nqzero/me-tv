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
#include "me-tv-types.h"
#include <giomm.h>

typedef enum
{
	CHANNEL_STREAM_TYPE_NONE = -1,
	CHANNEL_STREAM_TYPE_RTSP = 0,
	CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING = 1
} ChannelStreamType;

class ChannelStream
{
private:
	Glib::StaticRecMutex	mutex;
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

	Dvb::Demuxer& add_pes_demuxer(const String& demux_path,
		guint pid, dmx_pes_type_t pid_type, const gchar* type_text);
	Dvb::Demuxer& add_section_demuxer(const String& demux_path, guint pid, guint id);

	void clear_demuxers();
	void write(guchar* buffer, gsize length);

	virtual String get_description() = 0;
};

class RtspChannelStream : public ChannelStream
{
private:
	int								client_id;
	String							fifo_path;
	Glib::RefPtr<Glib::IOChannel>	output_channel;

	void write_data(guchar* buffer, gsize length);
	String get_description();
			
public:
	RtspChannelStream(Channel& channel, int client_id, const String& fifo_path);
	~RtspChannelStream();

	int get_client_id() const { return client_id; }
};

class RecordingChannelStream : public ChannelStream
{
private:
	String							mrl;
	String							description;
	Glib::RefPtr<Glib::IOChannel>	output_channel;

	void write_data(guchar* buffer, gsize length);
	String get_description();

public:
	RecordingChannelStream(Channel& channel, const String& mrl, const String& description);
	~RecordingChannelStream();
};

#endif


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

#include "channel_stream.h"
#include "dvb_si.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../common/exception.h"

class Lock : public Glib::RecMutex::Lock
{
public:
	Lock(Glib::StaticRecMutex& mutex, const Glib::ustring& name) :
		Glib::RecMutex::Lock(mutex) {}
	~Lock() {}
};

ChannelStream::ChannelStream(ChannelStreamType t, Channel& c) : channel(c)
{
	g_static_rec_mutex_init(mutex.gobj());
	type = t;
	last_insert_time = 0;
}

BroadcastingChannelStream::BroadcastingChannelStream(Channel& c, int id, const Glib::ustring& i, const Glib::ustring& a, int p) :
	ChannelStream(CHANNEL_STREAM_TYPE_BROADCAST, c), client_id(id), interface(i), address(a), port(p)
{
	int broadcast = 1;

	sd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sd < 0)
	{
		throw SystemException("Failed to create socket");
	}

	if ((setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast)) == -1)
        {
		throw SystemException("Failed to set broadcasting option");
        }

	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(port);
	recvaddr.sin_addr.s_addr = inet_addr(address.c_str());
	memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);

	g_debug("Added new channel stream '%s' -> '%s:%d'", channel.name.c_str(), address.c_str(), port);
}

RecordingChannelStream::RecordingChannelStream(Channel& c, gboolean scheduled, const Glib::ustring& m, const Glib::ustring& d) :
	ChannelStream(scheduled ? CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING : CHANNEL_STREAM_TYPE_RECORDING, c)
{
	mrl = m;
	description = d;

	g_debug("Added new channel stream '%s' -> '%s'", channel.name.c_str(), mrl.c_str());
}

void ChannelStream::clear_demuxers()
{
	Lock lock(mutex, "ChannelStream::clear_demuxers()");
	
	g_debug("Removing demuxers");
	while (!demuxers.empty())
	{
		Dvb::Demuxer* demuxer = demuxers.front();
		demuxers.pop_front();
		delete demuxer;
		g_debug("Demuxer removed");
	}
}

Dvb::Demuxer& ChannelStream::add_pes_demuxer(const Glib::ustring& demux_path,
	guint pid, dmx_pes_type_t pid_type, const gchar* type_text)
{	
	Lock lock(mutex, "ChannelStream::add_pes_demuxer()");
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	g_debug("Setting %s PID filter to %d (0x%X)", type_text, pid, pid);
	demuxer->set_pes_filter(pid, pid_type);
	return *demuxer;
}

Dvb::Demuxer& ChannelStream::add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id)
{	
	Lock lock(mutex, "FrontendThread::add_section_demuxer()");
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	demuxer->set_filter(pid, id);
	return *demuxer;
}

void BroadcastingChannelStream::write_data(guchar* buffer, gsize length)
{
	if (::sendto(sd, buffer, length, 0, (struct sockaddr*)&recvaddr, sizeof(recvaddr)) < 0)
	{
		throw SystemException("Failed to send data");
	}
}

void RecordingChannelStream::write_data(guchar* buffer, gsize length)
{
	if (!output_channel)
	{
		output_channel = Glib::IOChannel::create_from_file(mrl, "w");
		output_channel->set_encoding("");
		output_channel->set_flags(output_channel->get_flags() & Glib::IO_FLAG_NONBLOCK);
		output_channel->set_buffer_size(TS_PACKET_SIZE * PACKET_BUFFER_SIZE);
	}

	gsize bytes_written = 0;
	output_channel->write((const gchar*)buffer, length, bytes_written);
}

BroadcastingChannelStream::~BroadcastingChannelStream()
{
	::close(sd);
}

RecordingChannelStream::~RecordingChannelStream()
{
	if (output_channel)
	{
		output_channel.reset();
	}
}

void ChannelStream::write(guchar* buffer, gsize length)
{
	try
	{
		time_t now = time(NULL);
		if (now - last_insert_time > 2)
		{
			last_insert_time = now;

			guchar data[TS_PACKET_SIZE];

			stream.build_pat(data);
			write_data(data, TS_PACKET_SIZE);

			stream.build_pmt(data);
			write_data(data, TS_PACKET_SIZE);
		}

		write_data(buffer, length);
	}
	catch(...)
	{
		g_debug("Failed to write");
	}
}


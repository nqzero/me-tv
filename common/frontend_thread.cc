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

#include "frontend_thread.h"
#include "dvb_si.h"
#include "exception.h"
#include "common.h"
#include <sys/stat.h>

FrontendThread::FrontendThread(Dvb::Frontend& f, const String& encoding, guint t, gboolean i)
	: Thread("Frontend"), frontend(f), text_encoding(encoding), timeout(t), ignore_teletext(i)
{
	epg_thread = NULL;
	g_debug("FrontendThread created (%s)", frontend.get_path().c_str());
}

FrontendThread::~FrontendThread()
{
	g_debug("FrontendThread destroyed (%s)", frontend.get_path().c_str());
}

void FrontendThread::start()
{
	if (!streams.empty() && is_terminated())
	{
		g_debug("Starting frontend thread (%s)", frontend.get_path().c_str());
		Thread::start();
	}
}

void FrontendThread::stop()
{
	g_debug("Stopping frontend thread (%s)", frontend.get_path().c_str());
	join(true);
	g_debug("Frontend thread stopped and joined (%s)", frontend.get_path().c_str());
}

void FrontendThread::run()
{
	g_debug("Frontend thread running (%s)", frontend.get_path().c_str());

	String input_path = frontend.get_adapter().get_dvr_path();

	g_debug("Opening dvr device '%s' for reading ...", input_path.c_str());
	if ( (dvr_fd = ::open(input_path.c_str(), O_RDONLY | O_NONBLOCK) ) < 0 )
	{
		throw SystemException("Failed to open dvr device");
	}

	struct pollfd pfds[1];
	pfds[0].fd = dvr_fd;
	pfds[0].events = POLLIN;
	
	guchar buffer[TS_PACKET_SIZE * PACKET_BUFFER_SIZE];

	g_debug("Entering FrontendThread loop (%s)", frontend.get_path().c_str());
	while (!is_terminated())
	{
		if (streams.empty())
		{
			usleep(100000);
			continue;
		}
	
		try
		{
			if (::poll(pfds, 1, 100) < 0)
			{
				throw SystemException("Frontend poll failed");
			}

			gint bytes_read = ::read(dvr_fd, buffer, TS_PACKET_SIZE * PACKET_BUFFER_SIZE);

			if (bytes_read < 0)
			{
				if (errno == EAGAIN)
				{
					continue;
				}

				String message = String::compose("Frontend read failed (%1)", frontend.get_path());
				throw SystemException(message);
			}

			for (guint offset = 0; offset < (guint)bytes_read; offset += TS_PACKET_SIZE)
			{
				guint pid = ((buffer[offset+1] & 0x1f) << 8) + buffer[offset+2];

				for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
				{
					ChannelStream& channel_stream = **i;
					if (channel_stream.stream.contains_pid(pid))
					{
						channel_stream.write(buffer+offset, TS_PACKET_SIZE);
					}
				}
			}
		}
		catch(...)
		{
			// The show must go on!
		}
	}
		
	g_debug("FrontendThread loop exited (%s)", frontend.get_path().c_str());

	stop_epg_thread();

	::close(dvr_fd);
}

void FrontendThread::setup_dvb(ChannelStream& channel_stream)
{
	g_debug("Setting up DVB");

	Buffer buffer;
	const Channel& channel = channel_stream.channel;

	for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
	{
		ChannelStream* existing_channel_stream = *i;

		if (existing_channel_stream->channel.transponder != channel.transponder)
		{
			throw Exception("Failed to add a channel stream on a different transponder");
		}
	}

	String demux_path = frontend.get_adapter().get_demux_path();

	if (streams.empty())
	{
		frontend.open();
		frontend.tune_to(channel.transponder);
		start_epg_thread();
	}
	
	g_debug("Reading PAT");
	Dvb::Demuxer demuxer_pat(demux_path);
	demuxer_pat.set_filter(PAT_PID, PAT_ID);

	Mpeg::Stream& stream = channel_stream.stream;

	demuxer_pat.read_section(buffer);

	stream.clear();
	stream.set_pmt_pid(buffer, channel.service_id);

	demuxer_pat.stop();

	g_debug("Reading PMT");
	Dvb::Demuxer demuxer_pmt(demux_path);
	demuxer_pmt.set_filter(stream.get_pmt_pid(), PMT_ID);
	demuxer_pmt.read_section(buffer);
	demuxer_pmt.stop();

	stream.parse_pms(buffer, ignore_teletext);

	guint pcr_pid = stream.get_pcr_pid();
	if (pcr_pid != 0x1FFF)
	{
		channel_stream.add_pes_demuxer(demux_path, pcr_pid, DMX_PES_OTHER, "PCR");
	}
	
	gsize video_streams_size = stream.video_streams.size();
	for (guint i = 0; i < video_streams_size; i++)
	{
		channel_stream.add_pes_demuxer(demux_path, stream.video_streams[i].pid, DMX_PES_OTHER, "video");
	}

	gsize audio_streams_size = stream.audio_streams.size();
	for (guint i = 0; i < audio_streams_size; i++)
	{
		channel_stream.add_pes_demuxer(demux_path, stream.audio_streams[i].pid, DMX_PES_OTHER, "audio");
	}
				
	gsize subtitle_streams_size = stream.subtitle_streams.size();
	for (guint i = 0; i < subtitle_streams_size; i++)
	{
		channel_stream.add_pes_demuxer(demux_path, stream.subtitle_streams[i].pid, DMX_PES_OTHER, "subtitle");
	}

	gsize teletext_streams_size = stream.teletext_streams.size();
	for (guint i = 0; i < teletext_streams_size; i++)
	{
		channel_stream.add_pes_demuxer(demux_path, stream.teletext_streams[i].pid, DMX_PES_OTHER, "teletext");
	}

	g_debug("Finished setting up DVB (%s)", frontend.get_path().c_str());
}

void FrontendThread::start_epg_thread()
{
	if (!disable_epg_thread)
	{
		if (epg_thread == NULL)
		{
			epg_thread = new EpgThread(frontend, text_encoding, timeout);
			epg_thread->start();
			g_debug("EPG thread started");
		}
	}
}

void FrontendThread::stop_epg_thread()
{
	if (!disable_epg_thread)
	{
		if (epg_thread != NULL)
		{
			g_debug("Stopping EPG thread (%s)", frontend.get_path().c_str());
			delete epg_thread;
			epg_thread = NULL;
			g_debug("EPG thread stopped (%s)", frontend.get_path().c_str());
		}
	}
}

void FrontendThread::start_rtsp(Channel& channel, int client_id)
{
	g_debug("FrontendThread::start_rtsp(%s)", channel.name.c_str());
	stop();
	
	g_debug("Creating new stream output");

	String fifo_path = String::compose("/tmp/me-tv-%1.ts", client_id);

	if (Glib::file_test(fifo_path, Glib::FILE_TEST_EXISTS))
	{
		unlink(fifo_path.c_str());
	}

	if (mkfifo(fifo_path.c_str(), S_IRUSR|S_IWUSR) != 0)
	{
		throw Exception(Glib::ustring::compose(_("Failed to create FIFO '%1'"), fifo_path));
	}

	// Fudge the channel open.  Allows Glib::IO_FLAG_NONBLOCK
	int fd = open(fifo_path.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd == -1)
	{
		throw SystemException(Glib::ustring::compose(_("Failed to open FIFO for reading '%1'"), fifo_path));
	}

	RtspChannelStream* channel_stream = new RtspChannelStream(channel, client_id, fifo_path);
	setup_dvb(*channel_stream);
	streams.push_back(channel_stream);

	start();

	::close(fd);
}

void FrontendThread::stop_rtsp(int client_id)
{
	stop();
	gboolean found = false;

	ChannelStreamList::iterator iterator = streams.begin();

	while (iterator != streams.end() && !found)
	{
		ChannelStream* channel_stream = *iterator;
		if (channel_stream->type == CHANNEL_STREAM_TYPE_RTSP)
		{
			if (((RtspChannelStream*)channel_stream)->get_client_id() == client_id)
			{
				delete channel_stream;
				iterator = streams.erase(iterator);
				g_debug("Stopped RTSP stream");
				found = true;
			}
		}

		iterator++;
	}

	if (streams.empty())
	{
		frontend.close();
	}

	start();
}

String make_recording_filename(Channel& channel, const String& description)
{
	String start_time = get_local_time_text("%c");
	String filename;
	String title = description;
	
	if (title.empty())
	{
		filename = String::compose
		(
			"%1 - %2.mpeg",
			channel.name,
			start_time
		);
	}
	else
	{
		filename = String::compose
		(
			"%1 - %2 - %3.mpeg",
			title,
			channel.name,
			start_time
		);
	}

	// Clean filename
	String::size_type position = String::npos;
	while ((position = filename.find('/')) != String::npos)
	{
		filename.replace(position, 1, "_");
	}

	while ((position = filename.find(':')) != String::npos )
	{
		filename.replace(position, 1, "_");
	}

	String fixed_filename = Glib::filename_from_utf8(filename);
	
	return Glib::build_filename(recording_directory, fixed_filename);
}

bool is_recording_stream(ChannelStream* channel_stream)
{
	return channel_stream->type == CHANNEL_STREAM_TYPE_RECORDING || channel_stream->type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING;
}

void FrontendThread::start_recording(Channel& channel,
                                     const String& description,
                                     gboolean scheduled)
{
	stop();	

	ChannelStreamType requested_type = scheduled ? CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING : CHANNEL_STREAM_TYPE_RECORDING;
	ChannelStreamType current_type = CHANNEL_STREAM_TYPE_NONE;

	ChannelStreamList::iterator iterator = streams.begin();
	
	while (iterator != streams.end())
	{
		ChannelStream* channel_stream = *iterator;
		if (channel_stream->channel == channel && is_recording_stream(channel_stream))
		{
			current_type = channel_stream->type;
			break;
		}
		iterator++;
	}

	// No change required
	if (current_type == requested_type)
	{
		g_debug("Channel '%s' is currently being recorded (%s)",
		    channel.name.c_str(), scheduled ? "scheduled" : "manual");
	}
	else
	{
		// If SR requested but recording is currently manual then stop the current manual one
		if (requested_type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING && current_type == CHANNEL_STREAM_TYPE_RECORDING)
		{
			stop_recording(channel);
		}

		if (requested_type == CHANNEL_STREAM_TYPE_RECORDING && current_type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING)
		{
			g_debug("Ignoring request to manually record a channel that is currently scheduled for recording");
		}
		else
		{
			g_debug("Channel '%s' is not currently being recorded", channel.name.c_str());

			if (channel.transponder != frontend.get_frontend_parameters())
			{
				g_debug("Need to change transponders to record this channel");

				// Need to kill all current streams
				ChannelStreamList::iterator iterator = streams.begin();
				while (iterator != streams.end())
				{
					ChannelStream* channel_stream = *iterator;
					delete channel_stream;
					iterator = streams.erase(iterator);
				}
			}

			RecordingChannelStream* channel_stream = new RecordingChannelStream(
				channel, scheduled, make_recording_filename(channel, description), description);
			setup_dvb(*channel_stream);
			streams.push_back(channel_stream);
		}
	}
	
	g_debug("New recording channel created (%s)", frontend.get_path().c_str());

	start();
}

void FrontendThread::stop_recording(const Channel& channel)
{
	stop();

	ChannelStreamList::iterator iterator = streams.begin();

	while (iterator != streams.end())
	{
		ChannelStream* channel_stream = *iterator;
		if (channel_stream->channel == channel && is_recording_stream(channel_stream))
		{
			delete channel_stream;
			iterator = streams.erase(iterator);
		}
		else
		{
			iterator++;
		}
	}

	if (streams.empty())
	{
		frontend.close();
	}

	start();
}

gboolean FrontendThread::is_recording(const Channel& channel)
{
	for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
	{
		ChannelStream* channel_stream = *i;
		if (channel_stream->channel == channel && is_recording_stream(channel_stream))
		{
			return true;
		}
	}

	return false;
}

gboolean FrontendThread::is_available(const Channel& channel)
{
	if (!(channel.transponder == frontend.get_frontend_parameters()) && !streams.empty())
	{
		return false;
	}

	return true;
}

gboolean FrontendThread::is_rtsp()
{
	for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
	{
		ChannelStream* channel_stream = *i;
		if (channel_stream->type == CHANNEL_STREAM_TYPE_RTSP)
		{
			return true;
		}
	}

	return false;
}


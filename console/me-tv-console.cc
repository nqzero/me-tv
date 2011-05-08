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

#include <giomm.h>
#include <iostream>
#include "../common/client.h"
#include "../common/exception.h"
#include "../config.h"

int main (int argc, char *argv[])
{
	g_debug("Me TV Console %s\n", VERSION);
	
	Gio::init();

	Glib::ustring server_host;
	gint server_port = 1999;
	Client client;

	Glib::OptionEntry host_option_entry;
	host_option_entry.set_long_name("server-host");
	host_option_entry.set_description(_("Me TV server host (default: localhost)"));

	Glib::OptionEntry port_option_entry;
	port_option_entry.set_long_name("server-port");
	port_option_entry.set_description(_("Me TV server port (default: 1999)"));

	Glib::OptionGroup option_group(PACKAGE_NAME, "", _("Show Me TV help options"));
	option_group.add_entry(host_option_entry, server_host);
	option_group.add_entry(port_option_entry, server_port);

	Glib::OptionContext option_context;
	option_context.set_summary("Me TV console client");
	option_context.set_description("A command line client for interacting with Me TV");
	option_context.set_main_group(option_group);
		
	try
	{
		option_context.parse(argc, argv);

		gboolean registered = client.register_client(server_host, server_port);

		if (!registered)
		{
			throw Exception("Failed to register");
		}
		
		Glib::ustring line;

		gboolean unregister = true;
		gboolean done = false;
		while (!done)
		{
			std::cout << "=========================================================" << std::endl;
			std::cout << "1. Get status" << std::endl;
			std::cout << "2. Get channels" << std::endl;
			std::cout << "3. Get scheduled recordings" << std::endl;
			std::cout << "4. Terminate Me TV server (and then client)" << std::endl;
			std::cout << "q. Quit" << std::endl;
			std::cout << "> ";

			std::cin >> line;
			if (line == "1")
			{
				int index = 0;
				Client::FrontendList frontends = client.get_status(); 
				for (Client::FrontendList::iterator i = frontends.begin(); i != frontends.end(); i++)
				{
					Client::Frontend& frontend = *i;
					std::cout << ++index << ": " << frontend.path << std::endl; 
					Client::Frontend::StreamList& streams = frontend.streams;
					for (Client::Frontend::StreamList::iterator j = streams.begin(); j != streams.end(); j++)
					{
						Client::Frontend::Stream& stream = *j;
						Glib::ustring type = stream.type == 0 ? "Broadcast" : "Recording";
						std::cout << " - Channel " << stream.channel_id << " - " << type << std::endl;

						Client::DemuxerList& demuxers = stream.demuxers;
						for (Client::DemuxerList::iterator k = demuxers.begin(); k != demuxers.end(); k++)
						{
							Client::Demuxer& demuxer = *k;
							std::cout << "  - Demuxer " << demuxer.pid << " - " << demuxer.filter_type << std::endl;
						}

					}
				}
			}
			else if (line == "2")
			{
				Client::ChannelList channels = client.get_channels();
				for (Client::ChannelList::iterator i = channels.begin(); i != channels.end(); i++)
				{
					Client::Channel& channel = *i;
					std::cout << channel.id << ": " << channel.name;
					if (!channel.epg_events.empty())
					{
						Client::EpgEvent& epg_event = *(channel.epg_events.begin());
						std::cout << " - " << epg_event.title; 
					}
					std::cout << std::endl; 
				}
			}
			else if (line == "3")
			{
				Client::ScheduledRecordingList scheduled_recordings = client.get_scheduled_recordings();
				for (Client::ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
				{
					Client::ScheduledRecording& scheduled_recording = *i;
					std::cout << scheduled_recording.id << ": " << scheduled_recording.description;
					std::cout << " - Channel " << scheduled_recording.channel_id;
					std::cout << " - " << scheduled_recording.get_start_time_text() << " (" << scheduled_recording.start_time << ")";
					std::cout << " - " << scheduled_recording.get_duration_text() << " (" << scheduled_recording.duration << ")";
					std::cout << std::endl;
				}
			}
			else if (line == "4")
			{
				client.terminate();
				unregister = false;
				done = true;
			}
			else if (line == "q")
			{
				done = true;
			}
			else
			{
				std::cout << "Invalid input" << std::endl;
			}
		}

		if (unregister)
		{
			client.unregister_client();
		}
	}
	catch (const Glib::Exception& exception)
	{
		g_message("Exception: %s", exception.what().c_str());
	}
	catch (...)
	{
		g_message(_("An unhandled error occurred"));		
	}
	
	return 0;
}

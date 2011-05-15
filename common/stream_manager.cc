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

#include <glibmm.h>
#include "stream_manager.h"
#include "global.h"
#include "device_manager.h"
#include "dvb_transponder.h"
#include "dvb_si.h"
#include "exception.h"

void StreamManager::initialise(const String& text_encoding, guint timeout, gboolean ignore_teletext)
{
	g_debug("Creating stream manager");
	FrontendList& frontends = device_manager.get_frontends();
	for(FrontendList::iterator i = frontends.begin(); i != frontends.end(); i++)
	{
		g_debug("Creating frontend thread");
		FrontendThread* frontend_thread = new FrontendThread(**i, text_encoding, timeout, ignore_teletext);
		frontend_threads.push_back(frontend_thread);
	}
}

StreamManager::~StreamManager()
{
	g_debug("Destroying StreamManager");
	stop();

	FrontendThreadList::iterator i = frontend_threads.begin(); 
	while (i != frontend_threads.end())
	{
		FrontendThread* frontend_thread = *i;
		delete frontend_thread;
		i = frontend_threads.erase(i);
	}
	g_debug("StreamManager destroyed");
}

void StreamManager::start_recording(const ScheduledRecording& scheduled_recording)
{
	gboolean frontend_found = false;

	Channel channel = ChannelManager::get(scheduled_recording.channel_id);

	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end() && !frontend_found; i++)
	{
		FrontendThread& frontend_thread = **i;
		g_debug("CHECKING: %s", frontend_thread.frontend.get_path().c_str());
		if (frontend_thread.frontend.get_path() == scheduled_recording.device)
		{
			if (!frontend_thread.is_recording(channel))
			{
				frontend_thread.start_recording(channel, scheduled_recording.description, true);
			}
			else
			{
				g_debug("Channel '%s' is currently being recorded", channel.name.c_str());
			}

			frontend_found = true;
		}
	}

	if (!frontend_found)
	{
		String message = String::compose(
		    _("Failed to find frontend '%1' for scheduled recording"),
		    scheduled_recording.device);
		throw Exception(message);
	}
}

void StreamManager::stop_recording(const Channel& channel)
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		frontend_thread.stop_recording(channel);
	}
}

void StreamManager::start()
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		g_debug("Starting frontend thread");
		FrontendThread& frontend_thread = **i;
		frontend_thread.start();
	}
}

void StreamManager::stop()
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		g_debug("Stopping frontend thread");
		FrontendThread& frontend_thread = **i;
		frontend_thread.stop();
	}
}

void StreamManager::start_broadcasting(Channel& channel, int client_id, const String& interface, const String& address, int port)
{
	gboolean found = false;

	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end() && !found; i++)
	{
		FrontendThread& frontend_thread = **i;
		if (frontend_thread.frontend.get_frontend_type() != channel.transponder.frontend_type)
		{
			g_debug("'%s' incompatible", frontend_thread.frontend.get_name().c_str());
			continue;
		}
			
		if (frontend_thread.is_available(channel))
		{
			g_debug("Selected frontend '%s' (%s) for broadcast",
				frontend_thread.frontend.get_name().c_str(),
				frontend_thread.frontend.get_path().c_str());
			frontend_thread.start_broadcasting(channel, client_id, interface, address, port);
			found = true;
			break;
		}
	}

	if (!found)
	{
		throw Exception(_("Failed to get available frontend"));
	}
}

void StreamManager::stop_broadcasting(int client_id)
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		(*i)->stop_broadcasting(client_id);
	}
}

gboolean StreamManager::is_broadcasting(const String& device)
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread* frontend_thread = *i;
		if (frontend_thread->frontend.get_path() == device)
		{
			return frontend_thread->is_broadcasting();
		}
	}

	throw Exception(_("No such device"));
}


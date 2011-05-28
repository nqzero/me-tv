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

#include "rtsp_server_thread.h"
#include "exception.h"
#include <vector>
//#include <liveMedia.hh>

RTSPServerThread::RTSPServerThread() : Thread("RTSP Server")
{
/*	scheduler = BasicTaskScheduler::createNew();
 	env = BasicUsageEnvironment::createNew(*scheduler);
	rtsp_server = RTSPServer::createNew(*env, 8554);

	if (rtsp_server == NULL)
	{
		throw Exception("Failed to create RTSP server");
	}
*/
}
/*
ServerMediaSession* RTSPServerThread::lookupServerMediaSession(char const* streamName)
{
	String path = String::compose("/tmp/%1", streamName);
	ServerMediaSession* sms = ServerMediaSession::createNew(*env, "mpeg2TransportStream", "mpeg2TransportStream", "Me TV RTSP Server");
	sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(*env, path.c_str(), "", False));
	rtsp_server->addServerMediaSession(sms);
}
*/

void RTSPServerThread::run()
{
	std::vector<String> args;

	g_debug("Killing existing live555MediaServer processes");
	args.push_back("/usr/bin/killall");
	args.push_back("/usr/bin/live555MediaServer");
	Glib::spawn_sync(".", args);

	args.clear();

	g_debug("Spawning live555MediaServer");
	args.push_back("/usr/bin/live555MediaServer");
	Glib::spawn_sync("/tmp", args);

//	env->taskScheduler().doEventLoop();
}

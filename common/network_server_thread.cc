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

#include "network_server_thread.h"
#include "global.h"
#include "exception.h"
#include "common.h"
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

NetworkServerThread::NetworkServerThread(guint server_port) : Thread("Network Server")
{
	request_handler.set_broadcast_address(broadcast_address);

	g_message("Starting socket service on port %d", server_port);

	socket_server = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_server < 0)
	{
		throw SystemException("Failed to open socket");
	}

	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(server_port);
	if (bind(socket_server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	{
		throw SystemException("Failed to bind");
	}

	listen(socket_server, 5);
}

void NetworkServerThread::run()
{
	sigc::connection connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &NetworkServerThread::on_timeout), 1);

	while (!is_terminated())
	{
		struct sockaddr_in client_addr;
		socklen_t client_length = sizeof(client_addr);
		int socket_client = accept(socket_server, (struct sockaddr *)&client_addr, &client_length);
		if (socket_client > 0)
		{
			gboolean do_terminate = request_handler.handle_request(socket_client);
			::close(socket_client);

			if (do_terminate)
			{
				signal_quit();
				terminate();
			}
		}
	}

	connection.disconnect();
}

gboolean NetworkServerThread::on_timeout()
{
	static guint last_seconds = 60;

	try
	{
		guint now = time(NULL);
		guint seconds = now % 60;
		bool update = last_seconds > seconds;
		last_seconds = seconds;

		if (update)
		{
			request_handler.clients.check();
			ScheduledRecordingManager::check_auto_recordings();
			ScheduledRecordingManager::check_scheduled_recordings();

			signal_update();
		}
	}
	catch(...)
	{
		handle_error();
	}
	
	return true;
}

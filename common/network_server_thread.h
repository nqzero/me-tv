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

#ifndef __NETWORK_SERVER_THREAD_H__
#define __NETWORK_SERVER_THREAD_H__

#include "thread.h"
#include "request_handler.h"

class NetworkServerThread : public Thread
{
private:
	int				socket_server;
	RequestHandler	request_handler;
	
	gboolean on_timeout();
	
protected:
	void run();

public:
	NetworkServerThread(guint server_port, const String& broadcast_address);
};

#endif

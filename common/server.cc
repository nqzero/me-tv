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

#include "server.h"
#include "me-tv-types.h"
#include "common.h"
#include "data.h"
#include "crc32.h"

Server::Server(int port) : server_thread(port)
{
	Gnome::Gda::init();
	Crc32::init();
}

void Server::start()
{
	data_connection = Data::create_connection();

	recording_directory = Data::get_scalar("configuration", "value", "name", "recording_directory");
	preferred_language = Data::get_scalar("configuration", "value", "name", "preferred_language");

	device_manager.initialise(devices);
	stream_manager.initialise(text_encoding, read_timeout, ignore_teletext);
	stream_manager.start();
	rtsp_server_thread.start();
	server_thread.start();
}

void Server::stop()
{
	server_thread.terminate();
}

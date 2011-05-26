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

#ifndef __REQUEST_HANDLER__
#define __REQUEST_HANDLER__

#include <giomm/socketconnection.h>
#include <libxml++/libxml++.h>
#include "data.h"

class RequestHandler
{
private:
	class Client
	{
	public:
		~Client();

		int id;
		time_t last;
	};

	class ClientList : public std::list<Client>
	{
	private:
		int client_id;
		int get_free_port();
		bool is_port_used(int port);
	public:
		ClientList() : client_id((int)time(NULL)) {}

		Client& get(int client_id);
		int add();
		void update(int client_id);
		void check();
		void remove(int client_id);
	};

	xmlpp::Node* get_attribute(const xmlpp::Node* node, const String& xpath);
	String get_attribute_value(const xmlpp::Node* node, const String& xpath);
	int get_int_attribute_value(const xmlpp::Node* node, const String& xpath);
	gboolean get_bool_attribute_value(const xmlpp::Node* node, const String& xpath);
	void send_response(int sockfd,
		const String& error_message, const String& body);
	gboolean handle_connection(int sockfd);

public:
	RequestHandler() {}

	ClientList clients;

	gboolean handle_request(int sockfd);
};

#endif

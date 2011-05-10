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

#include "client.h"
#include "exception.h"
#include "common.h"
#include <glibmm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace xmlpp;

Node* check_response(Node* requestNode)
{
	if (requestNode->get_name() != "response")
	{
		throw Exception("Not a response node");
	}

	NodeSet nodeSet = requestNode->find("@error");
	if (!nodeSet.empty())
	{
		const Node* errorNode = *(nodeSet.begin());
		throw Exception(dynamic_cast<const Attribute*>(errorNode)->get_value());
	}
	return requestNode;
}

Glib::ustring get_start_time_text(int start_time)
{
	return get_local_time_text(start_time, "%A, %d %B %Y, %H:%M");
}

Glib::ustring Client::EpgEvent::get_start_time_text() const
{
	return ::get_start_time_text(start_time);
}

Glib::ustring Client::ScheduledRecording::get_start_time_text() const
{
	return ::get_start_time_text(start_time);
}

Glib::ustring get_duration_text(int duration)
{
	Glib::ustring result;
	guint hours = duration / (60*60);
	guint minutes = (duration % (60*60)) / 60;
	if (hours > 0)
	{
		result = Glib::ustring::compose(ngettext("1 hour", "%1 hours", hours), hours);
	}
	if (hours > 0 && minutes > 0)
	{
		result += ", ";
	}
	if (minutes > 0)
	{
		result += Glib::ustring::compose(ngettext("1 minute", "%1 minutes", minutes), minutes);
	}

	return result;
}

Glib::ustring Client::EpgEvent::get_duration_text() const
{
	return ::get_duration_text(duration);
}

Glib::ustring Client::ScheduledRecording::get_duration_text() const
{
	return ::get_duration_text(duration);
}

Client::Channel& Client::ChannelList::get_by_id(int channel_id)
{
	for (ChannelList::iterator i = begin(); i != end(); i++)
	{
		Channel& channel = *i;
		if (channel.id == channel_id)
		{
			return channel;
		}
	}

	throw Exception("Channel not found");
}

Client::Client()
{
	client_id = 0;
	broadcasting_channel_id = -1;
}

Client::~Client()
{
	if (client_id != 0)
	{
		unregister_client();
	}
}

Node* Client::send_request(const Glib::ustring& command)
{
	return send_request(command, "");
}

Node* Client::send_request(const Glib::ustring& command, ParameterList& parameters)
{
	Glib::ustring innerXml;

	for (ParameterList::iterator i = parameters.begin(); i != parameters.end(); i++)
	{
		Parameter& parameter = *i;
		innerXml += "<parameter name=\"" + parameter.name + "\" value=\"" + parameter.value + "\" />";
	}

	return send_request(command, innerXml);
}

Node* Client::send_request(const Glib::ustring& command, const Glib::ustring& innerXml)
{
	g_debug("Sending request");

	Glib::ustring request = "<?xml version=\"1.0\" ?>";
	request += Glib::ustring::compose("<request client_id=\"%1\" command=\"%2\">", client_id, command);
	request += innerXml;
	request += "</request>";

	struct sockaddr_in serv_addr;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		throw SystemException("Failed to open socket");
	}

	if (host.empty())
	{
		host = "localhost";
	}
	
	struct hostent* server = gethostbyname(host.c_str());
	if (server == NULL)
	{
		throw SystemException("Failed to get host");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);
	if (connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		throw SystemException("Failed to connect");
	}

	write_string(sockfd, request);
	::shutdown(sockfd, SHUT_WR);

	Glib::ustring response = read_string(sockfd);

	::close(sockfd);

	parser.parse_memory(response);
	
	return check_response(parser.get_document()->get_root_node());
}

Glib::ustring get_attribute_value(Node* node)
{
	return dynamic_cast<xmlpp::Attribute*>(node)->get_value();
}

Glib::ustring get_attribute_value(Node* node, const Glib::ustring& xpath)
{
	NodeSet nodeSet = node->find(xpath);

	if (nodeSet.size() != 1)
	{
		throw Exception(Glib::ustring::compose("Failed to get single attribute for '%1'", xpath));
	}

	return get_attribute_value(*(nodeSet.begin()));
}

int get_int_attribute_value(Node* node, const Glib::ustring& xpath)
{
	return ::atoi(get_attribute_value(node, xpath).c_str());
}

gboolean get_bool_attribute_value(Node* node, const Glib::ustring& xpath)
{
	return get_attribute_value(node, xpath) == "true";
}

Client::ChannelList Client::get_channels()
{
	NodeSet channelNodes = send_request("get_channels")->find("channel");

	ChannelList channels;
	for (NodeSet::iterator i = channelNodes.begin(); i != channelNodes.end(); i++)
	{
		Node* channel_node = *i;

		Channel channel;
		channel.id = get_int_attribute_value(channel_node, "@id");
		channel.name = get_attribute_value(channel_node, "@name");
		channel.record_extra_before = get_int_attribute_value(channel_node, "@record_extra_before");
		channel.record_extra_after = get_int_attribute_value(channel_node, "@record_extra_after");

		NodeSet epg_events = channel_node->find("event");
		for (NodeSet::iterator j = epg_events.begin(); j != epg_events.end(); j++)
		{
			Node* event_node = *j;

			Client::EpgEvent epg_event;
			epg_event.id = get_int_attribute_value(event_node, "@id");
			epg_event.channel_id = get_int_attribute_value(event_node, "@channel_id");
			epg_event.start_time = get_int_attribute_value(event_node, "@start_time");
			epg_event.duration = get_int_attribute_value(event_node, "@duration");
			epg_event.title = get_attribute_value(event_node, "@title");
			epg_event.subtitle = get_attribute_value(event_node, "@subtitle");
			epg_event.description = get_attribute_value(event_node, "@description");
			epg_event.scheduled_recording_id = get_int_attribute_value(event_node, "@scheduled_recording_id");
			channel.epg_events.push_back(epg_event);
		}

		channels.push_back(channel);
	}
	
	return channels;
}

Client::ChannelList Client::get_epg(int start_time, int end_time)
{
	ParameterList parameters;
	parameters.add("start_time", start_time);
	parameters.add("end_time", end_time);

	Node* root_node = send_request("get_epg", parameters);
	NodeSet channel_nodes = root_node->find("channel");

	ChannelList channels;
	for (NodeSet::iterator i = channel_nodes.begin(); i != channel_nodes.end(); i++)
	{
		Node* channel_node = *i;

		Channel channel;
		channel.id = get_int_attribute_value(channel_node, "@id");
		channel.name = get_attribute_value(channel_node, "@name");

		NodeSet epg_events = channel_node->find("event");
		for (NodeSet::iterator j = epg_events.begin(); j != epg_events.end(); j++)
		{
			Node* event_node = *j;

			EpgEvent epg_event;
			epg_event.id = get_int_attribute_value(event_node, "@id");
			epg_event.channel_id = get_int_attribute_value(event_node, "@channel_id");
			epg_event.start_time = get_int_attribute_value(event_node, "@start_time");
			epg_event.duration = get_int_attribute_value(event_node, "@duration");
			epg_event.title = get_attribute_value(event_node, "@title");
			epg_event.subtitle = get_attribute_value(event_node, "@subtitle");
			epg_event.description = get_attribute_value(event_node, "@description");
			epg_event.scheduled_recording_id = get_int_attribute_value(event_node, "@scheduled_recording_id");

			channel.epg_events.push_back(epg_event);
		}		

		channels.push_back(channel);
	}
	return channels;
}

Client::ScheduledRecordingList Client::get_scheduled_recordings()
{
	Client::ScheduledRecordingList results;

	Node* root_node = send_request("get_scheduled_recordings");
	NodeSet nodes = root_node->find("scheduled_recording");
	for (NodeSet::iterator i = nodes.begin(); i != nodes.end(); i++)
	{
		Client::ScheduledRecording scheduled_recording;

		Node* node = *i;
		scheduled_recording.id = get_int_attribute_value(*i, "@id");
		scheduled_recording.recurring_type = get_int_attribute_value(*i, "@recurring_type");
		scheduled_recording.channel_id = get_int_attribute_value(*i, "@channel_id");
		scheduled_recording.description = get_attribute_value(*i, "@description");
		scheduled_recording.start_time = get_int_attribute_value(*i, "@start_time");
		scheduled_recording.duration = get_int_attribute_value(*i, "@duration");
		scheduled_recording.device = get_attribute_value(*i, "@device");

		results.push_back(scheduled_recording);
	}
		
	return results;
}

void Client::add_scheduled_recording(int epg_event_id)
{
	ParameterList parameters;
	parameters.add("epg_event_id", epg_event_id);
	send_request("add_scheduled_recording", parameters);
}

void Client::remove_scheduled_recording(int scheduled_recording_id)
{
	ParameterList parameters;
	parameters.add("scheduled_recording_id", scheduled_recording_id);
	send_request("remove_scheduled_recording", parameters);
}

Client::BroadcastingStream Client::start_broadcasting(int channel_id, gboolean multicast)
{
	Client::BroadcastingStream result;

	if (broadcasting_channel_id == channel_id)
	{
		g_debug("Already broadcasting channel %d", broadcasting_channel_id);
	}
	else
	{
		stop_broadcasting();

		ParameterList parameters;
		parameters.add("channel", channel_id);
		parameters.add("multicast", multicast ? "true" : "false");
		Node* node = send_request("start_broadcasting", parameters);

		result.protocol = get_attribute_value(node, "stream/@protocol");
		result.address = get_attribute_value(node, "stream/@address");
		result.port = get_int_attribute_value(node, "stream/@port");

		broadcasting_channel_id = channel_id;
	}

	return result;
}

void Client::stop_broadcasting()
{
	if (broadcasting_channel_id != -1)
	{
		send_request("stop_broadcasting");
		broadcasting_channel_id = -1;
	}
}

Client::EpgEventList Client::search_epg(const Glib::ustring& text, gboolean include_description)
{
	ParameterList parameters;
	parameters.add("text", text);
	parameters.add("include_description", include_description ? "true" : "false");
	NodeSet events = send_request("search_epg", parameters)->find("event");

	Client::EpgEventList epg_events;
	for (NodeSet::iterator i = events.begin(); i != events.end(); i++)
	{
		Node* event_node = *i;

		Client::EpgEvent epg_event;
		epg_event.id = get_int_attribute_value(event_node, "@id");
		epg_event.channel_id = get_int_attribute_value(event_node, "@channel_id");
		epg_event.start_time = get_int_attribute_value(event_node, "@start_time");
		epg_event.duration = get_int_attribute_value(event_node, "@duration");
		epg_event.title = get_attribute_value(event_node, "@title");
		epg_event.subtitle = get_attribute_value(event_node, "@subtitle");
		epg_event.description = get_attribute_value(event_node, "@description");
		epg_event.scheduled_recording_id = get_int_attribute_value(event_node, "@scheduled_recording_id");

		epg_events.push_back(epg_event);
	}

	return epg_events;	
}

Client::FrontendList Client::get_status()
{
	NodeSet frontend_nodes = send_request("get_status")->find("frontend");

	Client::FrontendList frontends;
	for (NodeSet::iterator i = frontend_nodes.begin(); i != frontend_nodes.end(); i++)
	{
		Node* frontend_node = *i;

		Client::Frontend frontend;
		frontend.path = get_attribute_value(frontend_node, "@path");

		NodeSet stream_nodes = frontend_node->find("stream");
		for (NodeSet::iterator j = stream_nodes.begin(); j != stream_nodes.end(); j++)
		{
			Node* stream_node = *j;

			Client::Frontend::Stream stream;
			stream.channel_id = get_int_attribute_value(stream_node, "@channel_id");
			stream.type = get_int_attribute_value(stream_node, "@type");

			frontend.streams.push_back(stream);
		}

		frontends.push_back(frontend);
	}
	
	return frontends;
}

gboolean Client::register_client(const Glib::ustring& h, int p)
{
	try
	{
		host = h;
		port = p;
		Node* root_node = send_request("register");
		client_id = get_int_attribute_value(root_node, "client/@id");
	}
	catch(...)
	{
		return false;
	}

	return true;
}

void Client::unregister_client()
{
	send_request("unregister");
	broadcasting_channel_id = -1;
	client_id = 0;
}

void Client::add_channel(const Glib::ustring& line)
{
	ParameterList parameters;
	parameters.add("line", line);
	send_request("add_channel", parameters);
}

void Client::remove_channel(int channel_id)
{
	ParameterList parameters;
	parameters.add("channel_id", channel_id);
	send_request("remove_channel", parameters);
}

StringList Client::get_auto_record_list()
{
	StringList result;

	NodeSet nodes = send_request("get_auto_record_list")->find("auto_record");
	for (NodeSet::iterator i = nodes.begin(); i != nodes.end(); i++)
	{
		Node* node = *i; 
		result.push_back(get_attribute_value(node, "@title"));
	}

	return result;
}

void Client::set_auto_record_list(StringList& auto_record_list)
{
	Glib::ustring innerXml;

	for (StringList::iterator i = auto_record_list.begin(); i != auto_record_list.end(); i++)
	{
		innerXml += Glib::ustring::compose("<auto_record title=\"%1\" />", *i);
	}

	send_request("set_auto_record_list", innerXml);
}

Client::ConfigurationMap Client::get_configuration()
{
	Client::ConfigurationMap result;

	NodeSet nodes = send_request("get_configuration")->find("configuration");
	for (NodeSet::iterator i = nodes.begin(); i != nodes.end(); i++)
	{
		Node* node = *i;

		Glib::ustring name = get_attribute_value(node, "@name");
		Glib::ustring value = get_attribute_value(node, "@value");

		result[name] = value;
	}

	return result;
}

void Client::set_configuration(Client::ConfigurationMap& configuration)
{
	Glib::ustring innerXml;

	for (Client::ConfigurationMap::iterator i = configuration.begin(); i != configuration.end(); i++)
	{
		innerXml += Glib::ustring::compose("<configuration name=\"%1\" value=\"%2\" />", (*i).first, (*i).second);
	}

	send_request("set_configuration", innerXml);
}

void Client::set_channel(guint channel_id, const Glib::ustring& name,
		guint sort_order, gint record_extra_before, gint record_extra_after)
{
	ParameterList parameters;
	parameters.add("id", channel_id);
	parameters.add("name", name);
	parameters.add("sort_order", sort_order);
	parameters.add("record_extra_before", record_extra_before);
	parameters.add("record_extra_after", record_extra_after);
	send_request("set_channel", parameters);
}

void Client::terminate()
{
	send_request("terminate");
	client_id = 0;
}


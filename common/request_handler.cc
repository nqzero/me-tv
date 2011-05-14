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

#include "request_handler.h"
#include "exception.h"
#include "common.h"
#include "epg_events.h"
#include "global.h"
#include "channels_conf_line.h"

using namespace xmlpp;

Node* RequestHandler::get_attribute(const Node* node, const Glib::ustring& xpath)
{
	NodeSet result = node->find(xpath);
	return result.empty() ? NULL : result.at(0);
}

Glib::ustring RequestHandler::get_attribute_value(const Node* node, const Glib::ustring& xpath)
{
	Node* resultNode = get_attribute(node, xpath);
	if (resultNode == NULL)
	{
		throw Exception(Glib::ustring::compose(
			"XPath expression '%1' returned no results",
			xpath));
	}
	return dynamic_cast<Attribute*>(resultNode)->get_value();
}

int RequestHandler::get_int_attribute_value(const Node* node, const Glib::ustring& xpath)
{
	return ::atoi(get_attribute_value(node, xpath).c_str());
}

gboolean RequestHandler::get_bool_attribute_value(const Node* node, const Glib::ustring& xpath)
{
	return get_attribute_value(node, xpath) == "true";
}

RequestHandler::Client::~Client()
{
	stream_manager.stop_broadcasting(id);
}

void RequestHandler::ClientList::remove(int client_id)
{
	for (ClientList::iterator i = begin(); i != end(); i++)
	{
		Client& client = *i;
		if (client.id == client_id)
		{
			erase(i);
			return;
		}
	}

	throw Exception("Client not found");
}

int RequestHandler::ClientList::add()
{
	Client client;
	client.id = ++client_id;
	client.last = time(NULL);
	client.broadcast_port = get_free_port();
	push_back(client);

	return client.id;
}

void RequestHandler::ClientList::check()
{
	g_debug("Checking clients");

	time_t now = time(NULL);
	for (ClientList::iterator i = begin(); i != end(); i++)
	{
		Client& client = *i;
		if (now - client.last > 90)
		{
			int client_id = client.id;
			i = erase(i);
			g_debug("Removed client %d", client_id);
		}
	}
}

RequestHandler::Client& RequestHandler::ClientList::get(int client_id)
{
	for (ClientList::iterator i = begin(); i != end(); i++)
	{
		Client& client = *i;
		if (client.id == client_id)
		{
			return client;
		}
	}

	throw Exception("Client not found");
}

void RequestHandler::ClientList::update(int client_id)
{
	get(client_id).last = time(NULL);
}

bool RequestHandler::ClientList::is_port_used(int port)
{
	for (ClientList::iterator i = begin(); i != end(); i++)
	{
		Client& client = *i;
		if (client.broadcast_port == port)
		{
			return true;
		}
	}

	return false;
}

int RequestHandler::ClientList::get_free_port()
{
	bool found = false;
	int port = 2005;

	while (!found)
	{
		if (port >= 3000)
		{
			throw Exception("Port is too high");
		}

		if (!is_port_used(port))
		{
			found = true;
		}
		else
		{
			port++;
		}
	}

	return port;
}

gboolean RequestHandler::handle_connection(int sockfd)
{
	Glib::ustring body;
	gboolean result = false;

	Glib::ustring request = read_string(sockfd);

	DomParser parser;
	parser.parse_memory(request);
	g_debug("Input parsed");
	
	if (!parser)
	{
		throw Exception("Failed to parse request");
	}

	const Node* root_node = parser.get_document()->get_root_node();
	const Glib::ustring& command = get_attribute_value(root_node, "@command");

	Glib::ustring error_message;
	
	g_debug("Command: %s", command.c_str());
	if (command == "register")
	{
		body += Glib::ustring::compose("<client id=\"%1\" />", clients.add());
	}
	else
	{
		int client_id = get_int_attribute_value(root_node, "@client_id");
		clients.update(client_id);

		if (command == "unregister")
		{
			clients.remove(client_id);
		}
		else if (command == "get_server_information")
		{
			body += "<server version=\"";
			body += PACKAGE_NAME;
			body += "\" />";
		}
		else if (command == "get_status")
		{
			FrontendThreadList& frontend_threads = stream_manager.get_frontend_threads();
			for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
			{
				FrontendThread* frontend_thread = *i;

				body += "<frontend path=\"" + frontend_thread->frontend.get_path() + "\">";
				ChannelStreamList& streams = frontend_thread->get_streams();
				for (ChannelStreamList::iterator j = streams.begin(); j != streams.end(); j++)
				{
					ChannelStream* stream = *j;
					body += Glib::ustring::compose("<stream channel_id=\"%1\" type=\"%2\" description=\"%3\">",
						stream->channel.id, stream->type, stream->get_description());
					Dvb::DemuxerList& demuxers = stream->demuxers;
					for (Dvb::DemuxerList::iterator k = demuxers.begin(); k != demuxers.end(); k++)
					{
						Dvb::Demuxer* demuxer = *k;
						Glib::ustring type = "None";
						if (demuxer->filter_type == Dvb::Demuxer::FILTER_TYPE_PES)
						{
							type = "PES";
						}
						else if (demuxer->filter_type == Dvb::Demuxer::FILTER_TYPE_SECTION)
						{
							type = "SECTION";
						}
						body += Glib::ustring::compose("<demuxer pid=\"%1\" filter_type=\"%2\" />", demuxer->pid, type);
					}
					body += "</stream>";
				}
				body += "</frontend>";
			}
		}
		else if (command == "get_channels")
		{
			ChannelList channels = channel_manager.get_all();
			for (ChannelList::iterator i = channels.begin(); i != channels.end(); i++)
			{
				Channel& channel = *i;
				body += Glib::ustring::compose("<channel id=\"%1\" name=\"%2\" record_extra_before=\"%3\" record_extra_after=\"%4\">",
					channel.id, encode_xml(channel.name), channel.record_extra_before, channel.record_extra_after);
				EpgEvent epg_event;
				if (EpgEvents::get_current(channel.id, epg_event))
				{
					body += Glib::ustring::compose(
						"<event id=\"%1\" channel_id=\"%2\" start_time=\"%3\" duration=\"%4\" title=\"%5\" subtitle=\"%6\" description=\"%7\" scheduled_recording_id=\"%8\" />",
						epg_event.id,
						epg_event.channel_id,
						epg_event.start_time,
						epg_event.duration,
						encode_xml(epg_event.get_title()),
						encode_xml(epg_event.get_subtitle()),
						encode_xml(epg_event.get_description()),
						scheduled_recording_manager.is_recording(epg_event));
				}
				body += "</channel>";
			}
		}
		else if (command == "set_channel")
		{
			guint id = get_int_attribute_value(root_node, "parameter[@name=\"id\"]/@value");
			Glib::ustring name = get_attribute_value(root_node, "parameter[@name=\"name\"]/@value");
			guint sort_order = get_int_attribute_value(root_node, "parameter[@name=\"sort_order\"]/@value");
			gint record_extra_before = get_int_attribute_value(root_node, "parameter[@name=\"record_extra_before\"]/@value");
			gint record_extra_after = get_int_attribute_value(root_node, "parameter[@name=\"record_extra_after\"]/@value");
			if (name.empty() || sort_order == 0 || record_extra_before < 0 || record_extra_after < 0)
			{
				throw Exception("Invalid parameters");
			}

			channel_manager.set_channel(id, name, sort_order, record_extra_before, record_extra_after);
		}
		else if (command == "get_epg")
		{
			guint start_time = get_int_attribute_value(root_node, "parameter[@name=\"start_time\"]/@value");
			guint end_time = get_int_attribute_value(root_node, "parameter[@name=\"end_time\"]/@value");

			ChannelList channels = channel_manager.get_all();
			for (ChannelList::iterator i = channels.begin(); i != channels.end(); i++)
			{
				Channel& channel = *i;
				body += Glib::ustring::compose("<channel id=\"%1\" name=\"%2\">", channel.id, encode_xml(channel.name));

				EpgEventList epg_events = EpgEvents::get_all(start_time, end_time);
				for (EpgEventList::iterator i = epg_events.begin(); i != epg_events.end(); i++)
				{
					EpgEvent& epg_event = *i;

					if (epg_event.channel_id == channel.id)
					{
						body += Glib::ustring::compose(
							"<event id=\"%1\" channel_id=\"%2\" start_time=\"%3\" duration=\"%4\" title=\"%5\" subtitle=\"%6\" description=\"%7\" scheduled_recording_id=\"%8\" />",
							epg_event.id,
							epg_event.channel_id,
							epg_event.start_time,
							epg_event.duration,
							encode_xml(epg_event.get_title()),
							encode_xml(epg_event.get_subtitle()),
							encode_xml(epg_event.get_description()),
							scheduled_recording_manager.is_recording(epg_event));
					}
				}

				body += "</channel>";
			}
		}
		else if (command == "start_broadcasting")
		{
			int channel_id = ::atoi(get_attribute_value(root_node, "parameter[@name=\"channel\"]/@value").c_str());
			Channel channel = channel_manager.get(channel_id);
			Glib::ustring protocol = "udp";

			RequestHandler::Client& client = clients.get(client_id);
			stream_manager.start_broadcasting(channel, client_id, "", broadcast_address, client.broadcast_port);
			body += Glib::ustring::compose("<stream protocol=\"%1\" address=\"%2\" port=\"%3\" />",
				protocol, broadcast_address, client.broadcast_port);
		}
		else if (command == "stop_broadcasting")
		{
			stream_manager.stop_broadcasting(client_id);
		}
		else if (command == "add_scheduled_recording")
		{
			int epg_event_id = ::atoi(get_attribute_value(root_node, "parameter[@name=\"epg_event_id\"]/@value").c_str());
			EpgEvent epg_event = EpgEvents::get(epg_event_id);
			scheduled_recording_manager.add_scheduled_recording(epg_event);
			scheduled_recording_manager.check_scheduled_recordings();
		}
		else if (command == "remove_scheduled_recording")
		{
			int scheduled_recording_id = ::atoi(get_attribute_value(root_node, "parameter[@name=\"scheduled_recording_id\"]/@value").c_str());
			scheduled_recording_manager.remove_scheduled_recording(scheduled_recording_id);
			scheduled_recording_manager.check_scheduled_recordings();
		}
		else if (command == "get_scheduled_recordings")
		{
			ScheduledRecordingList recordings = scheduled_recording_manager.get_all();
			for (ScheduledRecordingList::iterator i = recordings.begin(); i != recordings.end(); i++)
			{
				ScheduledRecording& recording = *i;
				body += Glib::ustring::compose(
					"<scheduled_recording id=\"%1\" channel_id=\"%2\" recurring_type=\"%3\" start_time=\"%4\" duration=\"%5\" description=\"%6\" device=\"%7\" />",
					recording.id,
					recording.channel_id,
					recording.recurring_type,
					recording.start_time,
					recording.duration,
					encode_xml(recording.description),
					recording.device);
			}
		}
		else if (command == "add_channel")
		{
			Glib::ustring line = get_attribute_value(root_node, "parameter[@name=\"line\"]/@value"); 

			ChannelsConfLine channels_conf_line(line);
			guint parameter_count = channels_conf_line.get_parameter_count();
			
			g_debug("Line (%d parameters): '%s'", parameter_count, line.c_str());

			device_manager.check_frontend();

			Dvb::Frontend* frontend = *(device_manager.get_frontends().begin());
			Channel channel;
			channel.sort_order = 0;
			channel.transponder.frontend_type = frontend->get_frontend_type();
			channel.record_extra_before = 5;
			channel.record_extra_after = 10;
		
			switch(channel.transponder.frontend_type)
			{
				case FE_OFDM:
					if (parameter_count != 13)
					{
						throw Exception(_("Invalid parameter count"));
					}

					channel.name = channels_conf_line.get_name(0);
					channel.sort_order = 0;
		
					channel.transponder.frontend_parameters.frequency						= channels_conf_line.get_int(1);
					channel.transponder.frontend_parameters.inversion						= channels_conf_line.get_inversion(2);
					channel.transponder.frontend_parameters.u.ofdm.bandwidth				= channels_conf_line.get_bandwidth(3);
					channel.transponder.frontend_parameters.u.ofdm.code_rate_HP				= channels_conf_line.get_fec(4);
					channel.transponder.frontend_parameters.u.ofdm.code_rate_LP				= channels_conf_line.get_fec(5);
					channel.transponder.frontend_parameters.u.ofdm.constellation			= channels_conf_line.get_modulation(6);
					channel.transponder.frontend_parameters.u.ofdm.transmission_mode		= channels_conf_line.get_transmit_mode(7);
					channel.transponder.frontend_parameters.u.ofdm.guard_interval			= channels_conf_line.get_guard_interval(8);
					channel.transponder.frontend_parameters.u.ofdm.hierarchy_information	= channels_conf_line.get_hierarchy(9);
					channel.service_id														= channels_conf_line.get_service_id(12);
					break;
			
				case FE_QAM:
					if (parameter_count != 9)
					{
						throw Exception(_("Invalid parameter count"));
					}

					channel.name = channels_conf_line.get_name(0);
		
					channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_int(1);
					channel.transponder.frontend_parameters.inversion			= channels_conf_line.get_inversion(2);
					channel.transponder.frontend_parameters.u.qam.symbol_rate	= channels_conf_line.get_symbol_rate(3);
					channel.transponder.frontend_parameters.u.qam.fec_inner		= channels_conf_line.get_fec(4);
					channel.transponder.frontend_parameters.u.qam.modulation	= channels_conf_line.get_modulation(5);
					channel.service_id											= channels_conf_line.get_service_id(8);
					break;

				case FE_QPSK:
					if (parameter_count != 8)
					{
						throw Exception(_("Invalid parameter count"));
					}

					channel.name = channels_conf_line.get_name(0);
		
					channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_int(1)*1000;
					channel.transponder.polarisation							= channels_conf_line.get_polarisation(2);
					channel.transponder.satellite_number						= channels_conf_line.get_int(3);
					channel.transponder.frontend_parameters.u.qpsk.symbol_rate	= channels_conf_line.get_int(4) * 1000;
					channel.transponder.frontend_parameters.u.qpsk.fec_inner	= FEC_AUTO;
					channel.transponder.frontend_parameters.inversion			= INVERSION_AUTO;
					channel.service_id											= channels_conf_line.get_service_id(7);
					break;

				case FE_ATSC:
					if (parameter_count != 6)
					{
						throw Exception(_("Invalid parameter count"));
					}

					channel.name = channels_conf_line.get_name(0);
		
					channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_int(1);
					channel.transponder.frontend_parameters.inversion			= INVERSION_AUTO;
					channel.transponder.frontend_parameters.u.vsb.modulation	= channels_conf_line.get_modulation(2);
					channel.service_id											= channels_conf_line.get_service_id(5);
					break;
			
				default:
					throw Exception(_("Failed to import: importing a channels.conf is only supported with DVB-T, DVB-C, DVB-S and ATSC"));
			}

			channel_manager.add_channel(channel);
		
			body += "<success />";
		}
		else if (command == "remove_channel")
		{
			int channel_id = get_int_attribute_value(root_node, "parameter[@name=\"channel_id\"]/@value");
			Channel channel = channel_manager.get(channel_id);
			scheduled_recording_manager.remove_scheduled_recording(channel);
			channel_manager.remove_channel(channel_id);
		}
		else if (command == "search_epg")
		{
			Glib::ustring text = get_attribute_value(root_node, "parameter[@name=\"text\"]/@value");
			gboolean include_description = get_bool_attribute_value(root_node, "parameter[@name=\"include_description\"]/@value");

			EpgEventList epg_events = EpgEvents::search(text, include_description);
			for (EpgEventList::iterator i = epg_events.begin(); i != epg_events.end(); i++)
			{
				EpgEvent& epg_event = *i;
				body += Glib::ustring::compose(
					"<event id=\"%1\" channel_id=\"%2\" start_time=\"%3\" duration=\"%4\" title=\"%5\" subtitle=\"%6\" description=\"%7\" scheduled_recording_id=\"%8\" />",
					epg_event.id,
					epg_event.channel_id,
					epg_event.start_time,
					epg_event.duration,
					encode_xml(epg_event.get_title()),
					encode_xml(epg_event.get_subtitle()),
					encode_xml(epg_event.get_description()),
					scheduled_recording_manager.is_recording(epg_event));
			}
		}
		else if (command == "get_auto_record_list")
		{
			Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
				"select * from auto_record order by priority");
			Glib::RefPtr<DataModelIter> iter = model->create_iter();
				
			while (iter->move_next())
			{
				body += Glib::ustring::compose("<auto_record title=\"%1\" />", encode_xml(Data::get(iter, "title")));
			}
		}
		else if (command == "set_auto_record_list")
		{
			int priority = 0;

			NodeSet nodes = root_node->find("auto_record");
			data_connection->statement_execute_non_select("delete from autorecord");
			for (NodeSet::iterator i = nodes.begin(); i != nodes.end(); i++)
			{
				Node* node = *i;
				String title = get_attribute_value(node, "@title");
				replace_text(title, "'", "''");
				data_connection->statement_execute_non_select(String::compose(
					"insert into auto_record (title, priority) values ('%1',%2)",
					title, priority));
			}
		}
		else if (command == "get_configuration")
		{
			Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
				"select * from configuration");
			Glib::RefPtr<DataModelIter> iter = model->create_iter();
				
			while (iter->move_next())
			{
				body += Glib::ustring::compose("<configuration name=\"%1\" value=\"%2\" />",
					encode_xml(Data::get(iter, "name")), encode_xml(Data::get(iter, "value")));
			}
		}
		else if (command == "set_configuration")
		{
			NodeSet nodes = root_node->find("configuration");
			data_connection->statement_execute_non_select("delete from configuration");
			for (NodeSet::iterator i = nodes.begin(); i != nodes.end(); i++)
			{
				Node* node = *i;
				String name = get_attribute_value(node, "@name");
				String value = get_attribute_value(node, "@value");
				replace_text(value, "'", "''");
				data_connection->statement_execute_non_select(String::compose(
					"insert into configuration (name, value) values ('%1',%2)",
					name, value));
			}
		}
		else if (command == "terminate")
		{
			result = true;
		}
		else
		{
			throw Exception("Unknown command");
		}
	}

	send_response(sockfd, "", body);
	
	return result;
}

void RequestHandler::send_response(int sockfd,
	const Glib::ustring& error_message, const Glib::ustring& body)
{
	Glib::ustring response = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
	response += "<response";			
	if (!error_message.empty())
	{
		response += " error=\"" + error_message + "\"";
	}
	response += ">";
	response += body;
	response += "</response>";

	write_string(sockfd, response);
	::close(sockfd);
}

gboolean RequestHandler::handle_request(int sockfd)
{
	gboolean result = false;

	try
	{
		result = handle_connection(sockfd);
	}
	catch (const Exception& exception)
	{
		send_response(sockfd, encode_xml(exception.what()), "");
	}
	catch (const Glib::Error& exception)
	{
		send_response(sockfd, encode_xml(exception.what()), "");
	}
	catch (const std::exception& exception)
	{
		send_response(sockfd, encode_xml(exception.what()), "");
	}
	catch (...)
	{
		send_response(sockfd, "Unhandled exception", "");
	}

	return result;
}

void RequestHandler::set_broadcast_address(const Glib::ustring& address)
{
	broadcast_address = address;
}


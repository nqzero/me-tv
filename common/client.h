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

#ifndef __ME_TV_CLIENT__
#define __ME_TV_CLIENT__

#include <libxml++/libxml++.h>
#include <giomm/socketclient.h>
#include <map>
#include "../common/me-tv-types.h"
#include "../common/i18n.h"

class Client
{
public:
	class EpgEvent
	{
	public:
		int id;
		int channel_id;
		int start_time;
		int duration;
		Glib::ustring title;
		Glib::ustring subtitle;
		Glib::ustring description;
		int scheduled_recording_id;

		Glib::ustring get_start_time_text() const;
		Glib::ustring get_duration_text() const;
	};

	typedef std::list<EpgEvent> EpgEventList;

	class Channel
	{
	public:
		int id;
		Glib::ustring name;
		gboolean record_extra_before;
		gboolean record_extra_after;
		EpgEventList epg_events;
	};

	class ChannelList : public std::list<Channel>
	{
	public:
		Channel& get_by_id(int channel_id);
	};

	class BroadcastingStream
	{
	public:
		Glib::ustring protocol;
		Glib::ustring address;
		int port;
	};

	class ScheduledRecording
	{
	public:
		int id;
		int recurring_type;
		int channel_id;
		Glib::ustring description;
		int start_time;
		int duration;
		Glib::ustring device;

		Glib::ustring get_start_time_text() const;
		Glib::ustring get_duration_text() const;
	};

	typedef std::list<ScheduledRecording> ScheduledRecordingList;
	typedef std::map<Glib::ustring, Glib::ustring> ConfigurationMap;

	class Demuxer
	{
	public:
		int pid;
		Glib::ustring filter_type;
	};

	typedef std::list<Demuxer> DemuxerList;

	class Frontend
	{
	public:
		class Stream
		{
		public:
			int channel_id;
			int type;
			DemuxerList demuxers;
		};

		typedef std::list<Stream> StreamList;

		Glib::ustring path;
		StreamList streams;
	};

	typedef std::list<Frontend> FrontendList;

private:
	class Parameter
	{
	public:
		Parameter(const Glib::ustring& n, const Glib::ustring& v) : name(n), value(v) {}
		Parameter(const Glib::ustring& n, int v) : name(n), value(Glib::ustring::compose("%1", v)) {}

		Glib::ustring name;
		Glib::ustring value;
	};

	class ParameterList : public std::list<Parameter>
	{
	public:
		void add(const Glib::ustring& n, const Glib::ustring& v) { push_back(Parameter(n, v)); }
		void add(const Glib::ustring& n, int v) { push_back(Parameter(n, v)); }
	};

	Glib::ustring host;
	int port;
	int client_id;
	int broadcasting_channel_id;
	xmlpp::DomParser parser;
	xmlpp::Node* send_request(const Glib::ustring& command);
	xmlpp::Node* send_request(const Glib::ustring& command, ParameterList& parameters);
	xmlpp::Node* send_request(const Glib::ustring& command, const Glib::ustring& innerXml);

public:
	Client();
	~Client();

	int get_client_id() const { return client_id; }
	int get_broadcasting_channel_id() const { return broadcasting_channel_id; }

	void terminate();

	void add_channel(const Glib::ustring& line);
	void set_channel(guint channel_id, const Glib::ustring& name,
		guint sort_order, gint record_extra_before, gint record_extra_after);
	void remove_channel(int channel_id);
	ChannelList get_channels();
	ChannelList get_epg(int start_time, int duration);

	ScheduledRecordingList get_scheduled_recordings();
	void add_scheduled_recording(int epg_event_id);
	void remove_scheduled_recording(int scheduled_recording_id);

	BroadcastingStream start_broadcasting(int channel_id, gboolean multicast);
	void stop_broadcasting();

	EpgEventList search_epg(const Glib::ustring& text, gboolean include_description);
	FrontendList get_status();
	
	gboolean register_client(const Glib::ustring& host, int port);
	void unregister_client();
	gboolean is_registered() const { return client_id != 0; }

	StringList get_auto_record_list();
	void set_auto_record_list(StringList& auto_record_list);

	ConfigurationMap get_configuration();
	void set_configuration(ConfigurationMap& configuration);
};

#endif

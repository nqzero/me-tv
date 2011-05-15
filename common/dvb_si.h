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
 
#ifndef __DVB_SI_H__
#define __DVB_SI_H__

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <linux/dvb/frontend.h>
#include "common.h"
#include "i18n.h"

#define DVB_SECTION_BUFFER_SIZE	16*1024
#define TS_PACKET_SIZE			188
#define PACKET_BUFFER_SIZE		50

#define PAT_PID		0x00
#define NIT_PID		0x10
#define SDT_PID		0x11
#define EIT_PID		0x12
#define PSIP_PID	0x1FFB

#define PAT_ID		0x00
#define PMT_ID		0x02
#define NIT_ID		0x40
#define SDT_ID		0x42
#define EIT_ID		0x4E
#define MGT_ID		0xC7
#define TVCT_ID		0xC8
#define CVCT_ID		0xC9
#define PSIP_EIT_ID	0xCB
#define STT_ID		0xCD

namespace Dvb
{
	namespace SI
	{
		class EventText
		{
		public:
			String language;
			String title;
			String subtitle;
			String description;
		};
		
		class EventTextMap : public std::map<String, EventText>
		{
		public:
			gboolean contains(const String& language);
		};
		
		class Event
		{
		public:
			Event();

			guint	event_id;
			guint	version_number;
			guint	start_time;
			gulong	duration;
			
			EventTextMap texts;
		};

		typedef std::list<Event> EventList;
		
		class EventInformationSection
		{
		public:
			u_int table_id;
			u_int section_syntax_indicator;
			u_int service_id;
			u_int version_number;
			u_int current_next_indicator;
			u_int section_number;
			u_int last_section_number;
			u_int transport_stream_id;
			u_int original_network_id;
			u_int segment_last_section_number;
			u_int last_table_id;
			unsigned long crc;
			std::vector<Event> events;
		};

		class Service
		{
		public:
			guint id;
			guint type;
			gboolean eit_schedule_flag;
			String provider_name;
			String name;
		};

		class ServiceDescriptionSection
		{
		public:
			guint transport_stream_id;
			gboolean epg_events_available;
			std::vector<Service> services;
		};

		class NetworkInformationSection
		{
		public:
			std::vector<Dvb::Transponder> transponders;
		};
		
		class SystemTimeTable
		{
		public:
			gulong system_time;
			guint GPS_UTC_offset;
			guint daylight_savings;
		};

		class MasterGuideTable
		{
		public:
			guint type;
			guint pid;
		};

		typedef std::vector<MasterGuideTable> MasterGuideTableArray;

		class VirtualChannel
		{
		public:
			String short_name;
			guint major_channel_number;
			guint minor_channel_number;
			guint channel_TSID;
			guint program_number;
			guint service_type;
			guint source_id;
		};

		class VirtualChannelTable
		{
		public:
			guint transport_stream_id;
			std::vector<VirtualChannel> channels;
		};

		class SectionParser
		{
		private:
			guchar buffer[DVB_SECTION_BUFFER_SIZE];
			String text_encoding;
			guint timeout;
				
			guint get_bits(const guchar* buffer, guint bitpos, gsize bitcount);
			String convert_iso6937(const guchar* buffer, gsize length);
			gsize decode_event_descriptor (const guchar* buffer, Event& event);
			gsize read_section(Demuxer& demuxer);
		
			fe_code_rate_t parse_fec_inner(guint bitmask);
		public:
			SectionParser(const String& encoding, guint read_timeout);
			
			gsize get_text(String& s, const guchar* buffer);
			const guchar* get_buffer() const { return buffer; };

			void parse_eis (Demuxer& demuxer, EventInformationSection& section);
			void parse_psip_eis (Demuxer& demuxer, EventInformationSection& section);
			void parse_psip_mgt(Demuxer& demuxer, MasterGuideTableArray& tables);
			void parse_psip_vct(Demuxer& demuxer, VirtualChannelTable& section);
			void parse_psip_stt(Demuxer& demuxer, SystemTimeTable& table);
			void parse_sds (Demuxer& demuxer, ServiceDescriptionSection& section);
			void parse_nis (Demuxer& demuxer, NetworkInformationSection& section);
		};
	}
}

#endif

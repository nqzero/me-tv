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
 
#include "dvb_si.h"
#include "exception.h"
#include "atsc_text.h"

#define CRC_BYTE_SIZE		4
#define SHORT_EVENT			0x4D
#define EXTENDED_EVENT		0x4E

#define GPS_EPOCH			315964800

using namespace Dvb;
using namespace Dvb::SI;

Dvb::SI::Event::Event()
{
	event_id = 0;
	start_time = 0;
	duration = 0;
	version_number = 0;
}

SectionParser::SectionParser(const Glib::ustring& encoding, guint t)
{
	text_encoding = encoding;
	timeout = t;
}

// this method takes a bitmask as supplied by the NIT-section (satellite delivery system descriptor) for the FEC_INNER and converts them to a linux-style FEC value according to EN 300 468, v 1.9.1
fe_code_rate_t SectionParser::parse_fec_inner(guint bitmask)
{
	switch(bitmask)
	{
		case 0x01:
			return FEC_1_2;
		case 0x02:
			return FEC_2_3;
		case 0x03:
			return FEC_3_4;
		case 0x04:
			return FEC_5_6;
		case 0x05:
			return FEC_7_8;
		case 0x06:
			return FEC_8_9;
		case 0x07:
			return FEC_AUTO; // EN 300 468 specifies this as "fec 3/5", which is apparently not supported by the linux kernel or is a typo. just setting to auto.
		case 0x08:
			return FEC_4_5;
		default:
			return FEC_AUTO;
	}
	
	return FEC_AUTO; // this should never happen, but it will not hurt.
}

void SectionParser::parse_sds (Demuxer& demuxer, ServiceDescriptionSection& section)
{
	Buffer buffer;
	demuxer.read_section(buffer, timeout);
	gsize section_length = buffer.get_length();
	
	guint offset = 3;
	section.transport_stream_id = buffer.get_bits(offset, 0, 16);
	offset += 8;
	
	while (offset < section_length - 4)
	{
		Service service;

		service.id = buffer.get_bits(offset, 0, 16);
		offset += 2;
		service.eit_schedule_flag = buffer.get_bits(offset, 6, 1) == 1;
		if (service.eit_schedule_flag)
		{
			section.epg_events_available = true;
		}
		offset++;

		guint descriptors_loop_length = buffer.get_bits(offset, 4, 12);
		offset += 2;
		guint descriptors_end_offset = offset + descriptors_loop_length;
		while (offset < descriptors_end_offset)
		{
			guint descriptor_tag = buffer[offset];
			guint descriptor_length = buffer[offset + 1];
			if (descriptor_tag == 0x48)
			{
				service.type = buffer[offset + 2];
				guint service_provider_name_length = get_text(service.provider_name, buffer.get_buffer() + offset + 3);
				get_text(service.name, buffer.get_buffer() + offset + 3 + service_provider_name_length);
			}

			offset += descriptor_length + 2;
		}
		
		section.services.push_back(service);
	}
}

void SectionParser::parse_nis (Demuxer& demuxer, NetworkInformationSection& section)
{
	Buffer buffer;
	demuxer.read_section(buffer, timeout);
	gsize section_length = buffer.get_length();
	
	guint offset = 8;
	guint network_descriptor_length = buffer.get_bits(offset, 4, 12);
	offset += 2;
	
	// We don't care for the network descriptors, as we only want to get new frequencies out of the stream, and they are saved in the "Transport stream descriptor" section.
	offset += network_descriptor_length;
	
	// Now offset is at "transport_stream_loop_length" position.
	guint transport_stream_length = buffer.get_bits(offset, 4, 12);
	offset += 2;
	
	// loop through all transport descriptors and pick out 0x43 descriptors, as they contain new frequencies.
	while (offset < section_length - 4)
	{
		offset += 4;
		guint descriptors_loop_length = buffer.get_bits(offset, 4, 12);
		offset += 2;
		guint descriptors_end_offset = offset + descriptors_loop_length;
		
		while (offset < descriptors_end_offset)
		{
			guint descriptor_tag = buffer[offset++];
			guint descriptor_length = buffer[offset++];

			if (descriptor_tag == 0x43)
			{
				g_debug("Found Satellite Delivery System Descriptor");

				struct dvb_frontend_parameters frontend_parameters;
				
				// extract data for new transponder, according to EN 300 468: dvb-s
				frontend_parameters.frequency = 0;
				for (int i=0; i<8; i++)
				{
					frontend_parameters.frequency = frontend_parameters.frequency*10 + buffer.get_bits(offset, i*4, 4);
				}
				frontend_parameters.frequency *= 10;
				
				guint polarisation = (buffer.get_bits(offset + 6, 2, 1) == 1) ? POLARISATION_VERTICAL : POLARISATION_HORIZONTAL;
				
				frontend_parameters.u.qpsk.symbol_rate = 0;
				for(int i=0; i<7; i++)
				{
					frontend_parameters.u.qpsk.symbol_rate = frontend_parameters.u.qpsk.symbol_rate*10 + buffer.get_bits(offset + 7, i*4, 4);
				}
				frontend_parameters.u.qpsk.symbol_rate *= 100;
				
				frontend_parameters.u.qpsk.fec_inner = parse_fec_inner(buffer.get_bits(offset + 7, 28, 4));
				frontend_parameters.inversion = INVERSION_AUTO;
				
				Transponder transponder;
				transponder.frontend_parameters = frontend_parameters;
				transponder.polarisation = polarisation;
				section.transponders.push_back(transponder);
				
				g_debug("New frequency: %d, New polarisation: %d, Symbol rate %d, fec_inner %d",
					frontend_parameters.frequency, transponder.polarisation,
					frontend_parameters.u.qpsk.symbol_rate, frontend_parameters.u.qpsk.fec_inner);
			}
			else if (descriptor_tag == 0x44)
			{
				Transponder transponder;

				g_debug("Found Cable Delivery System Descriptor");

				guint frequency = 0;
				for (int i=0; i<8; i++)
				{
					frequency = frequency*10 + buffer.get_bits(offset, i*4, 4);
				}
				frequency *= 10;

				// reserved_future_use (12)
				guint fec_outer = buffer.get_bits(offset, 44, 4);
				guint modulation = buffer.get_bits(offset, 48, 8);
				guint symbol_rate = buffer.get_bits(offset, 56, 28);
				guint fec_inner = buffer.get_bits(offset, 84, 4);

				g_debug("frequency: %d", frequency);
				g_debug("FEC_outer: %d", fec_outer);
				g_debug("modulation: %d", modulation);
				g_debug("symbol_rate: %d", symbol_rate);
				g_debug("FEC_inner: %d", fec_inner);

				transponder.frontend_parameters.frequency = frequency;
				transponder.frontend_parameters.inversion = INVERSION_AUTO;

				transponder.frontend_parameters.u.qam.symbol_rate = symbol_rate;

				switch (modulation)
				{
					case 1: transponder.frontend_parameters.u.qam.modulation = QAM_16; break;
					case 2: transponder.frontend_parameters.u.qam.modulation = QAM_32; break;
					case 3: transponder.frontend_parameters.u.qam.modulation = QAM_64; break;
					case 4: transponder.frontend_parameters.u.qam.modulation = QAM_128; break;
					case 5: transponder.frontend_parameters.u.qam.modulation = QAM_256; break;
					default: transponder.frontend_parameters.u.qam.modulation = QAM_AUTO;
				}

				switch (fec_inner)
				{
					case 1: transponder.frontend_parameters.u.qam.fec_inner = FEC_1_2; break;
					case 2: transponder.frontend_parameters.u.qam.fec_inner = FEC_2_3; break;
					case 3: transponder.frontend_parameters.u.qam.fec_inner = FEC_3_4; break;
					case 4: transponder.frontend_parameters.u.qam.fec_inner = FEC_5_6; break;
					case 5: transponder.frontend_parameters.u.qam.fec_inner = FEC_7_8; break;
					case 6: transponder.frontend_parameters.u.qam.fec_inner = FEC_8_9; break;
#ifdef FEC_3_5
					case 7: transponder.frontend_parameters.u.qam.fec_inner = FEC_3_5; break;
#endif
					case 8: transponder.frontend_parameters.u.qam.fec_inner = FEC_4_5; break;
#ifdef FEC_9_10
					case 9: transponder.frontend_parameters.u.qam.fec_inner = FEC_9_10; break;
#endif
					default: transponder.frontend_parameters.u.qam.fec_inner = FEC_AUTO;
				}

				section.transponders.push_back(transponder);
			}
			else if (descriptor_tag == 0x5A)
			{
				Transponder transponder;

				g_debug("Found Terrestrial Delivery System Descriptor");
				guint centre_frequency = buffer.get_bits(offset, 0, 32) * 10;
				offset += 4;

				guint bandwidth = buffer.get_bits(offset, 0, 3);
				// priority (1)
				// Time_Slicing_indicator (1)
				// MPE-FEC_indicator (1)
				// reserved_future_use (2)
				guint constellation = buffer.get_bits(offset, 9, 2);
				guint hierarchy_information = buffer.get_bits(offset, 11, 3);
				guint code_rate_HP = buffer.get_bits(offset, 14, 3);
				guint code_rate_LP = buffer.get_bits(offset, 17, 3);
				guint guard_interval = buffer.get_bits(offset, 20, 2);
				guint transmission_mode = buffer.get_bits(offset, 22, 2);

				g_debug("centre_frequency: %d", centre_frequency);
				g_debug("bandwidth: %d", bandwidth);
				g_debug("constellation: %d", constellation);
				g_debug("hierarchy_information: %d", hierarchy_information);				
				g_debug("code_rate_HP: %d", code_rate_HP);
				g_debug("code_rate_LP: %d", code_rate_LP);
				g_debug("guard_interval: %d", guard_interval);
				g_debug("transmission_mode: %d", transmission_mode);

				transponder.frontend_parameters.frequency = centre_frequency;
				transponder.frontend_parameters.inversion = INVERSION_AUTO;
					
				switch (bandwidth)
				{
					case 0: transponder.frontend_parameters.u.ofdm.bandwidth = BANDWIDTH_8_MHZ; break;
					case 1: transponder.frontend_parameters.u.ofdm.bandwidth = BANDWIDTH_7_MHZ; break;
					case 2: transponder.frontend_parameters.u.ofdm.bandwidth = BANDWIDTH_6_MHZ; break;
					default: transponder.frontend_parameters.u.ofdm.bandwidth = BANDWIDTH_AUTO;
				}

				switch (constellation)
				{
					case 1: transponder.frontend_parameters.u.ofdm.constellation = QAM_16; break;
					case 2: transponder.frontend_parameters.u.ofdm.constellation = QAM_64; break;
					default: transponder.frontend_parameters.u.ofdm.constellation = QAM_AUTO;
				}

				switch (hierarchy_information)
				{
					case 0: case 4: transponder.frontend_parameters.u.ofdm.hierarchy_information = HIERARCHY_NONE; break;
					case 1: case 5: transponder.frontend_parameters.u.ofdm.hierarchy_information = HIERARCHY_1; break;
					case 2: case 6: transponder.frontend_parameters.u.ofdm.hierarchy_information = HIERARCHY_2; break;
					case 3: case 7: transponder.frontend_parameters.u.ofdm.hierarchy_information = HIERARCHY_4; break;
					default: transponder.frontend_parameters.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
				}

				switch (code_rate_HP)
				{
					case 0: transponder.frontend_parameters.u.ofdm.code_rate_HP = FEC_1_2; break;
					case 1: transponder.frontend_parameters.u.ofdm.code_rate_HP = FEC_2_3; break;
					case 2: transponder.frontend_parameters.u.ofdm.code_rate_HP = FEC_3_4; break;
					case 3: transponder.frontend_parameters.u.ofdm.code_rate_HP = FEC_5_6; break;
					case 4: transponder.frontend_parameters.u.ofdm.code_rate_HP = FEC_7_8; break;
					default: transponder.frontend_parameters.u.ofdm.code_rate_HP = FEC_AUTO;
				}

				switch (code_rate_LP)
				{
					case 0: transponder.frontend_parameters.u.ofdm.code_rate_LP = FEC_1_2; break;
					case 1: transponder.frontend_parameters.u.ofdm.code_rate_LP = FEC_2_3; break;
					case 2: transponder.frontend_parameters.u.ofdm.code_rate_LP = FEC_3_4; break;
					case 3: transponder.frontend_parameters.u.ofdm.code_rate_LP = FEC_5_6; break;
					case 4: transponder.frontend_parameters.u.ofdm.code_rate_LP = FEC_7_8; break;
					default: transponder.frontend_parameters.u.ofdm.code_rate_LP = FEC_AUTO;
				}

				switch (guard_interval)
				{
					case 0: transponder.frontend_parameters.u.ofdm.guard_interval = GUARD_INTERVAL_1_32; break;
					case 1: transponder.frontend_parameters.u.ofdm.guard_interval = GUARD_INTERVAL_1_16; break;
					case 2: transponder.frontend_parameters.u.ofdm.guard_interval = GUARD_INTERVAL_1_8; break;
					case 3: transponder.frontend_parameters.u.ofdm.guard_interval = GUARD_INTERVAL_1_4; break;
					default: transponder.frontend_parameters.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
				}

				switch (transmission_mode)
				{
					case 0: transponder.frontend_parameters.u.ofdm.transmission_mode = TRANSMISSION_MODE_2K; break;
					case 1: transponder.frontend_parameters.u.ofdm.transmission_mode = TRANSMISSION_MODE_8K; break;
					default: transponder.frontend_parameters.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
				}

				section.transponders.push_back(transponder);
			}
			else
			{
				g_debug("Ignoring descriptor tag 0x%02X", descriptor_tag);
			}
			offset += descriptor_length;
		}
	}
	
	g_debug("transport_stream_length is %d, network_descriptor_length is %d and offset is %d", transport_stream_length, network_descriptor_length, offset);
}

gsize get_atsc_text(Glib::ustring& string, const guchar* buffer)
{
	size_t text_position = 0;
	gsize offset = 0;
	
	gsize number_strings = buffer[offset++];
	for (guint i = 0; i < number_strings; i++)
	{
		offset += 3;
		gsize number_segments = buffer[offset++];
		for (guint j = 0; j < number_segments; j++)
		{
			struct atsc_text_string_segment* segment = (struct atsc_text_string_segment*)&buffer[offset];
			
			uint8_t* text = NULL;
			size_t text_length = 0;
			offset += 2;
			offset += buffer[offset];
			offset++;
			
			atsc_text_segment_decode(segment, &text, &text_length, &text_position);

			if (text)
			{
				string.append((const gchar*)text, g_utf8_strlen((const gchar*)text, (gssize)text_position));
				free(text);
			}
		}
	}
	
	return offset;
}

void SectionParser::parse_psip_stt(Demuxer& demuxer, SystemTimeTable& table)
{
	Buffer buffer;
	demuxer.read_section(buffer, timeout);
	gsize section_length = buffer.get_length();

	guint offset = 9;
	table.system_time = buffer.get_bits(offset, 0, 32); offset += 4;
	table.GPS_UTC_offset = buffer[offset++];
	table.daylight_savings = buffer.get_bits(offset, 0, 16);
}

void SectionParser::parse_psip_vct(Demuxer& demuxer, VirtualChannelTable& section)
{
	Buffer buffer;
	demuxer.read_section(buffer, timeout);
	gsize section_length = buffer.get_length();

	guint offset = 3;
	section.transport_stream_id = buffer.get_bits(offset, 0, 16);
	offset += 6;
	guint num_channels_in_section = buffer[offset++];

	for (guint i = 0; i < num_channels_in_section; i++)
	{
		VirtualChannel vc;
		gchar* result = g_convert((const gchar*)buffer.get_buffer() + offset, 14, "UTF-8", "UTF-16BE", NULL, NULL, NULL);
		if (!result)
		{
			throw Exception(_("Failed to convert channel name"));
		}
		
		vc.short_name = result;
		g_free(result);
		offset += 14;
		vc.major_channel_number = buffer.get_bits(offset, 4, 10);
		vc.minor_channel_number = buffer.get_bits(offset, 14, 10);
		offset += 8;
		vc.channel_TSID = buffer.get_bits(offset, 0, 16);
		offset += 2;
		vc.program_number = buffer.get_bits(offset, 0, 16);
		offset += 3;
		vc.service_type = buffer[offset++]&0x3f;
		vc.source_id = buffer.get_bits(offset, 0, 16);
		section.channels.push_back(vc);
		offset += 2;
		guint table_type_descriptors_length = buffer.get_bits(offset, 6, 10);
		offset += table_type_descriptors_length + 2;
	}
}

void SectionParser::parse_psip_mgt(Demuxer& demuxer, MasterGuideTableArray& tables)
{
	Buffer buffer;
	demuxer.read_section(buffer, timeout);
	gsize section_length = buffer.get_length();

	guint offset = 9;
	guint tables_defined = buffer.get_bits(offset, 0, 16);
	offset += 2;
	
	for (guint i = 0; i < tables_defined; i++)
	{
		MasterGuideTable mgt;
		mgt.type = buffer.get_bits(offset, 0, 16);
		offset += 2;
		mgt.pid = buffer.get_bits(offset, 3, 13);
		tables.push_back(mgt);
		offset += 7;
		guint table_type_descriptors_length = buffer.get_bits(offset, 4, 12);
		offset += table_type_descriptors_length + 2;
	}
}

void SectionParser::parse_psip_eis(Demuxer& demuxer, EventInformationSection& section)
{
	Buffer buffer;
	demuxer.read_section(buffer, timeout);
	gsize section_length = buffer.get_length();

	guint offset = 3;

	section.service_id = buffer.get_bits(offset, 0, 16);
	section.version_number = buffer.get_bits(42, 5);
	
	offset += 6;
	guint num_events_in_section = buffer[offset++];
	
	for (guint i = 0; i < num_events_in_section && (offset < (section_length - CRC_BYTE_SIZE)); i++)
	{
		Event event;

		event.version_number	= section.version_number;
		event.event_id			= buffer.get_bits(offset, 2, 14); offset += 2;
		event.start_time		= buffer.get_bits(offset, 0, 32); offset += 4;
		event.duration			= buffer.get_bits(offset, 4, 20); offset += 3;

		event.start_time += GPS_EPOCH;
		
		gsize title_length = buffer[offset++];
		if (title_length > 0)
		{
			EventText event_text;
			get_atsc_text(event_text.title, buffer.get_buffer() + offset);
			event.texts[event_text.language] = event_text;
			offset += title_length;
		}
		
		guint descriptors_length = buffer.get_bits(offset, 4, 12);
		offset += 2 + descriptors_length;
	
		section.events.push_back(event);
	}
}

void SectionParser::parse_eis(Demuxer& demuxer, EventInformationSection& section)
{
	Buffer buffer;
	demuxer.read_section(buffer, timeout);
	gsize section_length = buffer.get_length();
	
	section.table_id =						buffer[0];
	section.section_syntax_indicator =		buffer.get_bits(8, 1);
	section.service_id =					buffer.get_bits(24, 16);
	section.version_number =				buffer.get_bits(42, 5);
	section.current_next_indicator =		buffer.get_bits(47, 1);
	section.section_number =				buffer.get_bits(48, 8);
	section.last_section_number =			buffer.get_bits(56, 8);
	section.transport_stream_id =			buffer.get_bits(64, 16);
	section.original_network_id =			buffer.get_bits(80, 16);
	section.segment_last_section_number =	buffer.get_bits(96, 8);
	section.last_table_id =					buffer.get_bits(104, 8);

	unsigned int offset = 14;

	while (offset < (section_length - CRC_BYTE_SIZE))
	{
		Event event;

		gulong	start_time_MJD;  // 16
		gulong	start_time_UTC;  // 24
		gulong	duration;

		event.version_number	= section.version_number;
		event.event_id			= buffer.get_bits(offset, 0, 16);
		start_time_MJD			= buffer.get_bits(offset, 16, 16);
		start_time_UTC			= buffer.get_bits(offset, 32, 24);
		duration				= buffer.get_bits(offset, 56, 24);
		
		unsigned int descriptors_loop_length  = buffer.get_bits(offset, 84, 12);
		offset += 12;
		unsigned int end_descriptor_offset = descriptors_loop_length + offset;

		guint event_dur_hour = ((duration >> 20)&0xf)*10+((duration >> 16)&0xf);
		guint event_dur_min =  ((duration >> 12)&0xf)*10+((duration >>  8)&0xf);
		guint event_dur_sec =  ((duration >>  4)&0xf)*10+((duration      )&0xf);

		event.duration = (event_dur_hour*60 + event_dur_min) * 60 + event_dur_sec;
				
		if (start_time_MJD > 0 && event.event_id != 0)
		{
			long year =  (long) ((start_time_MJD  - 15078.2) / 365.25);
			long month =  (long) ((start_time_MJD - 14956.1 - (long)(year * 365.25) ) / 30.6001);
			long day =  (long) (start_time_MJD - 14956 - (long)(year * 365.25) - (long)(month * 30.6001));

			struct tm t;

			memset(&t, 0, sizeof(struct tm));

			t.tm_sec	= ((start_time_UTC >>  4)&0xf)*10+((start_time_UTC      )&0xf);
			t.tm_min	= ((start_time_UTC >> 12)&0xf)*10+((start_time_UTC >>  8)&0xf);
			t.tm_hour	= ((start_time_UTC >> 20)&0xf)*10+((start_time_UTC >> 16)&0xf);
			t.tm_mday	= day;
			t.tm_mon	= month - 2;
			t.tm_year	= year;
		
			time_t utc_time = mktime(&t) - timezone;
			event.start_time = utc_time; // + (daylight * 60 * 60);

			while (offset < end_descriptor_offset)
			{
				offset += decode_event_descriptor(buffer.get_buffer() + offset, event);
			}

			if (offset > end_descriptor_offset)
			{
				throw Exception(_("ASSERT: offset > end_descriptor_offset"));
			}

			section.events.push_back(event);
		}
	}
	section.crc = buffer.get_bits(offset, 0, 32);
	offset += 4;
	
	if (offset > section_length)
	{
		throw Exception(_("ASSERT: offset > end_section_offset"));
	}
}

Glib::ustring get_lang_desc(const guchar* b)
{
	char c[4];
	Glib::ustring s;
	memset( mempcpy( c, b+2, 3 ), 0, 1 );
	s = c;
	return s;
}

gsize SectionParser::decode_event_descriptor (const guchar* event_buffer, Event& event)
{
	if (event_buffer[1] == 0)
	{
		return 2;
	}

	gsize descriptor_length = event_buffer[1] + 2;
	
	guint descriptor_tag = event_buffer[0];	
	switch(descriptor_tag)
	{		
	case EXTENDED_EVENT:
		{
			Glib::ustring language = get_lang_desc (event_buffer + 1);
			
			if (!event.texts.contains(language))
			{
				EventText event_text_temp;
				event.texts[language] = event_text_temp;
				event_text_temp.language = language;
			}
			EventText& event_text = event.texts[language];
			
			unsigned int offset = 6;
			guint length_of_items = event_buffer[offset++];
			while (length_of_items > offset - 7)
			{
				offset += get_text(event_text.title, &event_buffer[offset]);
				offset += get_text(event_text.description, &event_buffer[offset]);
			}
			offset += get_text(event_text.description, &event_buffer[offset]);
		}
		break;

	case SHORT_EVENT:
		{
			Glib::ustring language = get_lang_desc (event_buffer);
			
			if (!event.texts.contains(language))
			{
				EventText event_text_temp;
				event.texts[language] = event_text_temp;
				event_text_temp.language = language;
			}
			EventText& event_text = event.texts[language];

			event_text.language = language;
			unsigned int offset = 5;
			offset += get_text(event_text.title, &event_buffer[offset]);
			offset += get_text(event_text.subtitle, &event_buffer[offset]);
		}
		break;
	default:
		break;
	}

	return descriptor_length;
}

guint SectionParser::get_bits(const guchar* b, guint bitpos, gsize bitcount)
{
	gsize i;
	gsize val = 0;

	for (i = bitpos; i < bitcount + bitpos; i++)
	{
		val = val << 1;
		val = val + ((b[i >> 3] & (0x80 >> (i & 7))) ? 1 : 0);
	}
	return val;
}

gsize SectionParser::get_text(Glib::ustring& s, const guchar* text_buffer)
{
	gsize length = text_buffer[0];
	guint text_index = 0;
	gchar text[length];
	guint index = 0;
	const gchar* codeset = "ISO-8859-15";
	
	if (length > 0)
	{
		// Skip over length byte
		index++;

		if (text_encoding.length() > 0 && text_encoding != "auto")
		{
			codeset = text_encoding.c_str();
		}
		else // Determine codeset from stream
		{			
			if (text_buffer[index] < 0x20)
			{
				switch (text_buffer[index])
				{
				case 0x01: codeset = "ISO-8859-5"; break;
				case 0x02: codeset = "ISO-8859-6"; break;
				case 0x03: codeset = "ISO-8859-7"; break;
				case 0x04: codeset = "ISO-8859-8"; break;
				case 0x05: codeset = "ISO-8859-9"; break;
				case 0x06: codeset = "ISO-8859-10"; break;
				case 0x07: codeset = "ISO-8859-11"; break;
				case 0x08: codeset = "ISO-8859-12"; break;
				case 0x09: codeset = "ISO-8859-13"; break;
				case 0x0A: codeset = "ISO-8859-14"; break;
				case 0x0B: codeset = "ISO-8859-15"; break;
				case 0x11: codeset = "UTF-16BE"; break;
				case 0x14: codeset = "UTF-16BE"; break;

				case 0x10:
					{
						// Skip 0x00
						index++;
						if (text_buffer[index] != 0x00)
						{
							g_warning("Second byte of code table id was not 0");
						}
						
						index++;
						
						switch(text_buffer[index])
						{
						case 0x01: codeset = "ISO-8859-1"; break;
						case 0x02: codeset = "ISO-8859-2"; break;
						case 0x03: codeset = "ISO-8859-3"; break;
						case 0x04: codeset = "ISO-8859-4"; break;
						case 0x05: codeset = "ISO-8859-5"; break;
						case 0x06: codeset = "ISO-8859-6"; break;
						case 0x07: codeset = "ISO-8859-7"; break;
						case 0x08: codeset = "ISO-8859-8"; break;
						case 0x09: codeset = "ISO-8859-9"; break;
						case 0x0A: codeset = "ISO-8859-10"; break;
						case 0x0B: codeset = "ISO-8859-11"; break;
						case 0x0C: codeset = "ISO-8859-12"; break;
						case 0x0D: codeset = "ISO-8859-13"; break;
						case 0x0E: codeset = "ISO-8859-14"; break;
						case 0x0F: codeset = "ISO-8859-15"; break;
						default: break;
						}
					}
					default: break;
				}
				
				index++;
			}
		}
		
		if (text_encoding == "iso6937")
		{
			s += convert_iso6937(text_buffer + index, length);
		}
		else
		{
			if (index < (length + 1))
			{
				while (index < (length + 1))
				{
					u_char ch = text_buffer[index];

					if (strcmp(codeset, "UTF-16BE") == 0)
					{
						text[text_index++] = ch;
					}
					else
					{
						if (ch == 0x86 || ch == 0x87)
						{
							// Ignore formatting
						}
						else if (ch == 0x8A)
						{
							text[text_index++] = '\n';
						}
						else if (ch >= 0x80 && ch < 0xA0)
						{
							text[text_index++] = '.';
						}
						else
						{
							text[text_index++] = ch;
						}
					}
					
					index++;
				}
				
				gsize bytes_read;
				gsize bytes_written;
				GError* error = NULL;
				
				gchar* result = g_convert(
					text,
					text_index,
					"UTF-8",
					codeset,
					&bytes_read,
					&bytes_written,
					&error);
				
				if (error != NULL || result == NULL)
				{
					const gchar* error_message = _("No message");
					if (error != NULL)
					{
						error_message = error->message;
					}
					
					Glib::ustring message = Glib::ustring::compose(
						_("Failed to convert to UTF-8: %1"),
						Glib::ustring(error_message));
					g_debug("%s", message.c_str());
					g_debug("Codeset: %s", codeset);
					g_debug("Length: %zu", length);
					for (guint i = 0; i < (length+1); i++)
					{
						gchar ch = text_buffer[i];
						if (!isprint(ch))
						{
							g_debug("text_buffer[%d]\t= 0x%02X", i, ch);
						}
						else
						{
							g_debug("text_buffer[%d]\t= 0x%02X '%c'", i, ch, ch);
						}
					}
					throw Exception(message);
				}
				
				s += result;
				g_free(result);
			}
		}
	}
	
	return length + 1;
}

Glib::ustring SectionParser::convert_iso6937(const guchar* text_buffer, gsize length)
{
	Glib::ustring result;
	gint val;
	guint i;
	
	for (i=0; i<length; i++ )
	{
		if ( text_buffer[i]<0x20 || (text_buffer[i]>=0x80 && text_buffer[i]<=0x9f) )
		{
			continue; // control codes
		}
		
		if ( text_buffer[i]>=0xC0 && text_buffer[i]<=0xCF ) // next char has accent
		{
			if ( i==length-1 ) // Accent char not followed by char
			{
				continue;
			}
			
			val = ( text_buffer[i]<<8 ) + text_buffer[i+1];
			++i;
			
			switch (val)
			{
				case 0xc141: result += gunichar(0x00C0); break; //LATIN CAPITAL LETTER A WITH GRAVE
				case 0xc145: result += gunichar(0x00C8); break; //LATIN CAPITAL LETTER E WITH GRAVE
				case 0xc149: result += gunichar(0x00CC); break; //LATIN CAPITAL LETTER I WITH GRAVE
				case 0xc14f: result += gunichar(0x00D2); break; //LATIN CAPITAL LETTER O WITH GRAVE
				case 0xc155: result += gunichar(0x00D9); break; //LATIN CAPITAL LETTER U WITH GRAVE
				case 0xc161: result += gunichar(0x00E0); break; //LATIN SMALL LETTER A WITH GRAVE
				case 0xc165: result += gunichar(0x00E8); break; //LATIN SMALL LETTER E WITH GRAVE
				case 0xc169: result += gunichar(0x00EC); break; //LATIN SMALL LETTER I WITH GRAVE
				case 0xc16f: result += gunichar(0x00F2); break; //LATIN SMALL LETTER O WITH GRAVE
				case 0xc175: result += gunichar(0x00F9); break; //LATIN SMALL LETTER U WITH GRAVE
				case 0xc220: result += gunichar(0x00B4); break; //ACUTE ACCENT
				case 0xc241: result += gunichar(0x00C1); break; //LATIN CAPITAL LETTER A WITH ACUTE
				case 0xc243: result += gunichar(0x0106); break; //LATIN CAPITAL LETTER C WITH ACUTE
				case 0xc245: result += gunichar(0x00C9); break; //LATIN CAPITAL LETTER E WITH ACUTE
				case 0xc249: result += gunichar(0x00CD); break; //LATIN CAPITAL LETTER I WITH ACUTE
				case 0xc24c: result += gunichar(0x0139); break; //LATIN CAPITAL LETTER L WITH ACUTE
				case 0xc24e: result += gunichar(0x0143); break; //LATIN CAPITAL LETTER N WITH ACUTE
				case 0xc24f: result += gunichar(0x00D3); break; //LATIN CAPITAL LETTER O WITH ACUTE
				case 0xc252: result += gunichar(0x0154); break; //LATIN CAPITAL LETTER R WITH ACUTE
				case 0xc253: result += gunichar(0x015A); break; //LATIN CAPITAL LETTER S WITH ACUTE
				case 0xc255: result += gunichar(0x00DA); break; //LATIN CAPITAL LETTER U WITH ACUTE
				case 0xc259: result += gunichar(0x00DD); break; //LATIN CAPITAL LETTER Y WITH ACUTE
				case 0xc25a: result += gunichar(0x0179); break; //LATIN CAPITAL LETTER Z WITH ACUTE
				case 0xc261: result += gunichar(0x00E1); break; //LATIN SMALL LETTER A WITH ACUTE
				case 0xc263: result += gunichar(0x0107); break; //LATIN SMALL LETTER C WITH ACUTE
				case 0xc265: result += gunichar(0x00E9); break; //LATIN SMALL LETTER E WITH ACUTE
				case 0xc269: result += gunichar(0x00ED); break; //LATIN SMALL LETTER I WITH ACUTE
				case 0xc26c: result += gunichar(0x013A); break; //LATIN SMALL LETTER L WITH ACUTE
				case 0xc26e: result += gunichar(0x0144); break; //LATIN SMALL LETTER N WITH ACUTE
				case 0xc26f: result += gunichar(0x00F3); break; //LATIN SMALL LETTER O WITH ACUTE
				case 0xc272: result += gunichar(0x0155); break; //LATIN SMALL LETTER R WITH ACUTE
				case 0xc273: result += gunichar(0x015B); break; //LATIN SMALL LETTER S WITH ACUTE
				case 0xc275: result += gunichar(0x00FA); break; //LATIN SMALL LETTER U WITH ACUTE
				case 0xc279: result += gunichar(0x00FD); break; //LATIN SMALL LETTER Y WITH ACUTE
				case 0xc27a: result += gunichar(0x017A); break; //LATIN SMALL LETTER Z WITH ACUTE
				case 0xc341: result += gunichar(0x00C2); break; //LATIN CAPITAL LETTER A WITH CIRCUMFLEX
				case 0xc343: result += gunichar(0x0108); break; //LATIN CAPITAL LETTER C WITH CIRCUMFLEX
				case 0xc345: result += gunichar(0x00CA); break; //LATIN CAPITAL LETTER E WITH CIRCUMFLEX
				case 0xc347: result += gunichar(0x011C); break; //LATIN CAPITAL LETTER G WITH CIRCUMFLEX
				case 0xc348: result += gunichar(0x0124); break; //LATIN CAPITAL LETTER H WITH CIRCUMFLEX
				case 0xc349: result += gunichar(0x00CE); break; //LATIN CAPITAL LETTER I WITH CIRCUMFLEX
				case 0xc34a: result += gunichar(0x0134); break; //LATIN CAPITAL LETTER J WITH CIRCUMFLEX
				case 0xc34f: result += gunichar(0x00D4); break; //LATIN CAPITAL LETTER O WITH CIRCUMFLEX
				case 0xc353: result += gunichar(0x015C); break; //LATIN CAPITAL LETTER S WITH CIRCUMFLEX
				case 0xc355: result += gunichar(0x00DB); break; //LATIN CAPITAL LETTER U WITH CIRCUMFLEX
				case 0xc357: result += gunichar(0x0174); break; //LATIN CAPITAL LETTER W WITH CIRCUMFLEX
				case 0xc359: result += gunichar(0x0176); break; //LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
				case 0xc361: result += gunichar(0x00E2); break; //LATIN SMALL LETTER A WITH CIRCUMFLEX
				case 0xc363: result += gunichar(0x0109); break; //LATIN SMALL LETTER C WITH CIRCUMFLEX
				case 0xc365: result += gunichar(0x00EA); break; //LATIN SMALL LETTER E WITH CIRCUMFLEX
				case 0xc367: result += gunichar(0x011D); break; //LATIN SMALL LETTER G WITH CIRCUMFLEX
				case 0xc368: result += gunichar(0x0125); break; //LATIN SMALL LETTER H WITH CIRCUMFLEX
				case 0xc369: result += gunichar(0x00EE); break; //LATIN SMALL LETTER I WITH CIRCUMFLEX
				case 0xc36a: result += gunichar(0x0135); break; //LATIN SMALL LETTER J WITH CIRCUMFLEX
				case 0xc36f: result += gunichar(0x00F4); break; //LATIN SMALL LETTER O WITH CIRCUMFLEX
				case 0xc373: result += gunichar(0x015D); break; //LATIN SMALL LETTER S WITH CIRCUMFLEX
				case 0xc375: result += gunichar(0x00FB); break; //LATIN SMALL LETTER U WITH CIRCUMFLEX
				case 0xc377: result += gunichar(0x0175); break; //LATIN SMALL LETTER W WITH CIRCUMFLEX
				case 0xc379: result += gunichar(0x0177); break; //LATIN SMALL LETTER Y WITH CIRCUMFLEX
				case 0xc441: result += gunichar(0x00C3); break; //LATIN CAPITAL LETTER A WITH TILDE
				case 0xc449: result += gunichar(0x0128); break; //LATIN CAPITAL LETTER I WITH TILDE
				case 0xc44e: result += gunichar(0x00D1); break; //LATIN CAPITAL LETTER N WITH TILDE
				case 0xc44f: result += gunichar(0x00D5); break; //LATIN CAPITAL LETTER O WITH TILDE
				case 0xc455: result += gunichar(0x0168); break; //LATIN CAPITAL LETTER U WITH TILDE
				case 0xc461: result += gunichar(0x00E3); break; //LATIN SMALL LETTER A WITH TILDE
				case 0xc469: result += gunichar(0x0129); break; //LATIN SMALL LETTER I WITH TILDE
				case 0xc46e: result += gunichar(0x00F1); break; //LATIN SMALL LETTER N WITH TILDE
				case 0xc46f: result += gunichar(0x00F5); break; //LATIN SMALL LETTER O WITH TILDE
				case 0xc475: result += gunichar(0x0169); break; //LATIN SMALL LETTER U WITH TILDE
				case 0xc520: result += gunichar(0x00AF); break; //MACRON
				case 0xc541: result += gunichar(0x0100); break; //LATIN CAPITAL LETTER A WITH MACRON
				case 0xc545: result += gunichar(0x0112); break; //LATIN CAPITAL LETTER E WITH MACRON
				case 0xc549: result += gunichar(0x012A); break; //LATIN CAPITAL LETTER I WITH MACRON
				case 0xc54f: result += gunichar(0x014C); break; //LATIN CAPITAL LETTER O WITH MACRON
				case 0xc555: result += gunichar(0x016A); break; //LATIN CAPITAL LETTER U WITH MACRON
				case 0xc561: result += gunichar(0x0101); break; //LATIN SMALL LETTER A WITH MACRON
				case 0xc565: result += gunichar(0x0113); break; //LATIN SMALL LETTER E WITH MACRON
				case 0xc569: result += gunichar(0x012B); break; //LATIN SMALL LETTER I WITH MACRON
				case 0xc56f: result += gunichar(0x014D); break; //LATIN SMALL LETTER O WITH MACRON
				case 0xc575: result += gunichar(0x016B); break; //LATIN SMALL LETTER U WITH MACRON
				case 0xc620: result += gunichar(0x02D8); break; //BREVE
				case 0xc641: result += gunichar(0x0102); break; //LATIN CAPITAL LETTER A WITH BREVE
				case 0xc647: result += gunichar(0x011E); break; //LATIN CAPITAL LETTER G WITH BREVE
				case 0xc655: result += gunichar(0x016C); break; //LATIN CAPITAL LETTER U WITH BREVE
				case 0xc661: result += gunichar(0x0103); break; //LATIN SMALL LETTER A WITH BREVE
				case 0xc667: result += gunichar(0x011F); break; //LATIN SMALL LETTER G WITH BREVE
				case 0xc675: result += gunichar(0x016D); break; //LATIN SMALL LETTER U WITH BREVE
				case 0xc720: result += gunichar(0x02D9); break; //DOT ABOVE (Mandarin Chinese light tone)
				case 0xc743: result += gunichar(0x010A); break; //LATIN CAPITAL LETTER C WITH DOT ABOVE
				case 0xc745: result += gunichar(0x0116); break; //LATIN CAPITAL LETTER E WITH DOT ABOVE
				case 0xc747: result += gunichar(0x0120); break; //LATIN CAPITAL LETTER G WITH DOT ABOVE
				case 0xc749: result += gunichar(0x0130); break; //LATIN CAPITAL LETTER I WITH DOT ABOVE
				case 0xc75a: result += gunichar(0x017B); break; //LATIN CAPITAL LETTER Z WITH DOT ABOVE
				case 0xc763: result += gunichar(0x010B); break; //LATIN SMALL LETTER C WITH DOT ABOVE
				case 0xc765: result += gunichar(0x0117); break; //LATIN SMALL LETTER E WITH DOT ABOVE
				case 0xc767: result += gunichar(0x0121); break; //LATIN SMALL LETTER G WITH DOT ABOVE
				case 0xc77a: result += gunichar(0x017C); break; //LATIN SMALL LETTER Z WITH DOT ABOVE
				case 0xc820: result += gunichar(0x00A8); break; //DIAERESIS
				case 0xc841: result += gunichar(0x00C4); break; //LATIN CAPITAL LETTER A WITH DIAERESIS
				case 0xc845: result += gunichar(0x00CB); break; //LATIN CAPITAL LETTER E WITH DIAERESIS
				case 0xc849: result += gunichar(0x00CF); break; //LATIN CAPITAL LETTER I WITH DIAERESIS
				case 0xc84f: result += gunichar(0x00D6); break; //LATIN CAPITAL LETTER O WITH DIAERESIS
				case 0xc855: result += gunichar(0x00DC); break; //LATIN CAPITAL LETTER U WITH DIAERESIS
				case 0xc859: result += gunichar(0x0178); break; //LATIN CAPITAL LETTER Y WITH DIAERESIS
				case 0xc861: result += gunichar(0x00E4); break; //LATIN SMALL LETTER A WITH DIAERESIS
				case 0xc865: result += gunichar(0x00EB); break; //LATIN SMALL LETTER E WITH DIAERESIS
				case 0xc869: result += gunichar(0x00EF); break; //LATIN SMALL LETTER I WITH DIAERESIS
				case 0xc86f: result += gunichar(0x00F6); break; //LATIN SMALL LETTER O WITH DIAERESIS
				case 0xc875: result += gunichar(0x00FC); break; //LATIN SMALL LETTER U WITH DIAERESIS
				case 0xc879: result += gunichar(0x00FF); break; //LATIN SMALL LETTER Y WITH DIAERESIS
				case 0xca20: result += gunichar(0x02DA); break; //RING ABOVE
				case 0xca41: result += gunichar(0x00C5); break; //LATIN CAPITAL LETTER A WITH RING ABOVE
				case 0xca55: result += gunichar(0x016E); break; //LATIN CAPITAL LETTER U WITH RING ABOVE
				case 0xca61: result += gunichar(0x00E5); break; //LATIN SMALL LETTER A WITH RING ABOVE
				case 0xca75: result += gunichar(0x016F); break; //LATIN SMALL LETTER U WITH RING ABOVE
				case 0xcb20: result += gunichar(0x00B8); break; //CEDILLA
				case 0xcb43: result += gunichar(0x00C7); break; //LATIN CAPITAL LETTER C WITH CEDILLA
				case 0xcb47: result += gunichar(0x0122); break; //LATIN CAPITAL LETTER G WITH CEDILLA
				case 0xcb4b: result += gunichar(0x0136); break; //LATIN CAPITAL LETTER K WITH CEDILLA
				case 0xcb4c: result += gunichar(0x013B); break; //LATIN CAPITAL LETTER L WITH CEDILLA
				case 0xcb4e: result += gunichar(0x0145); break; //LATIN CAPITAL LETTER N WITH CEDILLA
				case 0xcb52: result += gunichar(0x0156); break; //LATIN CAPITAL LETTER R WITH CEDILLA
				case 0xcb53: result += gunichar(0x015E); break; //LATIN CAPITAL LETTER S WITH CEDILLA
				case 0xcb54: result += gunichar(0x0162); break; //LATIN CAPITAL LETTER T WITH CEDILLA
				case 0xcb63: result += gunichar(0x00E7); break; //LATIN SMALL LETTER C WITH CEDILLA
				case 0xcb67: result += gunichar(0x0123); break; //LATIN SMALL LETTER G WITH CEDILLA
				case 0xcb6b: result += gunichar(0x0137); break; //LATIN SMALL LETTER K WITH CEDILLA
				case 0xcb6c: result += gunichar(0x013C); break; //LATIN SMALL LETTER L WITH CEDILLA
				case 0xcb6e: result += gunichar(0x0146); break; //LATIN SMALL LETTER N WITH CEDILLA
				case 0xcb72: result += gunichar(0x0157); break; //LATIN SMALL LETTER R WITH CEDILLA
				case 0xcb73: result += gunichar(0x015F); break; //LATIN SMALL LETTER S WITH CEDILLA
				case 0xcb74: result += gunichar(0x0163); break; //LATIN SMALL LETTER T WITH CEDILLA
				case 0xcd20: result += gunichar(0x02DD); break; //DOUBLE ACUTE ACCENT
				case 0xcd4f: result += gunichar(0x0150); break; //LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
				case 0xcd55: result += gunichar(0x0170); break; //LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
				case 0xcd6f: result += gunichar(0x0151); break; //LATIN SMALL LETTER O WITH DOUBLE ACUTE
				case 0xcd75: result += gunichar(0x0171); break; //LATIN SMALL LETTER U WITH DOUBLE ACUTE
				case 0xce20: result += gunichar(0x02DB); break; //OGONEK
				case 0xce41: result += gunichar(0x0104); break; //LATIN CAPITAL LETTER A WITH OGONEK
				case 0xce45: result += gunichar(0x0118); break; //LATIN CAPITAL LETTER E WITH OGONEK
				case 0xce49: result += gunichar(0x012E); break; //LATIN CAPITAL LETTER I WITH OGONEK
				case 0xce55: result += gunichar(0x0172); break; //LATIN CAPITAL LETTER U WITH OGONEK
				case 0xce61: result += gunichar(0x0105); break; //LATIN SMALL LETTER A WITH OGONEK
				case 0xce65: result += gunichar(0x0119); break; //LATIN SMALL LETTER E WITH OGONEK
				case 0xce69: result += gunichar(0x012F); break; //LATIN SMALL LETTER I WITH OGONEK
				case 0xce75: result += gunichar(0x0173); break; //LATIN SMALL LETTER U WITH OGONEK
				case 0xcf20: result += gunichar(0x02C7); break; //CARON (Mandarin Chinese third tone)
				case 0xcf43: result += gunichar(0x010C); break; //LATIN CAPITAL LETTER C WITH CARON
				case 0xcf44: result += gunichar(0x010E); break; //LATIN CAPITAL LETTER D WITH CARON
				case 0xcf45: result += gunichar(0x011A); break; //LATIN CAPITAL LETTER E WITH CARON
				case 0xcf4c: result += gunichar(0x013D); break; //LATIN CAPITAL LETTER L WITH CARON
				case 0xcf4e: result += gunichar(0x0147); break; //LATIN CAPITAL LETTER N WITH CARON
				case 0xcf52: result += gunichar(0x0158); break; //LATIN CAPITAL LETTER R WITH CARON
				case 0xcf53: result += gunichar(0x0160); break; //LATIN CAPITAL LETTER S WITH CARON
				case 0xcf54: result += gunichar(0x0164); break; //LATIN CAPITAL LETTER T WITH CARON
				case 0xcf5a: result += gunichar(0x017D); break; //LATIN CAPITAL LETTER Z WITH CARON
				case 0xcf63: result += gunichar(0x010D); break; //LATIN SMALL LETTER C WITH CARON
				case 0xcf64: result += gunichar(0x010F); break; //LATIN SMALL LETTER D WITH CARON
				case 0xcf65: result += gunichar(0x011B); break; //LATIN SMALL LETTER E WITH CARON
				case 0xcf6c: result += gunichar(0x013E); break; //LATIN SMALL LETTER L WITH CARON
				case 0xcf6e: result += gunichar(0x0148); break; //LATIN SMALL LETTER N WITH CARON
				case 0xcf72: result += gunichar(0x0159); break; //LATIN SMALL LETTER R WITH CARON
				case 0xcf73: result += gunichar(0x0161); break; //LATIN SMALL LETTER S WITH CARON
				case 0xcf74: result += gunichar(0x0165); break; //LATIN SMALL LETTER T WITH CARON
				case 0xcf7a: result += gunichar(0x017E); break; //LATIN SMALL LETTER Z WITH CARON
				default: break; // unknown
			}
		}
		else
		{
			switch ( text_buffer[i] )
			{
				case 0xa0: result += gunichar(0x00A0); break; //NO-BREAK SPACE
				case 0xa1: result += gunichar(0x00A1); break; //INVERTED EXCLAMATION MARK
				case 0xa2: result += gunichar(0x00A2); break; //CENT SIGN
				case 0xa3: result += gunichar(0x00A3); break; //POUND SIGN
				case 0xa5: result += gunichar(0x00A5); break; //YEN SIGN
				case 0xa7: result += gunichar(0x00A7); break; //SECTION SIGN
				case 0xa8: result += gunichar(0x00A4); break; //CURRENCY SIGN
				case 0xa9: result += gunichar(0x2018); break; //LEFT SINGLE QUOTATION MARK
				case 0xaa: result += gunichar(0x201C); break; //LEFT DOUBLE QUOTATION MARK
				case 0xab: result += gunichar(0x00AB); break; //LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
				case 0xac: result += gunichar(0x2190); break; //LEFTWARDS ARROW
				case 0xad: result += gunichar(0x2191); break; //UPWARDS ARROW
				case 0xae: result += gunichar(0x2192); break; //RIGHTWARDS ARROW
				case 0xaf: result += gunichar(0x2193); break; //DOWNWARDS ARROW
				case 0xb0: result += gunichar(0x00B0); break; //DEGREE SIGN
				case 0xb1: result += gunichar(0x00B1); break; //PLUS-MINUS SIGN
				case 0xb2: result += gunichar(0x00B2); break; //SUPERSCRIPT TWO
				case 0xb3: result += gunichar(0x00B3); break; //SUPERSCRIPT THREE
				case 0xb4: result += gunichar(0x00D7); break; //MULTIPLICATION SIGN
				case 0xb5: result += gunichar(0x00B5); break; //MICRO SIGN
				case 0xb6: result += gunichar(0x00B6); break; //PILCROW SIGN
				case 0xb7: result += gunichar(0x00B7); break; //MIDDLE DOT
				case 0xb8: result += gunichar(0x00F7); break; //DIVISION SIGN
				case 0xb9: result += gunichar(0x2019); break; //RIGHT SINGLE QUOTATION MARK
				case 0xba: result += gunichar(0x201D); break; //RIGHT DOUBLE QUOTATION MARK
				case 0xbb: result += gunichar(0x00BB); break; //RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
				case 0xbc: result += gunichar(0x00BC); break; //VULGAR FRACTION ONE QUARTER
				case 0xbd: result += gunichar(0x00BD); break; //VULGAR FRACTION ONE HALF
				case 0xbe: result += gunichar(0x00BE); break; //VULGAR FRACTION THREE QUARTERS
				case 0xbf: result += gunichar(0x00BF); break; //INVERTED QUESTION MARK
				case 0xd0: result += gunichar(0x2014); break; //EM DASH
				case 0xd1: result += gunichar(0x00B9); break; //SUPERSCRIPT ONE
				case 0xd2: result += gunichar(0x00AE); break; //REGISTERED SIGN
				case 0xd3: result += gunichar(0x00A9); break; //COPYRIGHT SIGN
				case 0xd4: result += gunichar(0x2122); break; //TRADE MARK SIGN
				case 0xd5: result += gunichar(0x266A); break; //EIGHTH NOTE
				case 0xd6: result += gunichar(0x00AC); break; //NOT SIGN
				case 0xd7: result += gunichar(0x00A6); break; //BROKEN BAR
				case 0xdc: result += gunichar(0x215B); break; //VULGAR FRACTION ONE EIGHTH
				case 0xdd: result += gunichar(0x215C); break; //VULGAR FRACTION THREE EIGHTHS
				case 0xde: result += gunichar(0x215D); break; //VULGAR FRACTION FIVE EIGHTHS
				case 0xdf: result += gunichar(0x215E); break; //VULGAR FRACTION SEVEN EIGHTHS
				case 0xe0: result += gunichar(0x2126); break; //OHM SIGN
				case 0xe1: result += gunichar(0x00C6); break; //LATIN CAPITAL LETTER AE
				case 0xe2: result += gunichar(0x00D0); break; //LATIN CAPITAL LETTER ETH (Icelandic)
				case 0xe3: result += gunichar(0x00AA); break; //FEMININE ORDINAL INDICATOR
				case 0xe4: result += gunichar(0x0126); break; //LATIN CAPITAL LETTER H WITH STROKE
				case 0xe6: result += gunichar(0x0132); break; //LATIN CAPITAL LIGATURE IJ
				case 0xe7: result += gunichar(0x013F); break; //LATIN CAPITAL LETTER L WITH MIDDLE DOT
				case 0xe8: result += gunichar(0x0141); break; //LATIN CAPITAL LETTER L WITH STROKE
				case 0xe9: result += gunichar(0x00D8); break; //LATIN CAPITAL LETTER O WITH STROKE
				case 0xea: result += gunichar(0x0152); break; //LATIN CAPITAL LIGATURE OE
				case 0xeb: result += gunichar(0x00BA); break; //MASCULINE ORDINAL INDICATOR
				case 0xec: result += gunichar(0x00DE); break; //LATIN CAPITAL LETTER THORN (Icelandic)
				case 0xed: result += gunichar(0x0166); break; //LATIN CAPITAL LETTER T WITH STROKE
				case 0xee: result += gunichar(0x014A); break; //LATIN CAPITAL LETTER ENG (Sami)
				case 0xef: result += gunichar(0x0149); break; //LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
				case 0xf0:	result += gunichar(0x0138); break; //LATIN SMALL LETTER KRA (Greenlandic)
				case 0xf1:	result += gunichar(0x00E6); break; //LATIN SMALL LETTER AE
				case 0xf2:	result += gunichar(0x0111); break; //LATIN SMALL LETTER D WITH STROKE
				case 0xf3:	result += gunichar(0x00F0); break; //LATIN SMALL LETTER ETH (Icelandic)
				case 0xf4:	result += gunichar(0x0127); break; //LATIN SMALL LETTER H WITH STROKE
				case 0xf5:	result += gunichar(0x0131); break; //LATIN SMALL LETTER DOTLESS I
				case 0xf6:	result += gunichar(0x0133); break; //LATIN SMALL LIGATURE IJ
				case 0xf7:	result += gunichar(0x0140); break; //LATIN SMALL LETTER L WITH MIDDLE DOT
				case 0xf8:	result += gunichar(0x0142); break; //LATIN SMALL LETTER L WITH STROKE
				case 0xf9:	result += gunichar(0x00F8); break; //LATIN SMALL LETTER O WITH STROKE
				case 0xfa:	result += gunichar(0x0153); break; //LATIN SMALL LIGATURE OE
				case 0xfb:	result += gunichar(0x00DF); break; //LATIN SMALL LETTER SHARP S (German)
				case 0xfc:	result += gunichar(0x00FE); break; //LATIN SMALL LETTER THORN (Icelandic)
				case 0xfd:	result += gunichar(0x0167); break; //LATIN SMALL LETTER T WITH STROKE
				case 0xfe:	result += gunichar(0x014B); break; //LATIN SMALL LETTER ENG (Sami)
				case 0xff: result += gunichar(0x00AD); break; //SOFT HYPHEN
				default: result += (char)text_buffer[i];
			}
		}
	}
	return result;
}

gboolean EventTextMap::contains(const Glib::ustring& language)
{
	for (const_iterator i = begin(); i != end(); i++)
	{
		if (i->first == language)
		{
			return true;
		}
	}
	
	return false;
}

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

#include <glibmm.h>
#include "mpeg_stream.h"
#include "exception.h"
#include "global.h"
#include "crc32.h"

Mpeg::Stream::Stream()
{
	g_debug("Creating MPEG stream");
	pmt_pid = 0;
	pcr_pid = 0;
	pat_counter = 0;
	pmt_counter = 0;
}

Mpeg::Stream::~Stream()
{
	g_debug("MPEG stream destroyed");
}

void Mpeg::Stream::clear()
{
	video_streams.clear();
	audio_streams.clear();
	subtitle_streams.clear();
	teletext_streams.clear();
}

gboolean Mpeg::Stream::is_pid_used(guint pid)
{
	gboolean used = false;
	guint index = 0;
	
	if (!video_streams.empty())
	{
		used = (pid==video_streams[0].pid);
	}
	
	while (index < audio_streams.size() && !used)
	{
		if (pid==audio_streams[index].pid)
		{
			used = true;
		}
		else
		{
			index++;
		}
	}

	index = 0;
	while (index < subtitle_streams.size() && !used)
	{
		if (pid==subtitle_streams[index].pid)
		{
			used = true;
		}
		else
		{
			index++;
		}
	}
	
	return used;
}

void Mpeg::Stream::build_pat(guchar* buffer)
{	
	buffer[0x00] = 0x47;
	buffer[0x01] = 0x40;
	buffer[0x02] = 0x00; // PID = 0x0000
	buffer[0x03] = 0x10 | (pat_counter++ & 0x0f);
	buffer[0x04] = 0x00; // CRC calculation begins here
	buffer[0x05] = 0x00; // 0x00: Program association section
	buffer[0x06] = 0xb0;
	buffer[0x07] = 0x11; // section_length
	buffer[0x08] = 0x00;
	buffer[0x09] = 0xbb; // TS id = 0x00b0
	buffer[0x0a] = 0xc1;
	// section # and last section #
	buffer[0x0b] = buffer[0x0c] = 0x00;
	// Network PID (useless)
	buffer[0x0d] = buffer[0x0e] = 0x00;
	buffer[0x0f] = 0xe0;
	buffer[0x10] = 0x10;
	
	// Program Map PID
	pmt_pid = 0xff;
	while (is_pid_used(pmt_pid))
	{
		pmt_pid--;
	}
	
	buffer[0x11] = 0x03;
	buffer[0x12] = 0xe8;
	buffer[0x13] = 0xe0;
	buffer[0x14] = pmt_pid;
	
	// Put CRC in buffer[0x15...0x18]
	guint crc32 = Crc32::calculate( (guchar*)buffer + 0x05, (guchar*)buffer + 0x15 );
	
	buffer[0x15] = (crc32 >> 24) & 0xff;
	buffer[0x16] = (crc32 >> 16) & 0xff;
	buffer[0x17] = (crc32 >>  8) & 0xff;
	buffer[0x18] = (crc32 >>  0) & 0xff;

	// needed stuffing bytes
	for (gint i=0x19; i < 188; i++)
	{
		buffer[i]=0xff;
	}
}

void Mpeg::Stream::build_pmt(guchar* buffer)
{
	int i, off=0;
	
	Mpeg::VideoStream video_stream;
	
	if (!video_streams.empty())
	{
		video_stream = video_streams[0];
	}

	buffer[0x00] = 0x47;
	buffer[0x01] = 0x40;
	buffer[0x02] = pmt_pid;
	buffer[0x03] = 0x10 | (pmt_counter++ & 0xf);
	buffer[0x04] = 0x00; // CRC calculation begins here
	buffer[0x05] = 0x02; // 0x02: Program map section
	buffer[0x06] = 0xb0;
	buffer[0x07] = 0x20; // section_length
	buffer[0x08] = 0x03;
	buffer[0x09] = 0xe8; // prog number
	buffer[0x0a] = 0xc1;
	// section # and last section #
	buffer[0x0b] = buffer[0x0c] = 0x00;
	// Program Clock Reference (PCR) PID
	buffer[0x0d] = pcr_pid>>8;
	buffer[0x0e] = pcr_pid&0xff;
	// program_info_length == 0
	buffer[0x0f] = 0xf0;
	buffer[0x10] = 0x00;
	// Video PID
	buffer[0x11] = video_stream.type; // video stream type
	buffer[0x12] = video_stream.pid>>8;
	buffer[0x13] = video_stream.pid&0xff;
	buffer[0x14] = 0xf0;
	buffer[0x15] = 0x09; // es info length
	// useless info
	buffer[0x16] = 0x07;
	buffer[0x17] = 0x04;
	buffer[0x18] = 0x08;
	buffer[0x19] = 0x80;
	buffer[0x1a] = 0x24;
	buffer[0x1b] = 0x02;
	buffer[0x1c] = 0x11;
	buffer[0x1d] = 0x01;
	buffer[0x1e] = 0xfe;
	off = 0x1e;

	// Audio streams
	for (guint index = 0; index < audio_streams.size(); index++)
	{
		Mpeg::AudioStream audio_stream = audio_streams[index];
		
		gchar language_code[4] = { 0 };
		
		strncpy(language_code, audio_stream.language.c_str(), 3);

		buffer[++off] = audio_stream.type;
		buffer[++off] = audio_stream.pid>>8;
		buffer[++off] = audio_stream.pid&0xff;

		if (audio_stream.type == STREAM_TYPE_AUDIO_AC3)
		{
			buffer[++off] = 0xf0;
			buffer[++off] = 0x0c; // es info length
			buffer[++off] = 0x05;
			buffer[++off] = 0x04;
			buffer[++off] = 0x41;
			buffer[++off] = 0x43;
			buffer[++off] = 0x2d;
			buffer[++off] = 0x33;
		}
		else
		{
			buffer[++off] = 0xf0;
			buffer[++off] = 0x06; // es info length
		}
		buffer[++off] = 0x0a; // iso639 descriptor tag
		buffer[++off] = 0x04; // descriptor length
		buffer[++off] = language_code[0];
		buffer[++off] = language_code[1];
		buffer[++off] = language_code[2];
		buffer[++off] = 0x00; // audio type
	}
	
	// Subtitle streams
	for (guint index = 0; index < subtitle_streams.size(); index++)
	{
		Mpeg::SubtitleStream subtitle_stream = subtitle_streams[index];
		
		gchar language_code[4] = { 0 };
		
		strncpy(language_code, subtitle_stream.language.c_str(), 3);
		
		buffer[++off] = subtitle_stream.type;
		buffer[++off] = subtitle_stream.pid>>8;
		buffer[++off] = subtitle_stream.pid&0xff;
		buffer[++off] = 0xf0;
		buffer[++off] = 0x0a; // es info length
		buffer[++off] = 0x59; // DVB sub tag
		buffer[++off] = 0x08; // descriptor length
		buffer[++off] = language_code[0];
		buffer[++off] = language_code[1];
		buffer[++off] = language_code[2];
		buffer[++off] = subtitle_stream.subtitling_type;
		buffer[++off] = subtitle_stream.composition_page_id>>8;
		buffer[++off] = subtitle_stream.composition_page_id&0xff;
		buffer[++off] = subtitle_stream.ancillary_page_id>>8;
		buffer[++off] = subtitle_stream.ancillary_page_id&0xff;
	}
	
	// TeleText streams
	for (guint index = 0; index < teletext_streams.size(); index++)
	{
		Mpeg::TeletextStream teletext_stream = teletext_streams[index];
		
		gint language_count = teletext_stream.languages.size();

		buffer[++off] = teletext_stream.type;
		buffer[++off] = teletext_stream.pid>>8;
		buffer[++off] = teletext_stream.pid&0xff;
		buffer[++off] = 0xf0;
		buffer[++off] = (language_count * 5) + 4;	// es info length
		buffer[++off] = 0x56;						// DVB teletext tag
		buffer[++off] = language_count * 5;			// descriptor length

		for (gint language_index = 0; language_index < language_count; language_index++)
		{
			Mpeg::TeletextLanguageDescriptor descriptor = teletext_stream.languages[language_index];
			gchar language_code[4] = { 0 };
			strncpy(language_code, descriptor.language.c_str(), 3);
			
			buffer[++off] = language_code[0];
			buffer[++off] = language_code[1];
			buffer[++off] = language_code[2];
			buffer[++off] = (descriptor.magazine_number & 0x06) | ((descriptor.type << 3) & 0xF8);
			buffer[++off] = descriptor.page_number;
		}
	}

	buffer[0x07] = off-3; // update section_length

	// Put CRC in ts[0x29...0x2c]
	guint crc32 = Crc32::calculate( (guchar*)buffer+0x05, (guchar*)buffer+off+1 );
		
	buffer[off+1] = (crc32 >> 24) & 0xff;
	buffer[off+2] = (crc32 >> 16) & 0xff;
	buffer[off+3] = (crc32 >>  8) & 0xff;
	buffer[off+4] = (crc32 >>  0) & 0xff;

	// needed stuffing bytes
	for (i=off+5 ; i < 188 ; i++)
	{
		buffer[i] = 0xff;
	}
}

void Mpeg::Stream::set_pmt_pid(const Buffer& buffer, guint service_id)
{
	gsize section_length = buffer.get_length();
	guint offset = 8;
	guint i = 0;

	pmt_pid = 0;
	g_debug("Searching for service ID %d", service_id);
	while (offset < (section_length - 4))
	{
		guint current_program_number = buffer.get_bits(offset, 0, 16);
		offset += 2;
		guint current_program_map_pid = buffer.get_bits(offset, 3, 13);
		offset += 2;

		g_debug("%d: Service ID: %d, PMT ID: %d", ++i, current_program_number, current_program_map_pid);

		if (service_id == current_program_number)
		{
			g_debug("Program Map ID found");
			pmt_pid = current_program_map_pid;
			break;
		}
	}

	if (pmt_pid == 0)
	{
		throw Exception(_("Failed to find Program Map PID for service"));
	}
}

void dump_descriptors(const unsigned char *buffer, int descriptors_loop_length)
{
	while (descriptors_loop_length > 0)
	{
		guchar descriptor_tag = buffer[0];
		guchar descriptor_length = buffer[1] + 2;

		if (!descriptor_length) break;

		g_debug("Descriptor tag: %d", descriptor_tag);

		buffer += descriptor_length;
		descriptors_loop_length -= descriptor_length;
	}
}

void Mpeg::Stream::parse_pms(const Buffer& buffer, gboolean ignore_teletext)
{	
	const guchar* desc = NULL;

	if (buffer[0] != 2)
	{
		throw Exception("Section is not a Program Mapping Section");
	}
	
	guint offset = 8;

	pcr_pid = ((buffer[8] & 0x1f) << 8) | buffer[9];
	g_debug("PCR PID: %d", pcr_pid);

	gsize program_info_length = ((buffer[10] & 0x0f) << 8) | buffer[11];

	offset += program_info_length + 4;

	guint section_length = buffer.get_length();
	while (section_length >= (5 + offset))
	{
		guint pid_type = buffer[offset];
		guint elementary_pid = ((buffer[offset+1] & 0x1f) << 8) | buffer[offset+2];
		gsize descriptor_length = ((buffer[offset+3] & 0x0f) << 8) | buffer[offset+4];
		
		switch (pid_type)
		{
		case STREAM_TYPE_MPEG1:
		case STREAM_TYPE_MPEG2:
		case STREAM_TYPE_MPEG4:
		case STREAM_TYPE_H264:
		case STREAM_TYPE_VIDEO:
			g_debug("Video PID: %d", elementary_pid);
			{
				VideoStream video_stream;
				video_stream.pid = elementary_pid;
				video_stream.type = pid_type;
				video_streams.push_back(video_stream);
			}
			break;
		case 0x03:
		case 0x04:
		case STREAM_TYPE_AUDIO_MPEG4:
		case STREAM_TYPE_AUDIO_AC3:
			g_debug("Audio PID: %d (Type: 0x%02X)", elementary_pid, pid_type);
			{
				AudioStream stream;
				stream.pid = elementary_pid;
				stream.type = pid_type;

				dump_descriptors(buffer.get_buffer() + offset + 5, descriptor_length);
				
				if (find_descriptor(0x0A, buffer.get_buffer() + offset + 5, descriptor_length, &desc, NULL))
				{
					stream.language = get_lang_desc (desc);
					g_debug("Language: %s", stream.language.c_str());
				}

				audio_streams.push_back(stream);
			}
			break;
		case 0x06:
			{
				int descriptor_loop_length = 0;

				dump_descriptors(buffer.get_buffer() + offset + 5, descriptor_length);

				if (find_descriptor(0x56, buffer.get_buffer() + offset + 5, descriptor_length, &desc, &descriptor_loop_length))
				{
					g_debug("Teletext PID: %d", elementary_pid);					
					TeletextStream stream;
					stream.type = pid_type;
					stream.pid = elementary_pid;

					gint descriptor_offset = desc - buffer.get_buffer();
					gint descriptor_index = 0;
					while (descriptor_index < descriptor_loop_length - 4)
					{
						TeletextLanguageDescriptor descriptor;
						
						descriptor.language			= get_lang_desc(desc + descriptor_index);
						descriptor.type				= buffer.get_bits(descriptor_offset + descriptor_index + 6, 0, 5);
						descriptor.magazine_number	= buffer.get_bits(descriptor_offset + descriptor_index + 6, 5, 3);
						descriptor.page_number		= buffer.get_bits(descriptor_offset + descriptor_index + 7, 0, 8);
						
						g_debug("TeleText: Language: '%s', Type: %d, Magazine Number: %d, Page Number: %d",
							descriptor.language.c_str(),
							descriptor.type,
							descriptor.magazine_number,
							descriptor.page_number);
						
						stream.languages.push_back(descriptor);
						
						descriptor_index += 5;
					}

					if (!ignore_teletext)
					{
						teletext_streams.push_back(stream);
					}
				}
				else if (find_descriptor (0x59, buffer.get_buffer() + offset + 5, descriptor_length, &desc, NULL))
				{
					g_debug("Subtitle PID: %d", elementary_pid);
					SubtitleStream stream;
					stream.pid = elementary_pid;
					stream.type = pid_type;

					gint descriptor_offest = desc - buffer.get_buffer();
					if (descriptor_length > 0)
					{
						stream.subtitling_type = buffer.get_bits(descriptor_offest + 5, 0, 8);
						if (stream.subtitling_type>=0x10 && stream.subtitling_type<=0x23)
						{
							stream.language				= get_lang_desc(desc);
							stream.composition_page_id	= buffer.get_bits(descriptor_offest + 6, 0, 16);
							stream.ancillary_page_id	= buffer.get_bits(descriptor_offest + 8, 0, 16);
							
							g_debug(
								"Subtitle composition_page_id: %d, ancillary_page_id: %d, language: %s",
								stream.composition_page_id, stream.ancillary_page_id, stream.language.c_str() );
						}
					}
					subtitle_streams.push_back(stream);
				}
				else if (find_descriptor (0x6A, buffer.get_buffer() + offset + 5, descriptor_length, NULL, NULL))
				{
					g_debug("AC3 PID Descriptor: %d", elementary_pid);
					AudioStream stream;
					stream.pid = elementary_pid;
					stream.type = STREAM_TYPE_AUDIO_AC3;

					desc = NULL;
					if (find_descriptor(0x0A, buffer.get_buffer() + offset + 5, descriptor_length, &desc, NULL))
					{
						stream.language = get_lang_desc (desc);
						g_debug("Language: %s", stream.language.c_str());
					}
					audio_streams.push_back(stream);
				}
			}
			break;
		default:
			g_debug("Unknown stream type: 0x%02X", pid_type);
			break;
		}
		
		offset += descriptor_length + 5;
	}
}

gboolean Mpeg::Stream::find_descriptor(guchar tag, const unsigned char *buf, int descriptors_loop_len, 
	const unsigned char **desc, int *desc_len)
{
	while (descriptors_loop_len > 0)
	{
		guchar descriptor_tag = buf[0];
		guchar descriptor_len = buf[1] + 2;

		if (!descriptor_len)
		{
			break;
		}

		if (tag == descriptor_tag)
		{
			if (desc) *desc = buf;
			if (desc_len) *desc_len = descriptor_len;
			return true;
		}

		buf += descriptor_len;
		descriptors_loop_len -= descriptor_len;
	}
	
	return false;
}

String Mpeg::Stream::get_lang_desc(const guchar* b)
{
	char c[4];
	memset( mempcpy( c, b+2, 3 ), 0, 1 );
	return c;
}

gboolean Mpeg::Stream::contains_pid(guint pid)
{
	guint index = 0;

	if (pid == pcr_pid) return true;

	for (index = 0; index < video_streams.size(); index++)
	{ if (video_streams[index].pid == pid) return true; }
	
	for (index = 0; index < audio_streams.size(); index++)
	{ if (audio_streams[index].pid == pid) return true; }

	for (index = 0; index < subtitle_streams.size(); index++)
	{ if (subtitle_streams[index].pid == pid) return true; }

	for (index = 0; index < teletext_streams.size(); index++)
	{ if (teletext_streams[index].pid == pid) return true; }

	return false;
}

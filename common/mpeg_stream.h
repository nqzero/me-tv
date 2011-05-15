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

#ifndef __MPEG_STREAM_H__
#define __MPEG_STREAM_H__

#define STREAM_TYPE_MPEG1		0x01
#define STREAM_TYPE_MPEG2		0x02
#define STREAM_TYPE_MPEG4		0x10
#define STREAM_TYPE_H264		0x1B
#define STREAM_TYPE_AUDIO_MPEG4	0x11
#define STREAM_TYPE_VIDEO		0x80
#define STREAM_TYPE_AUDIO_AC3	0x81

#include <linux/dvb/frontend.h>
#include "me-tv-types.h"
#include "i18n.h"
#include "buffer.h"

namespace Mpeg
{
	class PesStream
	{
	public:
		PesStream()
		{
			pid	= 0;
			type = 0;
		}
		
		guint pid;
		guint type;
	};
	
	class VideoStream : public PesStream
	{
	public:
		VideoStream()
		{
			type	= 2; // Default to MPEG 2
		}
	};

	class AudioStream : public PesStream
	{
	public:
		AudioStream()
		{
			language = _("Unknown language");
		}
		
		String	language;
	};

	class TeletextLanguageDescriptor
	{
	public:
		TeletextLanguageDescriptor()
		{
			type			= 0;
			language		= _("Unknown language");
			magazine_number	= 0;
			page_number		= 0;
		}

		guint			type;
		String	language;
		guint			magazine_number;
		guint			page_number;
	};

	class TeletextStream : public PesStream
	{
	public:
		std::vector<TeletextLanguageDescriptor> languages;
	};

	class SubtitleStream : public PesStream
	{
	public:
		SubtitleStream()
		{
			subtitling_type		= 0;
			ancillary_page_id	= 0;
			composition_page_id	= 0;
			language			= _("Unknown language");
		}

		guint subtitling_type;
		guint ancillary_page_id;
		guint composition_page_id;
		String language;
	};

	class Stream
	{
	private:
		guint pmt_pid;
		guint pcr_pid;

		gboolean is_pid_used(guint pid);
		gboolean find_descriptor(guchar tag, const unsigned char *buf, int descriptors_loop_len, const unsigned char **desc, int *desc_len);
		String get_lang_desc(const guchar* buffer);
		guint pat_counter;
		guint pmt_counter;
			
	public:
		Stream();
		~Stream();

		std::vector<VideoStream>	video_streams;
		std::vector<AudioStream>	audio_streams;
		std::vector<SubtitleStream>	subtitle_streams;
		std::vector<TeletextStream>	teletext_streams;

		guint get_pcr_pid() const { return pcr_pid; }
		guint get_pmt_pid() const { return pmt_pid; }
		void set_pmt_pid(const Buffer& buffer, guint service_id);
		void parse_pms(const Buffer& buffer, gboolean ignore_teletext);
		void build_pat(guchar* buffer);
		void build_pmt(guchar* buffer);
		gboolean contains_pid(guint pid);

		void clear();
	};
}

#endif

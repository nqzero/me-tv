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

#include "../common/i18n.h"
#include "dvb_demuxer.h"
#include "../common/exception.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

using namespace Dvb;

Demuxer::Demuxer(const Glib::ustring& device_path)
{
	fd = -1;
	filter_type = FILTER_TYPE_NONE;
	pid = -1;
	
	if ((fd = open(device_path.c_str(),O_RDWR|O_NONBLOCK)) < 0)
	{
		throw SystemException(_("Failed to open demux device"));
	}

	pfd[0].fd = fd;
	pfd[0].events = POLLIN | POLLOUT | POLLPRI;
}

Demuxer::~Demuxer()
{
	if (fd != -1)
	{
		stop();
		::close(fd);
		fd = -1;
	}
}

void Demuxer::set_pes_filter(uint16_t pid, dmx_pes_type_t pestype)
{
	struct dmx_pes_filter_params parameters;
	
	memset( &parameters, 0, sizeof( dmx_pes_filter_params ) );

	parameters.pid     = pid;
	parameters.input   = DMX_IN_FRONTEND;
	parameters.output  = DMX_OUT_TS_TAP;
	parameters.pes_type = pestype;
	parameters.flags   = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if (ioctl(fd, DMX_SET_PES_FILTER, &parameters) < 0)
	{
		throw SystemException(_("Failed to set PES filter"));
	}

	this->pid = pid;
	filter_type = FILTER_TYPE_PES;
}

void Demuxer::set_filter(ushort pid, ushort table_id, ushort mask)
{
	struct dmx_sct_filter_params parameters;

	memset( &parameters, 0, sizeof( dmx_sct_filter_params ) );
	
	parameters.pid = pid;
	parameters.timeout = 0;
	parameters.filter.filter[0] = table_id;
	parameters.filter.mask[0] = mask;
	parameters.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	g_debug("Demuxer::set_filter(%d,%d,%d)", pid, table_id, mask);
	if (ioctl(fd, DMX_SET_FILTER, &parameters) < 0)
	{
		throw SystemException(_("Failed to set section filter for demuxer"));
	}

	this->pid = pid;
	filter_type = FILTER_TYPE_SECTION;
}

void Demuxer::set_buffer_size(unsigned int buffer_size)
{
	if (ioctl(fd, DMX_SET_BUFFER_SIZE, buffer_size) < 0)
	{
		throw SystemException(_("Failed to set demuxer buffer size"));
	}
}

gint Demuxer::read(unsigned char* buffer, size_t length, gint timeout)
{
	if (!poll(timeout))
	{
		throw TimeoutException(_("Read timeout"));
	}

	gint bytes_read = ::read(fd, buffer, length);
	if (bytes_read == -1)
	{
		throw SystemException(_("Failed to read data from demuxer"));
	}
	
	return bytes_read;
}

void Demuxer::read_section(Buffer& buffer, gint timeout)
{
	guchar header[3];
	gsize bytes_read = read(header, 3, timeout);
	
	if (bytes_read != 3)
	{
		throw Exception(_("Failed to read header"));
	}
		
	gsize remaining_section_length = ((header[1] & 0x0f) << 8) + header[2];
	gsize section_length = remaining_section_length + 3;
	buffer.set_length(section_length);
	memcpy(buffer.get_buffer(), header, 3);
	bytes_read = read(buffer.get_buffer() + 3, remaining_section_length, timeout);
	
	if (bytes_read != remaining_section_length)
	{
		throw Exception(_("Failed to read section"));
	}

	if (buffer.crc32() != 0)
	{
		throw Exception(_("CRC32 check failed"));
	}
}

void Demuxer::stop()
{
	if (ioctl(fd, DMX_STOP) < 0)
	{
		throw SystemException(_("Failed to stop demuxer"));
	}
}

int Demuxer::get_fd() const
{
	return fd;
}

gboolean Demuxer::poll(gint timeout)
{
	gint result = ::poll(pfd, 1, timeout);
	
	if (result == -1)
	{
		throw SystemException (_("Failed to poll"));
	}
	
	return result > 0;
}

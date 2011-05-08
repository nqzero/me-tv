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
 
#include "buffer.h"
#include "crc32.h"

Buffer::Buffer()
{
	buffer = NULL;
	length = 0;
}

Buffer::Buffer(gsize l)
{
	buffer = new guchar[l];
	length = l;
}

Buffer::~Buffer()
{
	clear();
}

void Buffer::set_length(gsize l)
{
	if (l != length)
	{
		clear();
		buffer = new guchar[l];
		length = l;
	}
}

void Buffer::dump() const
{
	for (guint i = 0; i < length; i++)
	{
		guchar ch = buffer[i];
		if (g_ascii_isalnum(ch) || g_ascii_ispunct(ch))
		{
			g_debug ("buffer[%d] = 0x%02X; // (%c)", i, ch, ch);
		}
		else
		{
			g_debug ("buffer[%d] = 0x%02X;", i, ch);
		}
	}
}

void Buffer::clear()
{
	if (buffer != NULL)
	{	
		delete [] buffer;
	}
	
	buffer = NULL;
	length = 0;
}

guint Buffer::get_bits(guchar* buffer, guint position, gsize count)
{
	gsize val = 0;

	for (gsize i = position; i < count + position; i++)
	{
		val = val << 1;
		val = val + ((buffer[i >> 3] & (0x80 >> (i & 7))) ? 1 : 0);
	}
	
	return val;
}

guint Buffer::get_bits(guint offset, guint position, gsize count) const
{
	return get_bits(&buffer[offset], position, count);
}

guint Buffer::get_bits(guint position, gsize count) const
{
	return get_bits(buffer, position, count);
}

guint32 Buffer::crc32() const
{
	return Crc32::calculate(buffer, length);
}

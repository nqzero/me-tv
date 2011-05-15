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

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "dvb_transponder.h"
#include <linux/dvb/frontend.h>

class Channel
{
public:
	Channel();

	guint				id;
	String		name;
	guint				sort_order;
	String		mrl;
	gint				record_extra_before;
	gint				record_extra_after;
	
	guint				service_id;
	Dvb::Transponder	transponder;
	
	guint				get_transponder_frequency();
	String		get_text();

	bool operator==(const Channel& channel) const;
	bool operator!=(const Channel& channel) const;
};

class ChannelList: public std::list<Channel>
{
public:
	gboolean contains(guint channel_id);
};

#endif


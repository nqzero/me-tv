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

#ifndef __CHANNEL_MANAGER_H__
#define __CHANNEL_MANAGER_H__

#include "channel.h"
#include "data.h"

class ChannelManager
{
private:
	void load(Glib::RefPtr<DataModelIter>& iter, Channel& channel);

public:
	void remove_channel(guint channel_id);
	void add_channel(const Channel& channel);
	void set_channel(gint channel_id, const Glib::ustring& name,
		guint sort_order, guint record_extra_before, guint record_extra_after);

	Channel get(guint channel_id);
	gboolean find(Channel& channel, guint channel_id);
	gboolean find(Channel& channel, guint frequency, guint service_id);

	ChannelList get_all();
};

#endif

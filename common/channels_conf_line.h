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

#ifndef CHANNELS_CONF_LINE
#define CHANNELS_CONF_LINE

#include "global.h"
#include <linux/dvb/frontend.h>

struct StringTable
{
	const char*	text;
	guint		value;
};

class ChannelsConfLine
{
private:
	static struct StringTable bandwidth_table[];
	static struct StringTable fec_table[];
	static struct StringTable transmit_mode_table[];
	static struct StringTable guard_table[];
	static struct StringTable hierarchy_table[];
	static struct StringTable inversion_table[];	
	static struct StringTable modulation_table[];
	static struct StringTable polarisation_table[];
	
	std::vector<String> parts;
public:
	ChannelsConfLine(const String& line);
	
	guint get_parameter_count() const { return parts.size(); }
		
	const String&	get_name(guint index);
	fe_spectral_inversion_t	get_inversion(guint index);
	fe_bandwidth_t			get_bandwidth(guint index);
	fe_code_rate_t			get_fec(guint index);
	fe_modulation_t			get_modulation(guint index);
	fe_transmit_mode_t		get_transmit_mode(guint index);
	fe_guard_interval_t		get_guard_interval(guint index);
	fe_hierarchy_t			get_hierarchy(guint index);
	guint 					get_symbol_rate(guint index);
	guint 					get_service_id(guint index);
	guint					get_polarisation(guint index);
	guint					get_int(guint index);
};

#endif

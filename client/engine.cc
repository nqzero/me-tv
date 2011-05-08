/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Michael Lamothe 2010 <michael.lamothe@gmail.com>
 * 
 * gnome-media-player is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gnome-media-player is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "engine.h"
#include "../common/exception.h"
#include "me-tv.h"
#include <iomanip>

gboolean next;

Engine::~Engine()
{
}

void Engine::increment(gboolean forward, gboolean sh)
{
	int length = get_length();
	int time = get_time();

	if (time == 0 || length == 0)
	{
		float amount = sh ? 0.001 : 0.005;
		g_debug("%s %.2f percent", forward ? "Incrementing " : "Decrementing", amount * 100);
		set_percentage(get_percentage() + (forward ? amount : -amount));
	}
	else
	{
		int amount = sh ? 5000 : 30000;
		g_debug("%s %d seconds", forward ? "Incrementing" : "Decrementing", amount / 1000);
		set_time(get_time() + (forward ? amount : -amount));
	}
}

Glib::ustring Engine::format_2_digit(gint time)
{
	return Glib::ustring::format(std::setfill(L'0'), std::setw(2), time);
}

Glib::ustring Engine::format_time(gint time)
{
	Glib::ustring result;

	gint total_seconds = time / 1000;
	gint hours = total_seconds / 60 / 60;
	gint minutes = total_seconds / 60 % 60;
	gint seconds = total_seconds % 60;

	if (hours == 0)
	{
		result = Glib::ustring::compose("%1:%2", minutes, format_2_digit(seconds));
	}
	else
	{
		result = Glib::ustring::compose("%1:%2:%3", hours, format_2_digit(minutes), format_2_digit(seconds));
	}
	
	return result;
}

Glib::ustring Engine::get_text()
{
	int length = get_length();
	int time = get_time();
	Glib::ustring result;
	
	if (time == 0 || length == 0)
	{
		float percentage = get_percentage() * 100;
		if (percentage > 0.01)
		{
			result = Glib::ustring::compose("%1%%", (int)(percentage));
		}
	}
	else
	{
		result = Glib::ustring::compose("%1/%2", format_time(time), format_time(length));
	}

	return result;
}

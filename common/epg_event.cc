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

#include "epg_event.h"
#include "../common/i18n.h"
#include "../common/common.h"

EpgEventText::EpgEventText()
{
	id = 0;
	epg_event_id = 0;
}

EpgEvent::EpgEvent()
{
	id = 0;
	channel_id = 0;
	event_id = 0;
	start_time = 0;
	duration = 0;
	save = true;
}

Glib::ustring EpgEvent::get_title() const
{
	return get_default_text().title;
}

Glib::ustring EpgEvent::get_subtitle() const
{
	return get_default_text().subtitle;
}

Glib::ustring EpgEvent::get_description() const
{
	return get_default_text().description;
}

EpgEventText EpgEvent::get_default_text(const Glib::ustring& preferred_language) const
{
	EpgEventText result;
	gboolean found = false;
	
	for (EpgEventTextList::const_iterator i = texts.begin(); i != texts.end() && !found; i++)
	{
		EpgEventText text = *i;
		if (!preferred_language.empty() && preferred_language == text.language)
		{
			found = true;
			result = text;
		}
	}

	if (!found)
	{
		if (!texts.empty())
		{
			result = *(texts.begin());
		}
		else
		{
			result.title = _("Unknown title");	
			result.subtitle = _("Unknown subtitle");
			result.description = _("Unknown description");
		}
	}
	
	return result;
}

Glib::ustring EpgEvent::get_start_time_text() const
{
	return get_local_time_text(start_time, "%A, %d %B %Y, %H:%M");
}

Glib::ustring EpgEvent::get_duration_text() const
{
	Glib::ustring result;
	guint hours = duration / (60*60);
	guint minutes = (duration % (60*60)) / 60;
	if (hours > 0)
	{
		result = Glib::ustring::compose(ngettext("1 hour", "%1 hours", hours), hours);
	}
	if (hours > 0 && minutes > 0)
	{
		result += ", ";
	}
	if (minutes > 0)
	{
		result += Glib::ustring::compose(ngettext("1 minute", "%1 minutes", minutes), minutes);
	}
	
	return result;
}

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

#include "i18n.h"
#include "device_manager.h"
#include "exception.h"

Glib::ustring DeviceManager::get_adapter_path(guint adapter)
{
	return Glib::ustring::compose("/dev/dvb/adapter%1", adapter);
}

Glib::ustring DeviceManager::get_frontend_path(guint adapter, guint frontend_index)
{
	return Glib::ustring::compose("/dev/dvb/adapter%1/frontend%2", adapter, frontend_index);
}
	
void DeviceManager::initialise(const Glib::ustring& devices)
{
	Glib::ustring frontend_path;
	
	g_debug("Scanning DVB devices ...");
	guint adapter_count = 0;
	Glib::ustring adapter_path = get_adapter_path(adapter_count);
	while (Gio::File::create_for_path(adapter_path)->query_exists())
	{
		// NB: This leaks but is low in memory and does not accumulate over time
		Dvb::Adapter* adapter = new Dvb::Adapter(adapter_count);
		
		guint frontend_count = 0;
		frontend_path = get_frontend_path(adapter_count, frontend_count);
		while (Gio::File::create_for_path(frontend_path)->query_exists())
		{
			if (devices.empty() || devices.find(frontend_path) != Glib::ustring::npos)
			{
				Dvb::Frontend* frontend = new Dvb::Frontend(*adapter, frontend_count);
				try
				{
					if (!is_frontend_supported(*frontend))
					{
						g_debug("Frontend not supported");
					}
					else
					{					
						frontends.push_back(frontend);

						Glib::ustring frontend_type = "Unknown";

						switch(frontend->get_frontend_type())
						{
						case FE_ATSC: frontend_type = "ATSC"; break;
						case FE_OFDM: frontend_type = "DVB-T"; break;
						case FE_QAM: frontend_type = "DVB-C"; break;
						case FE_QPSK: frontend_type = "DVB-S"; break;
						default: break;
						}
		
						g_message("Device: '%s' (%s) at \"%s\"",
							frontend->get_frontend_info().name,
							frontend_type.c_str(),
							frontend->get_path().c_str());
					}
				}
				catch(...)
				{
					g_debug("Failed to load '%s'", frontend_path.c_str());
				}
			}
			
			frontend_path = get_frontend_path(adapter_count, ++frontend_count);
		}

		adapter_path = get_adapter_path(++adapter_count);
	}
}

gboolean DeviceManager::is_frontend_supported(const Dvb::Frontend& test_frontend)
{
	gboolean result = false;

	switch(test_frontend.get_frontend_type())
	{
	case FE_ATSC:
	case FE_OFDM:	// DVB-T
	case FE_QAM:	// DVB-C
	case FE_QPSK:	// DVB-S
		result = true;
		break;
	default:
		result = false;
		break;
	}

	return result;
}

Dvb::Frontend* DeviceManager::find_frontend_by_path(const Glib::ustring& path)
{
	Dvb::Frontend* result = NULL;

	for (FrontendList::iterator iterator = frontends.begin(); iterator != frontends.end(); iterator++)
	{
		Dvb::Frontend* current_frontend = *iterator;
		if (current_frontend->get_path() == path)
		{
			result = current_frontend;
		}
	}
	
	return result;
}

void DeviceManager::check_frontend()
{
	if (frontends.empty())
	{
		throw Exception(_("There are no DVB devices available"));
	}
}

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

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif /* HAVE_CONFIG_H */

#include "me-tv-server.h"
#include "../common/common.h"
#include "../common/server.h"
#include <glibmm.h>
#include <giomm.h>

#define ME_TV_SUMMARY _("Me TV is a digital television viewer for GTK")
#define ME_TV_DESCRIPTION _("Me TV was developed for the modern digital lounge room with a PC for a media centre that is capable "\
	"of normal PC tasks (web surfing, word processing and watching TV).\n")

int main(int argc, char** argv)
{
	try
	{
		Glib::init();
		Gio::init();
		
		signal_error.connect(sigc::ptr_fun(&on_error));

		g_log_set_handler(G_LOG_DOMAIN,
			(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			log_handler, NULL);

		g_message("Me TV Server started");

		if (!Glib::thread_supported())
		{
			Glib::thread_init();
		}

		gint server_port = 1999;
		String server_host;

		Glib::OptionEntry verbose_option_entry;
		verbose_option_entry.set_long_name("verbose");
		verbose_option_entry.set_short_name('v');
		verbose_option_entry.set_description(_("Enable verbose messages"));

		Glib::OptionEntry disable_epg_thread_option_entry;
		disable_epg_thread_option_entry.set_long_name("disable-epg-thread");
		disable_epg_thread_option_entry.set_description(_("Disable the EPG thread.  Me TV will stop collecting EPG events."));

		Glib::OptionEntry devices_option_entry;
		devices_option_entry.set_long_name("devices");
		devices_option_entry.set_description(_("Only use the specified frontend devices (e.g. --devices=/dev/dvb/adapter0/frontend0,/dev/dvb/adapter0/frontend1)"));

		Glib::OptionEntry read_timeout_option_entry;
		read_timeout_option_entry.set_long_name("read-timeout");
		read_timeout_option_entry.set_description(_("How long to wait (in seconds) before timing out while waiting for data from demuxer (default 5)."));

		Glib::OptionEntry server_port_option_entry;
		server_port_option_entry.set_long_name("server-port");
		server_port_option_entry.set_description(_("The network port for clients to connect to (default 1999)."));

		Glib::OptionGroup option_group(PACKAGE_NAME, "", _("Show Me TV Client help options"));
		option_group.add_entry(verbose_option_entry, verbose_logging);
		option_group.add_entry(disable_epg_thread_option_entry, disable_epg_thread);
		option_group.add_entry(devices_option_entry, devices);
		option_group.add_entry(read_timeout_option_entry, read_timeout);
		option_group.add_entry(server_port_option_entry, server_port);

		Glib::OptionContext option_context;
		option_context.set_summary(ME_TV_SUMMARY);
		option_context.set_description(ME_TV_DESCRIPTION);
		option_context.set_main_group(option_group);
		option_context.parse(argc, argv);

		Server server(server_port);
		server.start();

		g_message("Me TV Server entering main loop");
		Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create();
		main_loop->run();

		server.stop();

		g_message("Me TV Server exiting");
	}
	catch(...)
	{
		g_message("An unhandled exception was generated");
		handle_error();
	}
	
	return 0;
}


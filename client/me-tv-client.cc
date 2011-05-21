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

#include <gdk/gdk.h>
#include <gst/gst.h>
#include <gconfmm.h>
#include <glib/gprintf.h>
#include <unique/unique.h>
#include <dbus/dbus-glib.h>

#include "main_window.h"
#include "me-tv-client.h"
#include "configuration_manager.h"
#include "../common/server.h"

#define ME_TV_SUMMARY _("Me TV is a digital television viewer for GTK")
#define ME_TV_DESCRIPTION _("Me TV was developed for the modern digital lounge room with a PC for a media centre that is capable "\
	"of normal PC tasks (web surfing, word processing and watching TV).\n")

bool							disable_epg				= false;
bool							safe_mode				= false;
bool							minimised_mode			= false;
bool							no_screensaver_inhibit	= false;
bool							quit_on_close			= false;
String							engine_type				= "vlc";
Glib::RefPtr<Gtk::UIManager>	ui_manager;
Client							client;

Glib::RefPtr<Gtk::ToggleAction> toggle_action_fullscreen;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_mute;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_record_current;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_visibility;

Glib::RefPtr<Gtk::Action> action_about;
Glib::RefPtr<Gtk::Action> action_auto_record;
Glib::RefPtr<Gtk::Action> action_channels;
Glib::RefPtr<Gtk::Action> action_change_view_mode;
Glib::RefPtr<Gtk::Action> action_decrease_volume;
Glib::RefPtr<Gtk::Action> action_epg_event_search;
Glib::RefPtr<Gtk::Action> action_increase_volume;
Glib::RefPtr<Gtk::Action> action_preferences;
Glib::RefPtr<Gtk::Action> action_present;
Glib::RefPtr<Gtk::Action> action_quit;
Glib::RefPtr<Gtk::Action> action_restart_server;
Glib::RefPtr<Gtk::Action> action_scheduled_recordings;

sigc::signal<void, int> signal_start_broadcasting;
sigc::signal<void> signal_stop_broadcasting;
sigc::signal<void, int> signal_add_scheduled_recording;
sigc::signal<void, int> signal_remove_scheduled_recording;

static UniqueResponse on_message_received (
	UniqueApp*			app,
	UniqueCommand		command,
	UniqueMessageData*  message,
	guint          		time_,
	gpointer       		user_data)
{
	UniqueResponse response = UNIQUE_RESPONSE_FAIL;

	switch (command)
    {
		case 1:
			action_present->activate();
			response = UNIQUE_RESPONSE_OK;
			break;
			
		default:
			break;
	}

	return response;
}

ComboBoxText::ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBox(cobject)
{
	list_store = Gtk::ListStore::create(columns);
	clear();
	set_model(list_store);
	pack_start(columns.column_text);
	set_active(0);
}

void ComboBoxText::append_text(const String& text, const String& value)
{
	Gtk::TreeModel::Row row = *list_store->append();
	row[columns.column_text] = text;
	row[columns.column_value] = value;
}

void ComboBoxText::set_active_text(const String& text)
{
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); i++)
	{
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_text] == text)
		{
			set_active(i);
		}
	}
}

void ComboBoxText::set_active_value(const String& value)
{
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); i++)
	{
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_value] == value)
		{
			set_active(i);
		}
	}
}

void ComboBoxText::clear_items()
{
	list_store->clear();
}

String ComboBoxText::get_active_text()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_text];
		}
	}
	
	throw Exception(_("Failed to get active text value"));
}

String ComboBoxText::get_active_value()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_value];
		}
	}
	
	throw Exception(_("Failed to get active text value"));
}

ComboBoxEntryText::ComboBoxEntryText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBoxEntryText(cobject)
{
}

GdkLock::GdkLock()
{
	gdk_threads_enter();
}

GdkLock::~GdkLock()
{
	gdk_threads_leave();
}

GdkUnlock::GdkUnlock()
{
	gdk_threads_leave();
}

GdkUnlock::~GdkUnlock()
{
	gdk_threads_enter();
}

IntComboBox::IntComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBox(cobject)
{
	list_store = Gtk::ListStore::create(columns);
	clear();
	set_model(list_store);
	pack_start(columns.column_int);
	set_size(0);
	set_active(0);
}

void IntComboBox::set_size(guint size)
{	
	g_debug("Setting integer combo box size to %d", size);

	list_store->clear();
	for (guint i = 0; i < size; i++)
	{
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_int] = (i+1);
		
		if (list_store->children().size() == 1)
		{
			set_active(0);
		}
	}
	
	if (size == 0)
	{
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_int] = 1;
	}
	
	set_sensitive(size > 1);
}

guint IntComboBox::get_size()
{
	return list_store->children().size();
}

guint IntComboBox::get_active_value()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_int];
		}
	}
	
	throw Exception(_("Failed to get active integer value"));
}

void on_error_gtk(const String& message)
{
	Gtk::MessageDialog dialog(message);
	dialog.set_title(PACKAGE_NAME);
	dialog.run();
}

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	g_printf("Me TV %s\n", VERSION);

	if (!XInitThreads())
	{
		throw Exception("XInitThreads() failed");
	}

	if (!Glib::thread_supported())
	{
		Glib::thread_init();
	}
	gdk_threads_init();

	gst_init(NULL, NULL);
	Gio::init();
	Gnome::Conf::init();
	Gtk::Main main(argc, argv);

	Glib::add_exception_handler(&handle_error);
	signal_error.connect(sigc::ptr_fun(&on_error_gtk));

	String broadcast_address;
	gint server_port = 1999;
	String server_host;
	gboolean non_unique = false;

	g_log_set_handler(G_LOG_DOMAIN,
		(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
		log_handler, NULL);

	Glib::OptionEntry verbose_option_entry;
	verbose_option_entry.set_long_name("verbose");
	verbose_option_entry.set_short_name('v');
	verbose_option_entry.set_description(_("Enable verbose messages."));

	Glib::OptionEntry safe_mode_option_entry;
	safe_mode_option_entry.set_long_name("safe-mode");
	safe_mode_option_entry.set_short_name('s');
	safe_mode_option_entry.set_description(_("Start in safe mode."));

	Glib::OptionEntry minimised_option_entry;
	minimised_option_entry.set_long_name("minimised");
	minimised_option_entry.set_short_name('m');
	minimised_option_entry.set_description(_("Start minimised."));

	Glib::OptionEntry non_unique_option_entry;
	non_unique_option_entry.set_long_name("non-unique");
	non_unique_option_entry.set_description(_("Don't check for existing instances of Me TV."));

	Glib::OptionEntry quit_on_close_option_entry;
	quit_on_close_option_entry.set_long_name("quit-on-close");
	quit_on_close_option_entry.set_short_name('q');
	quit_on_close_option_entry.set_description(_("Quit Me TV when the main window is closed."));

	Glib::OptionEntry disable_epg_thread_option_entry;
	disable_epg_thread_option_entry.set_long_name("disable-epg-thread");
	disable_epg_thread_option_entry.set_description(_("Disable the EPG thread."));

	Glib::OptionEntry disable_epg_option_entry;
	disable_epg_option_entry.set_long_name("disable-epg");
	disable_epg_option_entry.set_description(_("Stops the rendering of the EPG event buttons on the UI."));

	Glib::OptionEntry engine_option_entry;
	engine_option_entry.set_short_name('e');
	engine_option_entry.set_long_name("engine");
	engine_option_entry.set_description(_("Specify engine type xine, gstreamer or vlc (default: vlc)."));

	Glib::OptionEntry no_screensaver_inhibit_option_entry;
	no_screensaver_inhibit_option_entry.set_long_name("no-screensaver-inhibit");
	no_screensaver_inhibit_option_entry.set_description(_("Tells Me TV not to call the screensaver Inhibit/UnInhibit methods for GNOME Screensaver."));

	Glib::OptionEntry devices_option_entry;
	devices_option_entry.set_long_name("devices");
	devices_option_entry.set_description(_("Only use the specified frontend devices. (e.g. --devices=/dev/dvb/adapter0/frontend0,/dev/dvb/adapter0/frontend1)"));

	Glib::OptionEntry host_option_entry;
	host_option_entry.set_long_name("server-host");
	host_option_entry.set_description(_("Me TV server host. (default: localhost)"));

	Glib::OptionEntry port_option_entry;
	port_option_entry.set_long_name("server-port");
	port_option_entry.set_description(_("Me TV server port. (default: 1999)"));

	Glib::OptionEntry read_timeout_option_entry;
	read_timeout_option_entry.set_long_name("read-timeout");
	read_timeout_option_entry.set_description(_("How long to wait (in seconds) before timing out while waiting for data from demuxer. (default 5)"));

	Glib::OptionEntry broadcast_address_option_entry;
	broadcast_address_option_entry.set_long_name("broadcast-address");
	broadcast_address_option_entry.set_description(_("The network broadcast address to send video streams for clients to display (default 127.0.0.1)."));
	
	Glib::OptionGroup option_group(PACKAGE_NAME, "", _("Show Me TV help options"));
	option_group.add_entry(verbose_option_entry, verbose_logging);
	option_group.add_entry(safe_mode_option_entry, safe_mode);
	option_group.add_entry(minimised_option_entry, minimised_mode);
	option_group.add_entry(disable_epg_thread_option_entry, disable_epg_thread);
	option_group.add_entry(disable_epg_option_entry, disable_epg);
	option_group.add_entry(no_screensaver_inhibit_option_entry, no_screensaver_inhibit);
	option_group.add_entry(devices_option_entry, devices);
	option_group.add_entry(read_timeout_option_entry, read_timeout);
	option_group.add_entry(host_option_entry, server_host);
	option_group.add_entry(port_option_entry, server_port);
	option_group.add_entry(engine_option_entry, engine_type);
	option_group.add_entry(non_unique_option_entry, non_unique);
	option_group.add_entry(quit_on_close_option_entry, quit_on_close);
	option_group.add_entry(broadcast_address_option_entry, broadcast_address);

	Glib::OptionContext option_context;
	option_context.set_summary(ME_TV_SUMMARY);
	option_context.set_description(ME_TV_DESCRIPTION);
	option_context.set_main_group(option_group);
		
	try
	{
		option_context.parse(argc, argv);

		UniqueApp* unique_application = unique_app_new_with_commands(
			"lamothe.me-tv-client", NULL,
			"run", (UniqueCommand)1,
			(char*)NULL);

		if (unique_app_is_running(unique_application) && !non_unique)
		{
			g_debug("Me TV is already running");

			UniqueMessageData* message = unique_message_data_new();
			unique_app_send_message(unique_application, (UniqueCommand)1, message);
		}
		else
		{
			g_signal_connect(unique_application, "message-received", G_CALLBACK (on_message_received), NULL);

			Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file(PACKAGE_DATA_DIR"/me-tv/glade/me-tv.ui");
	
			toggle_action_fullscreen = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_fullscreen"));
			toggle_action_mute = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_mute"));
			toggle_action_record_current = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_record_current"));
			toggle_action_visibility = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_visibility"));

			action_about = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_about"));
			action_auto_record = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_auto_record"));
			action_channels = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_channels"));
			action_change_view_mode = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_change_view_mode"));
			action_epg_event_search = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_epg_event_search"));
			action_preferences = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_preferences"));
			action_present = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_present"));
			action_restart_server = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_restart_server"));
			action_quit = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_quit"));
			action_scheduled_recordings = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_scheduled_recordings"));
			action_increase_volume = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_increase_volume"));
			action_decrease_volume = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_decrease_volume"));

			Glib::RefPtr<Gtk::ActionGroup> action_group = Gtk::ActionGroup::create();
			action_group->add(toggle_action_record_current, Gtk::AccelKey("R"));
			action_group->add(toggle_action_fullscreen, Gtk::AccelKey("F"));
			action_group->add(toggle_action_mute, Gtk::AccelKey("M"));
			action_group->add(toggle_action_visibility);

			action_group->add(action_about, Gtk::AccelKey("F1"));
			action_group->add(action_auto_record);
			action_group->add(action_channels);
			action_group->add(action_change_view_mode, Gtk::AccelKey("V"));
			action_group->add(action_epg_event_search);
			action_group->add(action_preferences);
			action_group->add(action_present);
			action_group->add(action_restart_server);
			action_group->add(action_quit);
			action_group->add(action_scheduled_recordings);
			action_group->add(action_increase_volume, Gtk::AccelKey("plus"));
			action_group->add(action_decrease_volume, Gtk::AccelKey("minus"));

			action_group->add(Gtk::Action::create("action_file", _("_File")));
			action_group->add(Gtk::Action::create("action_view", _("_View")));
			action_group->add(Gtk::Action::create("action_video", _("_Video")));
			action_group->add(Gtk::Action::create("action_audio", _("_Audio")));
			action_group->add(Gtk::Action::create("action_help", _("_Help")));
	
			action_group->add(Gtk::Action::create("action_subtitle_streams", _("Subtitles")));
			action_group->add(Gtk::Action::create("action_audio_streams", _("_Streams")));
			action_group->add(Gtk::Action::create("action_audio_channels", _("_Channels")));

			Gtk::RadioButtonGroup radio_button_group_audio_channel;
			action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_both", _("_Both")));
			action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_left", _("_Left")));
			action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_right", _("_Right")));
	
			action_quit->signal_activate().connect(sigc::ptr_fun(Gtk::Main::quit));

			ui_manager = Gtk::UIManager::create();
			ui_manager->insert_action_group(action_group);
			
			GError *error = NULL;
			DBusGConnection* dbus_connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
			if (dbus_connection == NULL)
			{
				g_message(_("Failed to get DBus session"));
			}

			configuration_manager.initialise();

			Server* server = NULL;
			if (server_host.empty())
			{
				server = new Server(server_port, broadcast_address);
				server->start();
			}
			else
			{
				quit_on_close = true;
			}

			client.register_client();

			if (!client.is_registered())
			{
				throw Exception("Failed to communicate with Me TV server");
			}

			MainWindow* main_window = MainWindow::create(builder);

			if (!minimised_mode)
			{
				main_window->show_all();
			}

			Client::ChannelList channels = client.get_channels();
			if (channels.empty())
			{
					action_channels->activate();
			}

			Gtk::Main::run();

			g_debug("Main loop exited");
			if (server != NULL)
			{
				client.terminate();
				server->stop();
			}
			else
			{
				client.unregister_client();
			}
		}
	}
	catch (const Glib::Exception& exception)
	{
		handle_error();
	}
	
	return 0;
}

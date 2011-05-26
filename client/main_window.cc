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

#include "main_window.h"
#include "me-tv-client.h"
#include "channels_dialog.h"
#include "preferences_dialog.h"
#include "scheduled_recordings_dialog.h"
#include "epg_event_dialog.h"
#include "epg_event_search_dialog.h"
#include "auto_record_dialog.h"
#include "gstreamer_engine.h"
#include "xine_engine.h"
#include "vlc_engine.h"
#include "configuration_manager.h"
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#define MINUTES_PER_COLUMN				1
#define COLUMNS_PER_HOUR				(60 * MINUTES_PER_COLUMN)
#define UPDATE_INTERVAL					60
#define SECONDS_UNTIL_CHANNEL_CHANGE	3

String ui_info =
	"<ui>"
	"	<menubar name='menu_bar'>"
	"		<menu action='action_file'>"
	"			<menuitem action='action_restart_server'/>"
	"			<separator/>"
	"			<menuitem action='action_quit'/>"
	"		</menu>"
	"		<menu action='action_view'>"
	"			<menuitem action='action_change_view_mode'/>"
	"			<separator/>"
	"			<menuitem action='action_scheduled_recordings'/>"
	"			<menuitem action='action_auto_record'/>"
	"			<menuitem action='action_epg_event_search'/>"
	"			<menuitem action='action_channels'/>"
	"			<menuitem action='action_preferences'/>"
	"		</menu>"
	"		<menu action='action_video'>"
	"			<menuitem action='toggle_action_fullscreen'/>"
	"		</menu>"
	"		<menu action='action_audio'>"
	"			<menuitem action='toggle_action_mute'/>"
	"			<menuitem action='action_increase_volume'/>"
	"			<menuitem action='action_decrease_volume'/>"
	"		</menu>"
	"		<menu action='action_help'>"
	"			<menuitem action='action_about'/>"
	"		</menu>"
	"	</menubar>"
	"</ui>";

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Window(cobject), builder(builder)
{
	g_debug("MainWindow constructor");

	view_mode					= VIEW_MODE_CONTROLS;
	prefullscreen_view_mode		= VIEW_MODE_CONTROLS;
	timeout_source				= 0;
	channel_change_timeout		= 0;
	temp_channel_number			= 0;
	engine						= NULL;
	output_fd					= -1;
	mute_state					= false;
	
	is_cursor_visible = true;
	gchar     bits[] = {0};
	GdkColor  color = {0, 0, 0, 0};
	GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, bits, 1, 1);
	hidden_cursor = gdk_cursor_new_from_pixmap(pixmap, pixmap, &color, &color, 0, 0);

	g_debug("Loading UI files");
	
	add_accel_group(ui_manager->get_accel_group());
	ui_manager->add_ui_from_string(ui_info);

	builder->get_widget("label_time", label_time);
	label_time->hide();

	builder->get_widget("drawing_area_video", drawing_area_video);
	drawing_area_video->set_double_buffered(false);
	drawing_area_video->signal_expose_event().connect(sigc::mem_fun(*this, &MainWindow::on_drawing_area_expose_event));
	
	builder->get_widget("hbox_controls", hbox_controls);
	menu_bar = (Gtk::MenuBar*)ui_manager->get_widget("/menu_bar");
	
	Gtk::EventBox* event_box_video = NULL;
	builder->get_widget("event_box_video", event_box_video);
	event_box_video->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_button_pressed));

	event_box_video->modify_fg(		Gtk::STATE_NORMAL, Gdk::Color("black"));
	event_box_video->modify_bg(		Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_fg(	Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_bg(	Gtk::STATE_NORMAL, Gdk::Color("black"));

	Gtk::AboutDialog* dialog_about = NULL;
	builder->get_widget("dialog_about", dialog_about);
	dialog_about->set_version(VERSION);
	
	set_keep_above(configuration_manager.get_boolean_value("keep_above"));
	
	Gtk::VBox* vbox_main_window = NULL;
	builder->get_widget("vbox_main_window", vbox_main_window);

	vbox_main_window->pack_start(*menu_bar, Gtk::PACK_SHRINK);
	vbox_main_window->reorder_child(*menu_bar, 0);

	offset = 0;

	builder->get_widget("table_epg", table_epg);
	builder->get_widget("scrolled_window_epg", scrolled_window_epg);
	
	epg_span_hours = configuration_manager.get_int_value("epg_span_hours");
	Gtk::Button* button = NULL;
	
	builder->get_widget("button_epg_now", button);
	button->signal_clicked().connect(sigc::bind<gint>(sigc::mem_fun(*this, &MainWindow::set_offset), 0));

	builder->get_widget("button_epg_previous", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::previous));

	builder->get_widget("button_epg_next", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::next));

	builder->get_widget("button_epg_previous_day", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::previous_day));

	builder->get_widget("button_epg_next_day", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::next_day));

	builder->get_widget("label_epg_page", label_epg_page);
	builder->get_widget("spin_button_epg_page", spin_button_epg_page);
	spin_button_epg_page->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_spin_button_epg_page_changed));

	menu_bar->show_all();
	set_view_mode(view_mode);

	toggle_action_fullscreen->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_fullscreen));
	toggle_action_mute->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_mute));
	toggle_action_visibility->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::toggle_visibility));

	action_about->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about));
	action_auto_record->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_auto_record));
	action_channels->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::show_channels_dialog));
	action_change_view_mode->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_change_view_mode));
	action_decrease_volume->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_decrease_volume));
	action_epg_event_search->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_epg_event_search));
	action_increase_volume->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_increase_volume));
	action_preferences->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_preferences));
	action_present->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_present));
	action_restart_server->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_restart_server));
	action_quit->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::hide));
	action_scheduled_recordings->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_scheduled_recordings));

	signal_start_broadcasting.connect(sigc::mem_fun(*this, &MainWindow::on_start_broadcasting));
	signal_stop_broadcasting.connect(sigc::mem_fun(*this, &MainWindow::on_stop_broadcasting));
	signal_update.connect(sigc::mem_fun(*this, &MainWindow::on_update));
	signal_error.connect(sigc::mem_fun(*this, &MainWindow::on_error));

	volume_button = new Gtk::VolumeButton();
	volume_button->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_button_volume_value_changed));
	volume_button->set_value(1);
	hbox_controls->pack_start(*volume_button, false, false);
	hbox_controls->reorder_child(*volume_button, 1);
	volume_button->show();
		
	last_motion_time = time(NULL);
	timeout_source = gdk_threads_add_timeout(1000, &MainWindow::on_timeout, this);

	g_debug("MainWindow constructed");
}

MainWindow::~MainWindow()
{
	if (timeout_source != 0)
	{
		g_source_remove(timeout_source);
	}
	save_geometry();

	g_debug("MainWindow destroyed");
}

MainWindow* MainWindow::create(Glib::RefPtr<Gtk::Builder> builder)
{
	MainWindow* main_window = NULL;
	builder->get_widget_derived("window_main", main_window);
	return main_window;
}

void MainWindow::show_channels_dialog()
{
	ChannelsDialog& channels_dialog = ChannelsDialog::create(builder);	
	gint dialog_result = channels_dialog.run();
	channels_dialog.hide();

	if (dialog_result == Gtk::RESPONSE_OK)
	{
		channels_dialog.save();	
	}

	get_epg();
	signal_update();
}

void MainWindow::show_preferences_dialog()
{
	PreferencesDialog& preferences_dialog = PreferencesDialog::create(builder);
	preferences_dialog.run();
	preferences_dialog.hide();
}

void MainWindow::on_change_view_mode()
{
	set_view_mode(view_mode == VIEW_MODE_VIDEO ? VIEW_MODE_CONTROLS : VIEW_MODE_VIDEO);
}

bool MainWindow::on_event_box_video_button_pressed(GdkEventButton* event_button)
{
	if (event_button->button == 1)
	{
		if (event_button->type == GDK_2BUTTON_PRESS)
		{
			toggle_action_fullscreen->activate();
		}
	}
	else if (event_button->button == 3)
	{
		on_change_view_mode();
	}
	
	return false;
}

bool MainWindow::on_motion_notify_event(GdkEventMotion* event_motion)
{
	Gtk::Widget* widget = NULL;

	last_motion_time = time(NULL);
	if (!is_cursor_visible)
	{
		builder->get_widget("event_box_video", widget);
		Glib::RefPtr<Gdk::Window> event_box_video = widget->get_window();
		if (event_box_video)
		{
			event_box_video->set_cursor();
			is_cursor_visible = true;
		}
	}

	hbox_controls->show();
	menu_bar->show();

	return true;
}

void MainWindow::unfullscreen(gboolean restore_mode)
{
	Gtk::Window::unfullscreen();
	label_time->hide();
	
	if (restore_mode)
	{
		set_view_mode(prefullscreen_view_mode);
	}
}

void MainWindow::fullscreen(gboolean change_mode)
{
	prefullscreen_view_mode = view_mode;
	if (change_mode)
	{
		set_view_mode(VIEW_MODE_VIDEO);
	}
	label_time->show();
	
	Gtk::Window::fullscreen();
}

gboolean MainWindow::is_fullscreen()
{
	return get_window() && (get_window()->get_state() & Gdk::WINDOW_STATE_FULLSCREEN) != 0;
}

gboolean MainWindow::on_timeout(gpointer data)
{
	MainWindow* main_window = (MainWindow*)data;
	main_window->on_timeout();
	return true;
}

void MainWindow::on_timeout()
{
	static guint last_seconds = 60;

	try
	{
		guint now = time(NULL);

		label_time->set_text(get_local_time_text(now,"%H:%M"));
		
		if (channel_change_timeout > 1)
		{
			channel_change_timeout--;
		}
		else if (channel_change_timeout == 1)
		{
			// Deactivate the countdown
			channel_change_timeout = 0;

			guint channel_index = temp_channel_number - 1;

			// Reset the temporary channel number for the next run
			temp_channel_number = 0;
		}
	
		// Hide the mouse
		if (now - last_motion_time > 3 && is_cursor_visible)
		{
			Gtk::Widget* widget = NULL;
			builder->get_widget("event_box_video", widget);
			Glib::RefPtr<Gdk::Window> event_box_video = widget->get_window();
			if (event_box_video)
			{
				event_box_video->set_cursor(Gdk::Cursor(hidden_cursor));
				is_cursor_visible = false;
			}

			if (view_mode == VIEW_MODE_VIDEO)
			{
				hbox_controls->hide();
				menu_bar->hide();
			}
		}
		
		guint seconds = now % 60;
		if (last_seconds > seconds)
		{
			last_seconds = seconds;
			
			get_epg();
			signal_update();
		}
	}
	catch(...)
	{
		handle_error();
	}
}

void MainWindow::set_view_mode(ViewMode mode)
{
	Gtk::Widget* widget = NULL;

	widget = ui_manager->get_widget("/menu_bar");
	widget->property_visible() = (mode == VIEW_MODE_CONTROLS);
		
	scrolled_window_epg->property_visible() = (mode == VIEW_MODE_CONTROLS);

	hbox_controls->property_visible() = (mode == VIEW_MODE_CONTROLS);

	Gtk::HBox* hbox_epg_controls = NULL;
	builder->get_widget("hbox_epg_controls", hbox_epg_controls);
	hbox_epg_controls->property_visible() = (mode == VIEW_MODE_CONTROLS);

	view_mode = mode;
}

void MainWindow::show_scheduled_recordings_dialog()
{
	ScheduledRecordingsDialog& scheduled_recordings_dialog = ScheduledRecordingsDialog::create(builder);
	scheduled_recordings_dialog.run();
	scheduled_recordings_dialog.hide();
	get_epg();
	signal_update();
}

void MainWindow::show_epg_event_search_dialog()
{
	EpgEventSearchDialog& epg_event_search_dialog = EpgEventSearchDialog::get(builder);
	epg_event_search_dialog.run();
	epg_event_search_dialog.hide();
	get_epg();
	signal_update();
}

void MainWindow::on_show()
{	
	gint x = configuration_manager.get_int_value("x");
	gint y = configuration_manager.get_int_value("y");
	gint width = configuration_manager.get_int_value("width");
	gint height = configuration_manager.get_int_value("height");
		
	if (width > 0 && height > 0)
	{
		g_debug("Setting geometry (%d, %d, %d, %d)", x, y, width, height);
		move(x, y);
		set_default_size(width, height);
	}

	Gtk::Window::on_show();
	Gdk::Window::process_all_updates();

	if (configuration_manager.get_boolean_value("keep_above"))
	{
		set_keep_above();
	}
	
	Gtk::EventBox* event_box_video = NULL;
	builder->get_widget("event_box_video", event_box_video);
	event_box_video->resize_children();

	client.register_client();

	if (!client.is_registered())
	{
		throw Exception("Failed to communicate with Me TV server");
	}

	get_epg();
	select_channel_to_play();
}

void MainWindow::on_hide()
{
	stop_broadcasting();
	save_geometry();
	client.unregister_client();
	Gtk::Window::on_hide();
}

bool MainWindow::on_delete_event(GdkEventAny* event)
{
	hide();
	if (quit_on_close)
	{
		Gtk::Main::quit();
	}

	return true;
}

void MainWindow::save_geometry()
{
	if (property_visible())
	{
		gint x = -1;
		gint y = -1;
		gint width = -1;
		gint height = -1;
		
		get_position(x, y);
		get_size(width, height);
		
		configuration_manager.set_int_value("x", x);
		configuration_manager.set_int_value("y", y);
		configuration_manager.set_int_value("width", width);
		configuration_manager.set_int_value("height", height);
		
		g_debug("Saved geometry (%d, %d, %d, %d)", x, y, width, height);
	}
}

void MainWindow::toggle_visibility()
{
	property_visible() = !property_visible();
}

void MainWindow::add_channel_number(guint channel_number)
{
	g_debug("Key %d pressed", channel_number);

	temp_channel_number *= 10;
	temp_channel_number += channel_number;

	if (channel_change_timeout == 0)
	{
		channel_change_timeout = SECONDS_UNTIL_CHANNEL_CHANGE;
	}
}

bool MainWindow::on_key_press_event(GdkEventKey* event_key)
{
	switch(event_key->keyval)
	{		
		case GDK_KP_0: case GDK_0: add_channel_number(0); break;
		case GDK_KP_1: case GDK_1: add_channel_number(1); break;
		case GDK_KP_2: case GDK_2: add_channel_number(2); break;
		case GDK_KP_3: case GDK_3: add_channel_number(3); break;
		case GDK_KP_4: case GDK_4: add_channel_number(4); break;
		case GDK_KP_5: case GDK_5: add_channel_number(5); break;
		case GDK_KP_6: case GDK_6: add_channel_number(6); break;
		case GDK_KP_7: case GDK_7: add_channel_number(7); break;
		case GDK_KP_8: case GDK_8: add_channel_number(8); break;
		case GDK_KP_9: case GDK_9: add_channel_number(9); break;
			
 		default:
			break;
	}
	
	return Gtk::Window::on_key_press_event(event_key);
}

bool MainWindow::on_drawing_area_expose_event(GdkEventExpose* event_expose)
{
	if (drawing_area_video != NULL && drawing_area_video->is_realized())
	{
		Glib::RefPtr<Gtk::Style> style = drawing_area_video->get_style();

		if (!style)
		{
			return true;
		}
		
		drawing_area_video->get_window()->draw_rectangle(
			style->get_bg_gc(Gtk::STATE_NORMAL), true,
			event_expose->area.x, event_expose->area.y,
			event_expose->area.width, event_expose->area.height);

		if (engine != NULL)
		{
			engine->on_expose(event_expose);
		}
	}

	return false;
}

void MainWindow::create_engine()
{
	if (engine != NULL)
	{
		throw Exception(_("Failed to start engine: Engine has already been started"));
	}
	
	g_debug("Creating engine");
	if (engine_type == "vlc")
	{
		engine = new VlcEngine();
	}
	else if (engine_type == "xine")
	{
		engine = new XineEngine();
	}
	else if (engine_type == "gstreamer")
	{
		engine = new GStreamerEngine();
	}
	else
	{
		throw Exception(String::compose("Unknown engine type '%1'", engine_type));
	}

	engine->set_mute_state(mute_state);
	
	g_debug("Engine created");
}

void MainWindow::stop_broadcasting()
{
	signal_stop_broadcasting();
}

void MainWindow::play(const String& mrl)
{
	if (engine == NULL)
	{
		create_engine();
	}

	if (engine != NULL)
	{
		engine->set_window(GDK_WINDOW_XID(drawing_area_video->get_window()->gobj()));
		engine->set_mrl(mrl);
		engine->set_volume(volume_button->get_value() * 100);
		engine->play();
	}
}

void MainWindow::on_update()
{
	g_debug("Updating UI");

	String description = "Me TV - It's TV for me computer";
	String title = description;

	int broadcasting_channel = client.get_broadcasting_channel_id();
	if (broadcasting_channel != -1)
	{
		Client::Channel& channel = epg.get_by_id(broadcasting_channel);

		title = "Me TV - " + channel.name;
		description = channel.name;

		if (!channel.epg_events.empty())
		{
			String event_title = (*(channel.epg_events.begin())).title;

			title += " - " + event_title;
			description += " - " + event_title;
		}
	}

	set_status_text(description);
	set_title(title);

	if (property_visible().get_value())
	{
		update(epg);
	}
	g_debug("UI updated");
}

void MainWindow::set_status_text(const String& text)
{
	Gtk::Label* label = NULL;
	builder->get_widget("label_status_text", label);
	label->set_text(text);

	label->set_tooltip_text(text);
}

void MainWindow::on_channels()
{
	show_channels_dialog();
}

void MainWindow::on_scheduled_recordings()
{
	show_scheduled_recordings_dialog();
	get_epg();
	signal_update();
}

void MainWindow::on_epg_event_search()
{
	show_epg_event_search_dialog();
	get_epg();
	signal_update();
}

void MainWindow::on_preferences()
{
	show_preferences_dialog();
}

void MainWindow::on_present()
{
	present();
}

void MainWindow::on_restart_server()
{
	throw Exception("Not implemented");
}

void MainWindow::on_about()
{
	Gtk::Dialog* about_dialog = NULL;
	builder->get_widget("dialog_about", about_dialog);
	about_dialog->run();
	about_dialog->hide();
}

void MainWindow::on_auto_record()
{
	AutoRecordDialog& auto_record_dialog = AutoRecordDialog::create(builder);
	auto_record_dialog.run();
	auto_record_dialog.hide();
}

void MainWindow::on_mute()
{
	set_mute_state(toggle_action_mute->get_active());
}

void MainWindow::on_increase_volume()
{
	volume_button->set_value(volume_button->get_value() * 1.1);
}

void MainWindow::on_decrease_volume()
{
	volume_button->set_value(volume_button->get_value() * 0.9);
}

void MainWindow::on_button_volume_value_changed(double value)
{
	if (engine != NULL)
	{
		engine->set_volume(value * 100);
	}
}

void MainWindow::set_mute_state(gboolean state)
{
	mute_state = state;

	toggle_action_mute->set_icon_name(state ? "audio-volume-muted" : "audio-volume-high");

	if (engine != NULL)
	{
		engine->set_mute_state(mute_state);
	}
}

void MainWindow::on_fullscreen()
{
	if (toggle_action_fullscreen->get_active())
	{
		fullscreen();
	}
	else
	{
		unfullscreen();
	}
}

void MainWindow::select_channel_to_play()
{
	if (epg.empty())
	{
		throw Exception("No channels");
	}
	
	int channel_id = configuration_manager.get_int_value("last_channel");
	signal_start_rtsp(channel_id);
}

void MainWindow::on_start_rtsp(int channel_id)
{
	stop_broadcasting();

	Client::RtspStream stream = client.start_rtsp(channel_id);

	String mrl = String::compose("rtsp://%1:8554/%2", client.id);

	play(mrl);
	
	configuration_manager.set_int_value("last_channel", channel_id);

	get_epg();
	signal_update();
}

void MainWindow::on_stop_broadcasting()
{
	if (engine != NULL)
	{
		g_debug("Stopping engine");
		engine->stop();
		delete engine;
		engine = NULL;
		g_debug("Engine stopped");
	}
}

void MainWindow::on_error(const String& message)
{
	set_status_text(message);
}

void MainWindow::set_offset(gint value)
{
	offset = value;
	get_epg();
	update(epg);
}

void MainWindow::previous()
{
	set_offset(offset - (epg_span_hours*60*60));
}

void MainWindow::next()
{
	set_offset(offset + (epg_span_hours*60*60));
}

void MainWindow::previous_day()
{
	set_offset(offset - (24*60*60));
}

void MainWindow::next_day()
{
	set_offset(offset + (24*60*60));
}

void MainWindow::on_spin_button_epg_page_changed()
{	
	get_epg();
	update_table(epg);
}

void MainWindow::get_epg()
{
	if (client.is_registered())
	{
		int start_time = time(NULL) + offset;
		int epg_span_hours = configuration_manager.get_int_value("epg_span_hours");
		epg = client.get_epg(start_time, start_time + epg_span_hours * 60 * 60);
	}
}

void MainWindow::update(Client::ChannelList& channels)
{
	g_debug("Updating EPG");

	update_pages(channels);
	update_table(channels);
	
	g_debug("EPG update complete");
}

void MainWindow::clear()
{
	std::list<Gtk::Widget*> children = table_epg->get_children();
	std::list<Gtk::Widget*>::iterator iterator = children.begin();
	while (iterator != children.end())
	{
		Gtk::Widget* first = *iterator;
		table_epg->remove(*first);
		delete first;
		iterator++;
	}
}

void MainWindow::update_pages(Client::ChannelList& channels)
{
	guint epg_page_size = configuration_manager.get_int_value("epg_page_size");
	if (epg_page_size == 0)
	{
		return;
	}

	double min = 0, max = 0;
	spin_button_epg_page->get_range(min, max);
	
	guint epg_page_count = max;

	guint channel_count = channels.size();
	guint new_epg_page_count = channel_count == 0 ? 1 : ((channel_count-1) / epg_page_size) + 1;
	
	if (new_epg_page_count != epg_page_count)
	{		
		spin_button_epg_page->set_range(1, new_epg_page_count);
	}
	label_epg_page->property_visible() = new_epg_page_count > 1;
	spin_button_epg_page->property_visible() = new_epg_page_count > 1;
}

void MainWindow::update_table(Client::ChannelList& channels)
{
	Gtk::Adjustment* hadjustment = scrolled_window_epg->get_hadjustment();
	Gtk::Adjustment* vadjustment = scrolled_window_epg->get_vadjustment();
	
	gdouble hvalue = hadjustment->get_value();
	gdouble vvalue = vadjustment->get_value();

	if (get_window())
	{
		get_window()->freeze_updates();
		
		clear();
		
		epg_span_hours = configuration_manager.get_int_value("epg_span_hours");

		Gtk::Widget* widget = NULL;

		builder->get_widget("button_epg_now", widget);
		widget->set_sensitive(offset != 0);

		table_epg->resize(epg_span_hours * COLUMNS_PER_HOUR + 1, channels.size() + 1);

		guint start_time = time(NULL) + offset;
		start_time = (start_time / COLUMNS_PER_HOUR) * COLUMNS_PER_HOUR;
		
		guint row = 0;
		gboolean show_epg_header = configuration_manager.get_boolean_value("show_epg_header");
		if (show_epg_header)
		{
			for (guint hour = 0; hour < epg_span_hours; hour++)
			{
				guint hour_time = start_time + (hour * 60 * 60);
				String hour_time_text = get_local_time_text(hour_time, "%c");

				Gtk::Frame* frame = Gtk::manage(new Gtk::Frame());
				Gtk::Label* label = Gtk::manage(new Gtk::Label(hour_time_text));
				label->set_alignment(0,0.5);
				label->show();
				frame->add(*label);
				frame->set_shadow_type(Gtk::SHADOW_OUT);
				
				attach_widget(*frame,
				    hour * COLUMNS_PER_HOUR + 1,
				    (hour+1) * COLUMNS_PER_HOUR + 1,
				    0, 1, Gtk::FILL | Gtk::EXPAND);
			}
			row++;
		}

		gint epg_page = spin_button_epg_page->is_visible() ? spin_button_epg_page->get_value_as_int() : 1;
		guint epg_page_size = configuration_manager.get_int_value("epg_page_size");
		gboolean show_channel_number = configuration_manager.get_boolean_value("show_channel_number");
		gboolean show_epg_time = configuration_manager.get_boolean_value("show_epg_time");
		gboolean show_epg_tooltips = configuration_manager.get_boolean_value("show_epg_tooltips");
		guint channel_start = (epg_page-1) * epg_page_size;
		guint channel_end = channel_start + epg_page_size;

		if (channel_end >= channels.size())
		{
			channel_end = channels.size();
		}
		
		Gtk::RadioButtonGroup group;
		guint channel_index = 0;
		for (Client::ChannelList::iterator i = channels.begin(); i != channels.end(); i++)
		{
			if (channel_index >= channel_start && channel_index < channel_end)
			{
				create_channel_row(group, *i, row++, start_time, channel_index + 1,
					show_channel_number, show_epg_time, show_epg_tooltips);
			}

			channel_index++;
		}
		get_window()->thaw_updates();
	}
	
	hadjustment->set_value(hvalue);
	vadjustment->set_value(vvalue);
}


void MainWindow::create_channel_row(Gtk::RadioButtonGroup& group, Client::Channel& channel,
	guint table_row, time_t start_time, guint channel_number,
	gboolean show_channel_number, gboolean show_epg_time, gboolean show_epg_tooltips)
{
	String channel_text = String::compose("<b>%1</b>", encode_xml(channel.name));
	if (show_channel_number)
	{
		channel_text = String::compose("<i>%1.</i> ", channel_number) + channel_text;
	}
		
	Gtk::RadioButton& channel_button = attach_radio_button(group, channel_text, false, 0, 1, table_row, table_row + 1);

	gboolean selected = client.get_broadcasting_channel_id() == channel.id;
	if (selected)
	{
		Gtk::HBox* hbox = dynamic_cast<Gtk::HBox*>(channel_button.get_child());
		Gtk::Image* image = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON));
		image->set_alignment(1, 0.5);
		hbox->pack_start(*image, false, false);
		image->show();
	}
	
	channel_button.set_active(selected);
	channel_button.signal_toggled().connect(
		sigc::bind<Gtk::RadioButton*, int>
		(
			sigc::mem_fun(*this, &MainWindow::on_channel_button_toggled),
			&channel_button, channel.id
		),
	    false
	);
	
	guint total_number_columns = 0;
	time_t end_time = start_time + epg_span_hours*60*60;
	guint last_event_end_time = 0;
	guint number_columns = epg_span_hours * COLUMNS_PER_HOUR + 1;

	if (!disable_epg)
	{
		Client::EpgEventList events = channel.epg_events;
		gboolean first = true;
		for (Client::EpgEventList::iterator i = events.begin(); i != events.end(); i++)
		{
			Client::EpgEvent& epg_event = *i;

			guint event_end_time = epg_event.start_time + epg_event.duration;		
			guint start_column = 0;
			if (epg_event.start_time < start_time)
			{
				start_column = 0;
			}
			else
			{
				start_column = (guint)round((epg_event.start_time - start_time) / COLUMNS_PER_HOUR);
			}
		
			guint end_column = (guint)round((event_end_time - start_time) / COLUMNS_PER_HOUR);
			if (end_column > number_columns-1)
			{
				end_column = number_columns-1;
			}
		
			guint column_count = end_column - start_column;
			if (start_column >= total_number_columns && column_count > 0)
			{
				// If there's a gap, plug it
				if (start_column > total_number_columns)
				{
					guint empty_columns = start_column - total_number_columns;
					Gtk::Button& button = attach_button(
						empty_columns < 10 ? _("-") : _("Unknown program"), false, true,
						total_number_columns + 1, start_column + 1, table_row, table_row + 1);
					button.set_sensitive(false);
					total_number_columns += empty_columns;
				}
			
				if (column_count > 0)
				{
					guint converted_start_time = epg_event.start_time;

					String text;
					if (show_epg_time)
					{
						text = get_local_time_text(converted_start_time, "<b>%H:%M");
						text += get_local_time_text(converted_start_time + epg_event.duration, " - %H:%M</b>\n");
					}
					text += encode_xml(epg_event.title);

					gboolean record = epg_event.scheduled_recording_id >= 0;
					
					Gtk::Button& button = attach_button(text, record, !first, start_column + 1, end_column + 1, table_row, table_row + 1);
					first = false;
					button.signal_clicked().connect(
						sigc::bind<Client::EpgEvent>
						(
							sigc::mem_fun(*this, &MainWindow::on_program_button_clicked),
							epg_event
						)
					);
					button.signal_button_press_event().connect(
						sigc::bind<Client::EpgEvent>
						(
							sigc::mem_fun(*this, &MainWindow::on_program_button_press_event),
							epg_event
						),
					    false
					);

					if (show_epg_tooltips)
					{
						String tooltip_text = epg_event.title;
						tooltip_text += get_local_time_text(converted_start_time, "\n%A, %B %d (%H:%M");
						tooltip_text += get_local_time_text(converted_start_time + epg_event.duration, " - %H:%M)");
						String subtitle = trim_string(epg_event.subtitle);
						String description = trim_string(epg_event.description);
						if (!subtitle.empty())
						{
							tooltip_text += "\n" + subtitle;
							if (!description.empty())
							{
								tooltip_text += " - ";
							}
						}
						else if (!description.empty())
						{
							tooltip_text += "\n";
						}
						tooltip_text += description;
						button.set_tooltip_text(tooltip_text);
					}
				}

				total_number_columns += column_count;
			}
			last_event_end_time = event_end_time;
		}
	}
	
	if (total_number_columns < number_columns-1)
	{
		guint empty_columns = (number_columns-1) - total_number_columns;
		Gtk::Button& button = attach_button(
			empty_columns < 10 ? _("-") : _("Unknown program"), false, true,
			total_number_columns + 1, number_columns, table_row, table_row + 1);
		button.set_sensitive(false);
	}
}

Gtk::RadioButton& MainWindow::attach_radio_button(Gtk::RadioButtonGroup& group, const String& text, gboolean record, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::RadioButton* button = Gtk::manage(new Gtk::RadioButton(group));
	button->property_draw_indicator() = false;
	button->set_active(false);
	button->set_alignment(0, 0.5);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());

	Gtk::Label* label = Gtk::manage(new Gtk::Label(text));
	label->set_use_markup(true);
	label->set_alignment(0, 0.5);
	label->set_padding(3, 0);
	hbox->pack_start(*label, true, true);

	if (record == true)
	{
		Gtk::Image* image = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_RECORD, Gtk::ICON_SIZE_BUTTON));
		image->set_alignment(1, 0.5);
		hbox->pack_end(*image, false, false);
	}
	
	button->add(*hbox);
	button->show_all();

	attach_widget(*button, left_attach, right_attach, top_attach, bottom_attach, attach_options);

	return *button;
}

Gtk::Button& MainWindow::attach_button(const String& text, gboolean record, gboolean ellipsize, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::Button* button = new Gtk::Button();
	button->set_alignment(0, 0.5);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());

	Gtk::Label* label = Gtk::manage(new Gtk::Label(text));
	label->set_use_markup(true);
	label->set_alignment(0, 0.5);
	if (ellipsize)
	{
		label->set_ellipsize(Pango::ELLIPSIZE_END);
	}
	hbox->pack_start(*label, true, true);

	if (record == true)
	{
		Gtk::Image* image = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_RECORD, Gtk::ICON_SIZE_BUTTON));
		hbox->pack_end(*image, false, false);
	}
		
	button->add(*hbox);	
	button->show_all();

	attach_widget(*button, left_attach, right_attach, top_attach, bottom_attach, attach_options);
	
	return *button;
}

Gtk::Label& MainWindow::attach_label(const String& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	Gtk::Label* label = new Gtk::Label(text.c_str());
	label->set_justify(Gtk::JUSTIFY_LEFT);
	label->set_use_markup(true);
	attach_widget(*label, left_attach, right_attach, top_attach, bottom_attach, attach_options);
	return *label;
}

void MainWindow::attach_widget(Gtk::Widget& widget, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options)
{
	table_epg->attach(widget, left_attach, right_attach, top_attach, bottom_attach, attach_options, Gtk::FILL, 0, 0);
	widget.show();
}

void MainWindow::on_channel_button_toggled(Gtk::RadioButton* button, int channel_id)
{
	if (button->get_active())
	{
		signal_start_broadcasting(channel_id);
	}
}

bool MainWindow::on_program_button_press_event(GdkEventButton* event, Client::EpgEvent& epg_event)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{
		if (epg_event.scheduled_recording_id >= 0)
		{
			client.remove_scheduled_recording(epg_event.scheduled_recording_id);
		}
		else
		{
			client.add_scheduled_recording(epg_event.id);
		}

		get_epg();
		signal_update();
	}

	return false;
}

void MainWindow::on_program_button_clicked(Client::EpgEvent& epg_event)
{
	EpgEventDialog::create(builder).show_epg_event(epg_event);
}

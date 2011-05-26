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

#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include "me-tv-client.h"
#include "engine.h"
#include <dbus/dbus.h>
#include <gtkmm/volumebutton.h>

typedef enum
{
	VIEW_MODE_VIDEO,
	VIEW_MODE_CONTROLS
} ViewMode;

class MainWindow : public Gtk::Window
{
private:
	const Glib::RefPtr<Gtk::Builder>	builder;
	Gtk::DrawingArea*					drawing_area_video;
	guint								last_motion_time;
	GdkCursor*							hidden_cursor;
	gboolean							is_cursor_visible;
	Gtk::MenuBar*						menu_bar;
	Gtk::VolumeButton*					volume_button;
	Gtk::HBox*							hbox_controls;
	Gtk::Label*							label_time;
	ViewMode							view_mode;
	ViewMode							prefullscreen_view_mode;
	guint								timeout_source;
	Engine*								engine;
	gint								output_fd;
	Glib::StaticRecMutex				mutex;
	gboolean							mute_state;
	guint								channel_change_timeout;
	guint								temp_channel_number;
	gint								offset;
	gsize								span_hours;
	gsize								span_minutes;
	gsize								span_seconds;
	guint								epg_span_hours;
	Gtk::SpinButton*					spin_button_epg_page;
	Gtk::Label*							label_epg_page;
	Gtk::Table*							table_epg;
	Gtk::ScrolledWindow*				scrolled_window_epg;
	Client::ChannelList					epg;
	
	void stop();
	void set_view_mode(ViewMode display_mode);
	void load_devices();
	void set_state(const String& name, gboolean state);
	void add_channel_number(guint channel_number);
	void toggle_mute();
	void set_mute_state(gboolean state);
	void set_status_text(const String& text);
	void select_channel_to_play();
	void stop_rtsp();

	void play(const String& mrl);

	void toggle_visibility();
	void save_geometry();

	void fullscreen(gboolean change_mode = true);
	void unfullscreen(gboolean restore_mode = true);
	gboolean is_fullscreen();

	bool on_delete_event(GdkEventAny* event);
	bool on_motion_notify_event(GdkEventMotion* event);
	bool on_drawing_area_expose_event(GdkEventExpose* event);
	static gboolean on_timeout(gpointer data);
	void on_timeout();
	bool on_key_press_event(GdkEventKey* event);
	bool on_event_box_video_button_pressed(GdkEventButton* event);

	void on_start_rtsp(int channel_id);
	void on_stop_rtsp();
	void on_update();
	void on_error(const String& message);

	void on_show();
	void on_hide();
			
	void on_about();
	void on_auto_record();
	void on_button_volume_value_changed(double value);
	void on_change_view_mode();
	void on_channels();
	void on_decrease_volume();
	void on_devices();
	void on_epg_event_search();
	void on_fullscreen();
	void on_increase_volume();
	void on_meters();
	void on_mute();
	void on_next_channel();
	void on_preferences();
	void on_previous_channel();
	void on_present();
	void on_restart_server();
	void on_scheduled_recordings();

	void create_engine();
	
	void previous();
	void next();
	void previous_day();
	void next_day();
		
	void on_channel_button_toggled(Gtk::RadioButton* button, int channel_id);
	bool on_program_button_press_event(GdkEventButton* event, Client::EpgEvent& epg_event);
	void on_program_button_clicked(Client::EpgEvent& epg_event);
	void on_spin_button_epg_page_changed();
		
	void clear();
	void update_pages(Client::ChannelList& channels);
	void update_table(Client::ChannelList& channels);
	void get_epg();
	
	Gtk::RadioButton& attach_radio_button(Gtk::RadioButtonGroup& group, const String& text, gboolean record, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	Gtk::Button& attach_button(const String& text, gboolean record, gboolean ellipsize, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	Gtk::Label& attach_label(const String& text, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	void attach_widget(Gtk::Widget& widget, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, Gtk::AttachOptions attach_options = Gtk::FILL);
	void create_channel_row(Gtk::RadioButtonGroup& group, Client::Channel& channel,
		guint table_row, time_t start_time, guint channel_number,
		gboolean show_channel_number, gboolean show_epg_time, gboolean show_epg_tooltips);

	void update(Client::ChannelList& channels);
	void set_offset(gint value);
	void set_epg_page(gint value);
	void increment_offset(gint value);

public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
	virtual ~MainWindow();
		
	static MainWindow* create(Glib::RefPtr<Gtk::Builder> builder);
		
	void show_channels_dialog();
	void show_preferences_dialog();
	void show_scheduled_recordings_dialog();
	void show_epg_event_search_dialog();
};

#endif

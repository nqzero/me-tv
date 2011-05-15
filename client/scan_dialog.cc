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

#include "scan_dialog.h"
#include "me-tv-client.h"

void show_error(const String& message)
{
	GdkLock gdk_lock;
	Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	dialog.set_title(_("Me TV - Error"));
	dialog.run();
}

ScanDialog& ScanDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	ScanDialog* scan_dialog = NULL;
	builder->get_widget_derived("window_scan_wizard", scan_dialog);
	return *scan_dialog;
}

ScanDialog::ScanDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Window(cobject), builder(builder)
{
	builder->get_widget("notebook_scan_wizard", notebook_scan_wizard);

	Gtk::Button* button = NULL;

	builder->get_widget("button_scan_wizard_add", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_add_clicked));

	builder->get_widget("button_scan_wizard_next", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_next_clicked));

	builder->get_widget("button_scan_wizard_cancel", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_cancel_clicked));

	builder->get_widget("button_scan_stop", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScanDialog::on_button_scan_stop_clicked));

	notebook_scan_wizard->set_show_tabs(false);

	builder->get_widget("progress_bar_scan", progress_bar_scan);
	builder->get_widget("tree_view_scanned_channels", tree_view_scanned_channels);
	
	list_store = Gtk::ListStore::create(columns);
	tree_view_scanned_channels->set_model(list_store);
	tree_view_scanned_channels->append_column(_("Service Name"), columns.column_name);
	tree_view_scanned_channels->set_enable_search(true);
	tree_view_scanned_channels->set_search_column(columns.column_name);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scanned_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);

	Gtk::FileChooserButton* file_chooser_button_scan = NULL;
	builder->get_widget("file_chooser_button_scan", file_chooser_button_scan);
	
	file_chooser_button_scan->signal_file_set().connect(sigc::mem_fun(*this, &ScanDialog::on_file_chooser_button_scan_file_set));
	
	Gtk::FileChooserButton* file_chooser_button_import = NULL;
	builder->get_widget("file_chooser_button_import", file_chooser_button_import);
	file_chooser_button_import->signal_file_set().connect(sigc::mem_fun(*this, &ScanDialog::on_file_chooser_button_import_file_set));

	// This is the only thing that's working
	Gtk::RadioButton* radio_button_import = NULL;
	builder->get_widget("radio_button_import", radio_button_import);
	radio_button_import->set_active();
}

ScanDialog::~ScanDialog()
{
	stop_scan();
}

void ScanDialog::on_show()
{
	channel_count = 0;
	progress_bar_scan->set_fraction(0);
	
	Gtk::Button* button = NULL;

	builder->get_widget("button_scan_stop", button);
	button->hide();

	builder->get_widget("button_scan_wizard_cancel", button);
	button->show();

	builder->get_widget("button_scan_wizard_add", button);
	button->hide();

	builder->get_widget("button_scan_wizard_next", button);
	button->show();
	
	notebook_scan_wizard->set_current_page(0);

	Window::on_show();
}

void ScanDialog::on_hide()
{
	stop_scan();
	Window::on_hide();
}

void ScanDialog::stop_scan()
{
}

void ScanDialog::on_file_chooser_button_scan_file_set()
{
	Gtk::RadioButton* radio_button_scan = NULL;
	builder->get_widget("radio_button_scan", radio_button_scan);
	radio_button_scan->set_active();
}

void ScanDialog::on_file_chooser_button_import_file_set()
{
	Gtk::RadioButton* radio_button_import = NULL;
	builder->get_widget("radio_button_import", radio_button_import);
	radio_button_import->set_active();
}

void ScanDialog::on_button_scan_wizard_cancel_clicked()
{
	stop_scan();
	list_store->clear();
	hide();
}

void ScanDialog::on_button_scan_stop_clicked()
{	
	stop_scan();

	Gtk::Button* button = NULL;

	builder->get_widget("button_scan_stop", button);
	button->hide();

	builder->get_widget("button_scan_wizard_add", button);
	button->show();

	builder->get_widget("button_scan_wizard_cancel", button);
	button->show();
}

void ScanDialog::on_button_scan_wizard_next_clicked()
{
	stop_scan();

	list_store->clear();

	String initial_tuning_file;
	
	Gtk::RadioButton* radio_button_auto_scan = NULL;
	builder->get_widget("radio_button_auto_scan", radio_button_auto_scan);

	Gtk::RadioButton* radio_button_scan = NULL;
	builder->get_widget("radio_button_scan", radio_button_scan);

	Gtk::RadioButton* radio_button_import = NULL;
	builder->get_widget("radio_button_import", radio_button_import);
	
	if (radio_button_scan->get_active() || radio_button_auto_scan->get_active())
	{
		throw Exception("Not implemented");
	}
	else if (radio_button_import->get_active())
	{
		Gtk::FileChooserButton* file_chooser_button = NULL;
		builder->get_widget("file_chooser_button_import", file_chooser_button);
		String channels_conf_path = file_chooser_button->get_filename();
		import_channels_conf(channels_conf_path);
		hide();
	}
}

void ScanDialog::on_button_scan_wizard_add_clicked()
{
	hide();
}

void ScanDialog::on_combo_box_auto_scan_range_changed()
{
	Gtk::RadioButton* radio_button_auto_scan = NULL;
	builder->get_widget("radio_button_auto_scan", radio_button_auto_scan);
//	radio_button_auto_scan->set_active();
}

void ScanDialog::import_channels_conf(const String& channels_conf_path)
{
	Glib::RefPtr<Glib::IOChannel> file = Glib::IOChannel::create_from_file(channels_conf_path, "r");
	String line;
	guint line_count = 0;

	progress_bar_scan->set_fraction(0);
	progress_bar_scan->set_text(_("Importing channels"));

	Gtk::Button* button = NULL;
	builder->get_widget("button_scan_wizard_next", button);
	button->hide();
	notebook_scan_wizard->next_page();

	gboolean done = false;
	
	while (!done)
	{
		Glib::IOStatus io_status;

		try
		{
			io_status = file->read_line(line);
		}
		catch(...)
		{
			throw Exception(_("Failed to read data from file.  Please ensure that the channels file is UTF-8 encoded."));
		}
		
		if (io_status != Glib::IO_STATUS_NORMAL)
		{
			break;
		}
		
		line = trim_string(line);

		if (!line.empty())
		{
			client.add_channel(line);
		}
	}

	builder->get_widget("button_scan_wizard_add", button);
	button->show();

	g_debug("Finished importing channels");
}


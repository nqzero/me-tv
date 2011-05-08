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

#include "me-tv.h"
#include <gtkmm.h>
#include "scan_dialog.h"
#include "channels_dialog.h"
#include "../common/i18n.h"
#include "../common/exception.h"

ChannelsDialog& ChannelsDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	ChannelsDialog* channels_dialog = NULL;
	builder->get_widget_derived("dialog_channels", channels_dialog);
	return *channels_dialog;
}

ChannelsDialog::ChannelsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder)
{
	Gtk::Button* button = NULL;

	builder->get_widget("button_scan", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ChannelsDialog::on_button_scan_clicked));

	builder->get_widget("button_remove_selected_channels", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ChannelsDialog::on_button_remove_selected_channels_clicked));

	tree_view_displayed_channels = NULL;
	
	builder->get_widget("tree_view_displayed_channels", tree_view_displayed_channels);
	
	int column_count = 0;
	list_store = Gtk::ListStore::create(columns);
	tree_view_displayed_channels->set_model(list_store);

	column_count = tree_view_displayed_channels->append_column_editable(_("Channel Name"), columns.column_name);
	tree_view_displayed_channels->get_column(column_count - 1)->set_expand(true);

	column_count = tree_view_displayed_channels->append_column_numeric_editable(_("Record Before"), columns.column_record_extra_before, "%d");
	tree_view_displayed_channels->get_column(column_count - 1)->set_expand(false);
	
	column_count = tree_view_displayed_channels->append_column_numeric_editable(_("Record After"), columns.column_record_extra_after, "%d");
	tree_view_displayed_channels->get_column(column_count - 1)->set_expand(false);

	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_displayed_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);
}

void ChannelsDialog::show_scan_dialog()
{
	ScanDialog& scan_dialog = ScanDialog::create(builder);
	scan_dialog.show();
	Gtk::Main::run(scan_dialog);

	Client::ChannelList channels = client.get_channels();
	load();
}

void ChannelsDialog::on_button_scan_clicked()
{
	show_scan_dialog();
}

void ChannelsDialog::on_button_remove_selected_channels_clicked()
{
	get_window()->freeze_updates();
	Glib::RefPtr<Gtk::TreeSelection> tree_selection = tree_view_displayed_channels->get_selection();
	std::list<Gtk::TreeModel::Path> selected_channels = tree_selection->get_selected_rows();
	while (!selected_channels.empty())
	{
		const Gtk::TreeIter& iter = list_store->get_iter(*selected_channels.begin());
		client.remove_channel((*iter)[columns.column_id]);
		list_store->erase(iter);
		selected_channels = tree_selection->get_selected_rows();
	}
	get_window()->thaw_updates();
}

void ChannelsDialog::load()
{
	load(client.get_channels());
}

void ChannelsDialog::load(const Client::ChannelList& channels)
{
	list_store->clear();
	
	for (Client::ChannelList::const_iterator iterator = channels.begin(); iterator != channels.end(); iterator++)
	{
		Client::Channel channel = *iterator;

		Gtk::TreeModel::iterator row_iterator = list_store->append();
		Gtk::TreeModel::Row row					= *row_iterator;
		row[columns.column_id]					= channel.id;
		row[columns.column_name]				= channel.name;
		row[columns.column_record_extra_before]	= channel.record_extra_before;
		row[columns.column_record_extra_after]	= channel.record_extra_after;
	}
}

void ChannelsDialog::save()
{
	Glib::RefPtr<Gtk::TreeModel> model = tree_view_displayed_channels->get_model();
	Gtk::TreeModel::Children children = model->children();
	Gtk::TreeIter iterator = children.begin();

	int sort_order = 0;
	while (iterator != children.end())
	{
		Gtk::TreeModel::Row row(*iterator);
		guint id = row[columns.column_id];
		Glib::ustring name = row[columns.column_name];
		gint record_extra_before = row[columns.column_record_extra_before];
		gint record_extra_after = row[columns.column_record_extra_after];
		client.set_channel(id, name, ++sort_order, record_extra_before, record_extra_after);

		iterator++;
	}
}

void ChannelsDialog::on_show()
{
	Client::ChannelList channels = client.get_channels();
	load(channels);

	Gtk::Dialog::on_show();

	if (channels.empty())
	{
		show_scan_dialog();
	}
}


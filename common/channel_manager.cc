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

#include "channel_manager.h"
#include "global.h"
#include "i18n.h"
#include "exception.h"

void ChannelManager::load(Glib::RefPtr<DataModelIter>& iter, Channel& channel)
{
	channel.id											= Data::get_int(iter, "id");
	channel.name										= Data::get(iter, "name");
	channel.transponder.frontend_type					= (fe_type_t)Data::get_int(iter, "type");
	channel.sort_order									= Data::get_int(iter, "sort_order");
	channel.mrl											= Data::get(iter, "mrl");
	channel.service_id									= Data::get_int(iter, "service_id");
	channel.transponder.frontend_parameters.frequency	= Data::get_int(iter, "frequency");
	channel.transponder.frontend_parameters.inversion	= (fe_spectral_inversion)Data::get_int(iter, "inversion");
	channel.record_extra_before							= Data::get_int(iter, "record_extra_before");
	channel.record_extra_after							= Data::get_int(iter, "record_extra_after");
	
	switch(channel.transponder.frontend_type)
	{
	case FE_OFDM:
		channel.transponder.frontend_parameters.u.ofdm.bandwidth				= (fe_bandwidth)Data::get_int(iter, "bandwidth");
		channel.transponder.frontend_parameters.u.ofdm.code_rate_HP				= (fe_code_rate)Data::get_int(iter, "code_rate_hp");
		channel.transponder.frontend_parameters.u.ofdm.code_rate_LP				= (fe_code_rate)Data::get_int(iter, "code_rate_lp");
		channel.transponder.frontend_parameters.u.ofdm.constellation			= (fe_modulation)Data::get_int(iter, "constellation");
		channel.transponder.frontend_parameters.u.ofdm.transmission_mode		= (fe_transmit_mode)Data::get_int(iter, "transmission_mode");
		channel.transponder.frontend_parameters.u.ofdm.guard_interval			= (fe_guard_interval)Data::get_int(iter, "guard_interval");
		channel.transponder.frontend_parameters.u.ofdm.hierarchy_information	= (fe_hierarchy)Data::get_int(iter, "hierarchy_information");
		break;
			
	case FE_QAM:
		channel.transponder.frontend_parameters.u.qam.symbol_rate				= Data::get_int(iter, "symbol_rate");
		channel.transponder.frontend_parameters.u.qam.fec_inner					= (fe_code_rate)Data::get_int(iter, "fec_inner");
		channel.transponder.frontend_parameters.u.qam.modulation				= (fe_modulation)Data::get_int(iter, "modulation");
		break;
	
	case FE_QPSK:
		channel.transponder.frontend_parameters.u.qpsk.symbol_rate				= Data::get_int(iter, "symbol_rate");
		channel.transponder.frontend_parameters.u.qpsk.fec_inner				= (fe_code_rate)Data::get_int(iter, "fec_inner");
		channel.transponder.polarisation										= Data::get_int(iter, "polarisation");
		break;

	case FE_ATSC:
		channel.transponder.frontend_parameters.u.vsb.modulation				= (fe_modulation)Data::get_int(iter, "modulation");
		break;

	default:
		throw Exception(_("Unknown frontend type"));
	}
}

gboolean ChannelManager::find(Channel& channel, guint channel_id)
{
	gboolean result = false;

	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
		String::compose("select * from channel where id = %1", channel_id));
	Glib::RefPtr<DataModelIter> iter = model->create_iter();
	
	if (iter->move_next())
	{
		load(iter, channel);
		result = true;
	}
	
	return result;
}

gboolean ChannelManager::find(Channel& channel, guint frequency, guint service_id)
{
	gboolean result = false;

	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
		String::compose("select * from channel where frequency = %1 and service_id = %2", frequency, service_id));
	Glib::RefPtr<DataModelIter> iter = model->create_iter();
	
	if (iter->move_next())
	{
		load(iter, channel);
		result = true;
	}
	
	return result;
}

Channel ChannelManager::get(guint channel_id)
{
	Channel channel;

	if (!find(channel, channel_id))
	{
		throw Exception(String::compose(_("Channel %1 not found"), channel_id));
	}

	return channel;
}

void ChannelManager::remove_channel(guint channel_id)
{
	g_debug("Deleting channel '%d'", channel_id);
	data_connection->statement_execute_non_select(
		String::compose("delete from channel where id = %1", channel_id));
	g_debug("Channel '%d' deleted", channel_id);
}

void ChannelManager::add_channel(const Channel& channel)
{
	g_debug("Saving channel '%s'", channel.name.c_str());
	Glib::RefPtr<SqlBuilder> builder = SqlBuilder::create(SQL_STATEMENT_INSERT);
	builder->set_table("channel");

	builder->add_field_value("name", channel.name);
	builder->add_field_value("type", channel.transponder.frontend_type);
	builder->add_field_value("sort_order", channel.sort_order);
	builder->add_field_value("mrl", channel.mrl);
	builder->add_field_value("service_id", channel.service_id);
	builder->add_field_value("frequency", channel.transponder.frontend_parameters.frequency);
	builder->add_field_value("inversion", channel.transponder.frontend_parameters.inversion);
	builder->add_field_value("record_extra_before", channel.record_extra_before);
	builder->add_field_value("record_extra_after", channel.record_extra_after);

	switch(channel.transponder.frontend_type)
	{
	case FE_OFDM:
		builder->add_field_value("bandwidth", channel.transponder.frontend_parameters.u.ofdm.bandwidth);
		builder->add_field_value("code_rate_hp", channel.transponder.frontend_parameters.u.ofdm.code_rate_HP);
		builder->add_field_value("code_rate_lp", channel.transponder.frontend_parameters.u.ofdm.code_rate_LP);
		builder->add_field_value("constellation", channel.transponder.frontend_parameters.u.ofdm.constellation);
		builder->add_field_value("transmission_mode", channel.transponder.frontend_parameters.u.ofdm.transmission_mode);
		builder->add_field_value("guard_interval", channel.transponder.frontend_parameters.u.ofdm.guard_interval);
		builder->add_field_value("hierarchy_information", channel.transponder.frontend_parameters.u.ofdm.hierarchy_information);
		break;
				
	case FE_QAM:
		builder->add_field_value("symbol_rate", channel.transponder.frontend_parameters.u.qam.symbol_rate);
		builder->add_field_value("fec_inner", channel.transponder.frontend_parameters.u.qam.fec_inner);
		builder->add_field_value("modulation", channel.transponder.frontend_parameters.u.qam.modulation);
		break;
				
	case FE_QPSK:
		builder->add_field_value("symbol_rate", channel.transponder.frontend_parameters.u.qpsk.symbol_rate);
		builder->add_field_value("fec_inner", channel.transponder.frontend_parameters.u.qpsk.fec_inner);
		builder->add_field_value("polarisation", channel.transponder.polarisation);
		break;
				
	case FE_ATSC:
		builder->add_field_value("modulation", channel.transponder.frontend_parameters.u.vsb.modulation);
		break;

	default:
		throw Exception(_("Unknown frontend type"));
	}

	data_connection->statement_execute_non_select_builder(builder);
	
	g_debug("Channel '%s' saved", channel.name.c_str());
}

ChannelList ChannelManager::get_all()
{
	ChannelList channels;

	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
		"select * from channel order by sort_order;");
	Glib::RefPtr<DataModelIter> iter = model->create_iter();
	
	while (iter->move_next())
	{
		Channel channel;
		load(iter, channel);
		channels.push_back(channel);
	}
	
	return channels;
}

void ChannelManager::set_channel(int channel_id, const String& name,
	guint sort_order, guint record_extra_before, guint record_extra_after)
{
	data_connection->statement_execute_non_select(
		String::compose("update channel set name = '%2', sort_order = %3, record_extra_before = '%4', record_extra_after = '%5' "
		                "where id = %1;", channel_id, name, sort_order, record_extra_before, record_extra_after));	
	g_debug("Channel '%s' updated", name.c_str());
}

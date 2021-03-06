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

#include "epg_events.h"
#include "data.h"
#include "common.h"
#include "exception.h"

void EpgEvents::add_epg_event(Glib::RefPtr<Batch>& batch, const EpgEvent& epg_event)
{
	Glib::RefPtr<SqlBuilder> builder = SqlBuilder::create(SQL_STATEMENT_INSERT);

	builder->set_table("epg_event");

	builder->add_field_value("channel_id", epg_event.channel_id);
	builder->add_field_value("version_number", epg_event.version_number);
	builder->add_field_value("event_id", epg_event.event_id);
	builder->add_field_value("start_time", (guint)epg_event.start_time);
	builder->add_field_value("duration", epg_event.duration);

	batch->add_statement(builder->get_statement());

	Glib::RefPtr<SqlParser> parser = SqlParser::create();
	
	for (EpgEventTextList::const_iterator i = epg_event.texts.begin(); i != epg_event.texts.end(); i++)
	{
		const EpgEventText epg_event_text = *i;

		g_debug("Adding text (%d, %d,'%s')", epg_event.event_id, epg_event.channel_id, epg_event_text.title.c_str());

		String title = epg_event_text.title;
		String subtitle = epg_event_text.subtitle;
		String description = epg_event_text.description;

	    replace_text(title, "'", "''");
	    replace_text(subtitle, "'", "''");
		replace_text(description, "'", "''");
		
		Glib::RefPtr<Statement> statement = parser->parse_string(
			String::compose(
				"insert into epg_event_text ("
			    "epg_event_id, language, title, subtitle, description"
				") values ("
				"(select id from epg_event where channel_id = %1 and event_id = %2), '%3', '%4', '%5', '%6');",
			    epg_event.channel_id, epg_event.event_id,
			    epg_event_text.language, title, subtitle, description
			));
		batch->add_statement(statement);
	}			
}

void EpgEvents::update_epg_event(const EpgEvent& epg_event)
{
	g_debug("Updating %d", epg_event.event_id);
	Glib::RefPtr<SqlBuilder> builder = SqlBuilder::create(SQL_STATEMENT_UPDATE);
	builder->set_where(
		builder->add_cond(SQL_OPERATOR_TYPE_EQ,
			builder->add_id("id"),
			builder->add_expr(epg_event.id)));
		
	builder->set_table("epg_event");

	builder->add_field_value("channel_id", epg_event.channel_id);
	builder->add_field_value("version_number", epg_event.version_number);
	builder->add_field_value("event_id", epg_event.event_id);
	builder->add_field_value("start_time", (guint)epg_event.start_time);
	builder->add_field_value("duration", epg_event.duration);

	data_connection->statement_execute_non_select_builder(builder);
	
	for (EpgEventTextList::const_iterator k = epg_event.texts.begin(); k != epg_event.texts.end(); k++)
	{
		const EpgEventText epg_event_text = *k;
		Glib::RefPtr<SqlBuilder> builder_text = SqlBuilder::create(SQL_STATEMENT_UPDATE);

		builder->set_where(
			builder->add_cond(SQL_OPERATOR_TYPE_EQ,
				builder->add_id("id"),
				builder->add_expr(epg_event_text.id)));

		builder_text->set_table("epg_event_text");
		builder_text->add_field_value("epg_event_id", epg_event.id);
		builder_text->add_field_value("language", epg_event_text.language);
		builder_text->add_field_value("title", epg_event_text.title);
		builder_text->add_field_value("subtitle", epg_event_text.subtitle);
		builder_text->add_field_value("description", epg_event_text.description);

		data_connection->statement_execute_non_select_builder(builder_text);
	}			
}

gboolean EpgEvents::get_current(guint channel_id, EpgEvent& epg_event)
{	
	return false;
}

EpgEventList EpgEvents::get_all(time_t start_time, time_t end_time)
{
	EpgEventList result;

	String statement = "select * from epg_event ee, epg_event_text eet where ee.id = eet.epg_event_id";
	if (start_time != 0 || end_time != -1)
	{
		statement += String::compose(
			" and ((start_time <= %1 and (start_time+duration) >= %2) or "
			"(start_time >= %1 and start_time <= %2) or "
			"((start_time+duration) >= %1 and (start_time+duration) <= %2))",
			start_time, end_time);
	}
	statement += " order by start_time";
	
	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(statement);
	Glib::RefPtr<DataModelIter> iter = model->create_iter();
	
	while (iter->move_next())
	{
		EpgEvent epg_event;
		load(iter, epg_event);
		result.push_back(epg_event);
	}
	                                                                          
	return result;
}

void EpgEvents::load(Glib::RefPtr<DataModelIter> iter, EpgEvent& epg_event)
{
	epg_event.id				= Data::get_int(iter, "id");
	epg_event.channel_id		= Data::get_int(iter, "channel_id");
	epg_event.version_number	= Data::get_int(iter, "version_number");
	epg_event.event_id			= Data::get_int(iter, "event_id");
	epg_event.start_time		= Data::get_int(iter, "start_time");
	epg_event.duration			= Data::get_int(iter, "duration");

	EpgEventText epg_event_text;
		
	epg_event_text.epg_event_id	= epg_event.id;
	epg_event_text.language		= Data::get(iter, "language");
	epg_event_text.title		= Data::get(iter, "title");
	epg_event_text.subtitle		= Data::get(iter, "subtitle");
	epg_event_text.description	= Data::get(iter, "description");
	
	if (epg_event_text.subtitle == "-")
	{
		epg_event_text.subtitle.clear();
		epg_event.save = true;
	}

	epg_event.texts.push_back(epg_event_text);
}

EpgEvent EpgEvents::get(int epg_event_id)
{
	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(
		String::compose("select * from epg_event ee, epg_event_text eet "
		                "where ee.id = eet.epg_event_id and ee.id = %1",
		                epg_event_id));
	Glib::RefPtr<DataModelIter> iter = model->create_iter();
	
	if (!iter->move_next())
	{
		throw Exception("EPG event not found");
	}

	EpgEvent epg_event;
	load(iter, epg_event);

	return epg_event;
}

EpgEventList EpgEvents::search(const String& text, gboolean search_description)
{
	EpgEventList result;

	time_t now = time(NULL);
	String upper_text = text.uppercase();

	String statement = "select * from epg_event ee, epg_event_text eet where ee.id = eet.epg_event_id and ";
	statement += String::compose("start_time > %1 and (upper(eet.title) like '%%%2%%'", now, text);
	if (search_description)
	{
		statement += String::compose(" or upper(eet.description) like '%%%1%%'", text);
	}
	statement += ")";

	Glib::RefPtr<DataModel> model = data_connection->statement_execute_select(statement);
	Glib::RefPtr<DataModelIter> iter = model->create_iter();
	while (iter->move_next())
	{
		EpgEvent epg_event;
		load(iter, epg_event);
		result.push_back(epg_event);
	}

	return result;
}

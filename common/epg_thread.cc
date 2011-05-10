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

#include "epg_thread.h"
#include "epg_events.h"
#include "dvb_si.h"
#include "../common/exception.h"

class EITDemuxers
{
private:
	GSList* eit_demuxers;
	guint demuxer_count;
	Glib::ustring demuxer_path;

public:
	EITDemuxers(const Glib::ustring& path)
	{
		demuxer_path = path;
		demuxer_count = 0;
		eit_demuxers = NULL;
	}
	
	~EITDemuxers()
	{
		delete_all();
	}
		
	gboolean get_next_eit(Dvb::SI::SectionParser& parser, Dvb::SI::EventInformationSection& section, gboolean is_atsc, const Glib::ustring& text_encoding);
	
	Dvb::Demuxer* add()
	{
		Dvb::Demuxer* demuxer = new Dvb::Demuxer(demuxer_path);
		eit_demuxers = g_slist_append(eit_demuxers, demuxer);
		demuxer_count++;
		return demuxer;
	}

	void delete_all()
	{
		while (eit_demuxers != NULL)
		{
			delete (Dvb::Demuxer*)eit_demuxers->data;
			eit_demuxers = g_slist_delete_link(eit_demuxers, eit_demuxers);
		}
		demuxer_count = 0;
	}
};

gboolean EITDemuxers::get_next_eit(Dvb::SI::SectionParser& parser, Dvb::SI::EventInformationSection& section, gboolean is_atsc, const Glib::ustring& text_encoding)
{
	if (eit_demuxers == NULL)
	{
		throw Exception(_("No demuxers"));
	}
	
	Dvb::Demuxer* selected_eit_demuxer = NULL;
	
	struct pollfd fds[demuxer_count];
	guint count = 0;
	
	GSList* eit_demuxer = eit_demuxers;
	while (eit_demuxer != NULL)
	{				
		fds[count].fd = ((Dvb::Demuxer*)eit_demuxer->data)->get_fd();
		fds[count].events = POLLIN;
		count++;
		eit_demuxer = g_slist_next(eit_demuxer);
	}

	gint result = ::poll(fds, demuxer_count, 2000);
	if (result >= 0)
	{
		count = 0;
		eit_demuxer = eit_demuxers;
		while (eit_demuxer != NULL && selected_eit_demuxer == NULL)
		{
			Dvb::Demuxer* current = (Dvb::Demuxer*)eit_demuxer->data;
			if ((fds[count++].revents&POLLIN) != 0)
			{
				selected_eit_demuxer = current;
			}
			eit_demuxer = g_slist_next(eit_demuxer);				
		}

		if (selected_eit_demuxer == NULL)
		{
			g_debug("Failed to get an EIT demuxer with events");
			return false;
		}

		if (is_atsc)
		{
			parser.parse_psip_eis(*selected_eit_demuxer, section);
		}
		else
		{
			parser.parse_eis(*selected_eit_demuxer, section);
		}
	}

	return result >= 0;
}

EpgThread::EpgThread(Dvb::Frontend& f, const Glib::ustring& encoding, guint t)
	: Thread("EPG Thread"), frontend(f), text_encoding(encoding), timeout(t)
{
}

class ChannelCache
{
private:
	class ChannelEntry
	{
	public:
		guint channel_id;
		guint frequency;
		guint service_id;
	};

	std::list<ChannelEntry> channels;
public:
	void add(guint channel_id, guint frequency, guint service_id)
	{
		ChannelEntry channel_entry;
		channel_entry.channel_id = channel_id;
		channel_entry.frequency = frequency;
		channel_entry.service_id = service_id;
		channels.push_back(channel_entry);
	}
	
	guint get(guint frequency, guint service_id)
	{
		for (std::list<ChannelEntry>::iterator i = channels.begin(); i != channels.end(); i++)
		{
			ChannelEntry channel_entry = *i;
			if (channel_entry.frequency == frequency && channel_entry.service_id == service_id)
			{
				return channel_entry.channel_id;
			}
		}
		return 0;
	}
};

class EpgCache
{
private:
	class EpgEntry
	{
	public:
		EpgEvent epg_event;
		gboolean saved;
	};

	gboolean is_dirty;

	std::list<EpgEntry> events;
public:
	EpgCache() : is_dirty(false) {}

	void add(EpgEvent& epg_event, gboolean saved)
	{
		EpgEntry epg_entry;
		epg_entry.epg_event = epg_event;
		epg_entry.saved = saved;
		events.push_back(epg_entry);

		if (!saved)
		{
			is_dirty = true;
		}
	}
	
	guint get(guint event_id, guint channel_id)
	{
		for (std::list<EpgEntry>::iterator i = events.begin(); i != events.end(); i++)
		{
			EpgEntry epg_entry = *i;
			if (epg_entry.epg_event.event_id == event_id && epg_entry.epg_event.channel_id == channel_id)
			{
				return epg_entry.epg_event.version_number;
			}
		}
		
		return 0;
	}

	void save()
	{
		if (is_dirty)
		{
			g_debug("Saving EPG events");
			for (std::list<EpgEntry>::iterator i = events.begin(); i != events.end(); i++)
			{
				EpgEntry& epg_entry = *i;
				if (!epg_entry.saved)
				{
					EpgEvents::add_epg_event(epg_entry.epg_event);
					epg_entry.saved = true;
				}
			}
			data_connection->commit_transaction(0);
			g_debug("EPG events saved");

			is_dirty = false;
		}
	}
};

void EpgThread::run()
{
	try
	{
		Glib::ustring					demux_path				= frontend.get_adapter().get_demux_path();
		EITDemuxers						demuxers(demux_path);
		Dvb::SI::SectionParser			parser(text_encoding, timeout);
		Dvb::SI::MasterGuideTableArray	master_guide_tables;
		Dvb::SI::VirtualChannelTable	virtual_channel_table;
		Dvb::SI::SystemTimeTable		system_time_table;
		ChannelCache					channel_cache;
		EpgCache						epg_cache;
	
		gboolean is_atsc = frontend.get_frontend_type() == FE_ATSC;
		if (is_atsc)
		{
			system_time_table.GPS_UTC_offset = 15;
			{
				Dvb::Demuxer demuxer_stt(demux_path);
				demuxer_stt.set_filter(PSIP_PID, STT_ID);
				parser.parse_psip_stt(demuxer_stt, system_time_table);
			}

			{
				Dvb::Demuxer demuxer_vct(demux_path);
				demuxer_vct.set_filter(PSIP_PID, TVCT_ID, 0xFE);
				parser.parse_psip_vct(demuxer_vct, virtual_channel_table);
			}

			{
				Dvb::Demuxer demuxer_mgt(demux_path);
				demuxer_mgt.set_filter(PSIP_PID, MGT_ID);
				parser.parse_psip_mgt(demuxer_mgt, master_guide_tables);
			}

			guint i = master_guide_tables.size();
			if (i > 0) do
			{
				--i;
				Dvb::SI::MasterGuideTable mgt = master_guide_tables[i];
				if (mgt.type >= 0x0100 && mgt.type <= 0x017F)
				{
					demuxers.add()->set_filter(mgt.pid, PSIP_EIT_ID, 0);
					g_debug("Set up PID 0x%02X for events", mgt.pid);
				}
			} while (i > 0);
		}
		else
		{
			demuxers.add()->set_filter(EIT_PID, EIT_ID, 0);
		}

		EpgEventList epg_events = EpgEvents::get_all();
		for (EpgEventList::iterator i = epg_events.begin(); i != epg_events.end(); i++)
		{
			EpgEvent& epg_event = *i;
			epg_cache.add(epg_event, true);
		}

		time_t last_save = time(NULL);
		
		guint frequency = frontend.get_frontend_parameters().frequency;
		while (!is_terminated())
		{
			try
			{
				Dvb::SI::EventInformationSection section;
			
				if (!demuxers.get_next_eit(parser, section, is_atsc, text_encoding))
				{
					terminate();
				}
				else
				{
					guint service_id = section.service_id;
				
					if (is_atsc)
					{
						bool found = false;
						gsize size = virtual_channel_table.channels.size();
	
						for (guint i = 0; i < size && !found; i++)
						{
							Dvb::SI::VirtualChannel& vc = virtual_channel_table.channels[i];
							if (vc.source_id == service_id)
							{
								service_id = vc.program_number;
								found = true;
							}
						}

						if (!found)
						{
							g_message(_("Unknown source_id %u"), service_id);
							service_id = 0;
						}
					}

					guint channel_id = channel_cache.get(frequency, service_id);
					if (channel_id == 0)
					{
						Channel channel;
						if (channel_manager.find(channel, frequency, service_id))
						{
							channel_id = channel.id;
							channel_cache.add(channel.id, frequency, service_id);
						}
					}
					
					if (channel_id != 0)
					{
						for (unsigned int k = 0; section.events.size() > k; k++)
						{
							Dvb::SI::Event& event	= section.events[k];

							guint version_number = epg_cache.get(event.event_id, channel_id);
							if (version_number == 0)
							{
								EpgEvent epg_event;

								epg_event.id				= 0;
								epg_event.channel_id		= channel_id;
								epg_event.version_number	= event.version_number;
								epg_event.event_id			= event.event_id;
								epg_event.start_time		= event.start_time;
								epg_event.duration			= event.duration;
						
								if (is_atsc)
								{
									epg_event.start_time -= system_time_table.GPS_UTC_offset;
								}
						
								if (epg_event.get_end_time() >= (time(NULL) - 20*60*60))
								{
									for (Dvb::SI::EventTextMap::iterator i = event.texts.begin(); i != event.texts.end(); i++)
									{
										EpgEventText epg_event_text;
										const Dvb::SI::EventText& event_text = i->second;
						
										epg_event_text.id					= 0;
										epg_event_text.epg_event_id			= 0;
										epg_event_text.language				= event_text.language;
										epg_event_text.title				= event_text.title;
										epg_event_text.subtitle				= event_text.subtitle;
										epg_event_text.description			= event_text.description;

										if (epg_event_text.subtitle == "-")
										{
											epg_event_text.subtitle.clear();
										}
						
										epg_event.texts.push_back(epg_event_text);
									}
								}

								g_debug("Adding event %d to EPG cache", epg_event.event_id);
								epg_cache.add(epg_event, false);
							}
						}
					}

					time_t now = time(NULL);
					if (now - last_save > 10)
					{
						epg_cache.save();
						last_save = now;
					}
				}
			}
			catch(const Glib::Exception& ex)
			{
				g_message("Exception in EPG thread loop: %s", ex.what().c_str());
			}
		}
	}
	catch(const Glib::Exception& ex)
	{
		g_message("Unrecoverable exception in EPG thread loop: %s", ex.what().c_str());
	}
	catch(...)
	{
		g_message("Unrecoverable exception in EPG thread loop");
	}
	
	g_debug("Exiting EPG thread");
}

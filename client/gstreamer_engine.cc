/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Michael Lamothe 2010 <michael.lamothe@gmail.com>
 * 
 * Me TV is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Me TV is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gstreamer_engine.h"
#include <gst/gst.h>
#include <gst/gstbus.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>

static gboolean on_bus_message (GstBus* bus, GstMessage *message, gpointer data)
{
	switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ERROR: {
			GError*	err = NULL;
			gchar* debug = NULL;

			gst_message_parse_error(message, &err, &debug);
			String message = err->message;
			g_error_free(err);
			g_free(debug);

			throw Exception(message);
			
			break;
		}

		case GST_MESSAGE_EOS:
			next = true;
			break;

		default:
			break;
	}

	return TRUE;
}

GStreamerEngine::GStreamerEngine()
{
	playbin = gst_element_factory_make("playbin2", "playbin");

	if (!GST_IS_ELEMENT(playbin))
	{
		throw Exception("Failed to create playbin");
	}

	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(playbin));
	gst_bus_add_watch(bus, on_bus_message, NULL);
	gst_object_unref(bus);
}

GStreamerEngine::~GStreamerEngine()
{
	g_debug("Destroying GStreamer engine");
	stop();
	if (GST_IS_ELEMENT(playbin))
	{
		gst_object_unref(GST_OBJECT (playbin));
	}
	g_debug("GStreamer engine destroyed");
}

void GStreamerEngine::set_window(int w)
{
	videosink = gst_element_factory_make("xvimagesink", "videosink");
	if (videosink == NULL || !GST_IS_ELEMENT(videosink))
	{
		throw Exception("Failed to create video sink");
	}
	g_object_set(G_OBJECT(videosink), "force-aspect-ratio", true, NULL);

	gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(videosink), w);

	g_debug("Deinterlacing is not available with the GStreamer engine");
	g_object_set(G_OBJECT(playbin),"video-sink", videosink, NULL);
}

void GStreamerEngine::pause(gboolean state)
{
	if ((bool)state)
		gst_element_set_state(GST_ELEMENT(playbin), GST_STATE_PAUSED);
	else
		gst_element_set_state(GST_ELEMENT(playbin), GST_STATE_PLAYING);
}

void GStreamerEngine::play()
{
	gst_element_set_state(GST_ELEMENT(playbin), GST_STATE_PLAYING);
}

void GStreamerEngine::stop()
{
	gst_element_set_state(GST_ELEMENT(playbin), GST_STATE_NULL);
}

void GStreamerEngine::set_mrl(const String& mrl)
{
	g_object_set(G_OBJECT(playbin), "uri", mrl.c_str(), NULL);
}

int GStreamerEngine::get_time()
{
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 position = 0;
	gst_element_query_position (playbin, &fmt, &position);
	return position / 1000000;
}

void GStreamerEngine::set_time(int time)
{
	gint64 t = (gint64)time;
	if (t < 0) t = 0;
	gst_element_seek_simple(playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, t * 1000000);
}

int GStreamerEngine::get_length()
{
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 length = 0;
	gst_element_query_duration(playbin, &fmt, &length);
	return length / 1000000;
}

float GStreamerEngine::get_percentage()
{
	int length = get_length();
	return length == 0 ? 0 : get_time()/(float)length;
}

void GStreamerEngine::set_percentage(float percentage)
{
	g_debug("Length: %d", get_length());
	set_time(percentage * get_length());
}

void GStreamerEngine::set_volume(double value)
{
	g_object_set(G_OBJECT(playbin), "volume", (gdouble)value/100, NULL);
}

double GStreamerEngine::get_volume()
{
	gdouble valvolume;
	g_object_get(G_OBJECT(playbin), "volume", &valvolume, NULL);
	return (double)valvolume * 100;
}

void GStreamerEngine::set_mute_state(gboolean mute)
{
	g_object_set(G_OBJECT(playbin), "volume", mute, NULL);
}

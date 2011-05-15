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

#include "xine_engine.h"
#include <xine/xineutils.h>
#include <X11/Xutil.h>
#include <gdk/gdkx.h>

#define INPUT_MOTION (ExposureMask | StructureNotifyMask | PropertyChangeMask)

static void dest_size_cb(void *data,
    int video_width, int video_height, double video_pixel_aspect,
	int *dest_width, int *dest_height, double *dest_pixel_aspect)
{
	XineEngine* engine = (XineEngine*)data;
	XWindowAttributes attributes;

	XLockDisplay(engine->get_display());
	XGetWindowAttributes(engine->get_display(), engine->get_window(), &attributes);
	XUnlockDisplay(engine->get_display());

	*dest_width        = attributes.width;
	*dest_height       = attributes.height;
	*dest_pixel_aspect = engine->get_pixel_aspect();
}

static void frame_output_cb(void *data,
    int video_width, int video_height, double video_pixel_aspect,
    int *dest_x, int *dest_y,
	int *dest_width, int *dest_height, double *dest_pixel_aspect,
    int *win_x, int *win_y)
{
	XineEngine* engine = (XineEngine*)data;
	XWindowAttributes attributes;

	XLockDisplay(engine->get_display());
	XGetWindowAttributes(engine->get_display(), engine->get_window(), &attributes);
	XUnlockDisplay(engine->get_display());

	*dest_x            = 0;
	*dest_y            = 0;
	*win_x             = 0;
	*win_y             = 0;
	*dest_width        = attributes.width;
	*dest_height       = attributes.height;
	*dest_pixel_aspect = engine->get_pixel_aspect();
}

static void event_listener(void *user_data, const xine_event_t *event)
{
	switch(event->type)
	{
		case XINE_EVENT_UI_PLAYBACK_FINISHED:
			next = true;
			break;
		default:
			break;
	}
}

XineEngine::XineEngine()
{
	if ((display = XOpenDisplay(getenv("DISPLAY"))) == NULL)
	{
		throw Exception("XOpenDisplay() failed");
	}
	
	window = 0;
	
	xine = xine_new();
	xine_init(xine);
}

XineEngine::~XineEngine()
{
	xine_close(stream);
	xine_event_dispose_queue(event_queue);
	xine_dispose(stream);
	if (audio_port)
	{
		xine_close_audio_driver(xine, audio_port);
	}
	if (video_port != NULL)
	{
		xine_close_video_driver(xine, video_port);
	}
	xine_exit(xine);
}

void XineEngine::set_window(int w)
{
	x11_visual_t	vis;
	double			res_h, res_v;
	char			*mrl = NULL;
	const char*		video_driver = "auto";
	const char*		audio_driver = "auto";

	window = w;
	screen = XDefaultScreen(display);

	XLockDisplay(display);
	XSelectInput (display, window, INPUT_MOTION);
	res_h = (DisplayWidth(display, screen) * 1000 / DisplayWidthMM(display, screen));
	res_v = (DisplayHeight(display, screen) * 1000 / DisplayHeightMM(display, screen));

	XSync(display, False);
	XUnlockDisplay(display);

	vis.display           = display;
	vis.screen            = screen;
	vis.d                 = window;
	vis.dest_size_cb      = dest_size_cb;
	vis.frame_output_cb   = frame_output_cb;
	vis.user_data         = this;
	pixel_aspect          = res_v / res_h;

	if (fabs(pixel_aspect - 1.0) < 0.01)
	{
		pixel_aspect = 1.0;
	}

	if ((video_port = xine_open_video_driver(xine, video_driver, XINE_VISUAL_TYPE_X11, (void *) &vis)) == NULL)
	{
		throw Exception("Failed to initialise video driver");
	}

	audio_port	= xine_open_audio_driver(xine, audio_driver, NULL);
	stream		= xine_stream_new(xine, audio_port, video_port);
	event_queue	= xine_event_new_queue(stream);
	xine_event_create_listener_thread(event_queue, event_listener, NULL);

	xine_port_send_gui_data(video_port, XINE_GUI_SEND_DRAWABLE_CHANGED, (void *) window);
	xine_port_send_gui_data(video_port, XINE_GUI_SEND_VIDEOWIN_VISIBLE, (void *) 1);

	if (video_port != NULL && false) // && action_deinterlace->get_active())
	{
		g_debug("tvtime deinterlacer");

		xine_post_wire_video_port(xine_get_video_source(stream), video_port);

		xine_post_t* plugin = xine_post_init(xine, "tvtime", 0, &audio_port, &video_port);
		if (plugin == NULL)
		{
			throw Exception("Failed to create tvtime plugin");
		}

		xine_post_out_t* plugin_output = xine_post_output (plugin, "video out")
				? : xine_post_output (plugin, "video")
				? : xine_post_output (plugin, xine_post_list_outputs (plugin)[0]);
		if (plugin_output == NULL)
		{
			throw Exception("Failed to get xine plugin output for deinterlacing");
		}

		xine_post_in_t* plugin_input = xine_post_input (plugin, "video")
				? : xine_post_input (plugin, "video in")
				? : xine_post_input (plugin, xine_post_list_inputs (plugin)[0]);

		if (plugin_input == NULL)
		{
			throw Exception("Failed to get xine plugin input for deinterlacing");
		}

		xine_post_wire(xine_get_video_source (stream), plugin_input);
		xine_post_wire_video_port(plugin_output, video_port);

		xine_set_param(stream, XINE_PARAM_VO_DEINTERLACE, true);
	}
	else
	{
		g_debug("Deinterlacer disabled");
		xine_set_param(stream, XINE_PARAM_VO_DEINTERLACE, false);
	}
}

void XineEngine::pause(gboolean state)
{
	xine_set_param(stream, XINE_PARAM_SPEED, state ? XINE_SPEED_PAUSE : XINE_SPEED_NORMAL);
}

void XineEngine::play()
{
	if (!xine_play(stream, 0, 0))
	{
		throw Exception("Failed to play mrl");
	}
}

void XineEngine::stop()
{
	xine_stop(stream);
}

void XineEngine::set_mrl(const String& mrl)
{	
	if (!xine_open(stream, mrl.c_str()))
	{
		throw Exception("Failed to open mrl");
	}
}

void XineEngine::set_time(int time)
{
	if (time < 0) time = 0;
	if (time > get_length())
	{
		next = true;
	}
	else
	{
		if (!xine_play(stream, 0, time))
		{
			throw Exception("Failed to play mrl");
		}
	}
}

bool XineEngine::has_media()
{
	return false;
}

int XineEngine::get_time()
{
	int result = 0;
	xine_get_pos_length(stream, NULL, &result, NULL);
	return result;
}

int XineEngine::get_length()
{
	int length = 0;
	xine_get_pos_length(stream, NULL, NULL, &length);
	return length;
}

void XineEngine::on_expose(GdkEventExpose* event_expose)
{
	if (event_expose->count == 0)
	{
		XExposeEvent xevent;

		xevent.type = Expose;
		xevent.serial = 0;
		xevent.send_event = true;
		xevent.display = display;
		xevent.window = window;
		xevent.x = event_expose->area.x;
		xevent.y = event_expose->area.y;
		xevent.width = event_expose->area.width;
		xevent.height = event_expose->area.height;
		xevent.count = 0;

		xine_port_send_gui_data(video_port, XINE_GUI_SEND_EXPOSE_EVENT, &xevent);
    }
}

float XineEngine::get_percentage()
{
	int position = 0;
	int time = 0;
	int length = 0;
	xine_get_pos_length(stream, &position, &time, &length);
	return position == 0 ? (time/(float)length) : (position/(float)65535);
}

void XineEngine::set_percentage(float percentage)
{
	int value = percentage * 65535;
	if (value > 65535)
	{
		value = 65535;
	}
	else if (value < 0)
	{
		value = 0;
	}
	
	if (!xine_play(stream, value, 0))
	{
		throw Exception("Failed to set percentage");
	}
}

void XineEngine::set_volume(double value)
{
	xine_set_param(stream, XINE_PARAM_AUDIO_VOLUME,value);
}

double XineEngine::get_volume()
{
	return xine_get_param(stream, XINE_PARAM_AUDIO_VOLUME);
}

void XineEngine::set_mute_state(gboolean mute)
{
	xine_set_param(stream, XINE_PARAM_AUDIO_MUTE, mute ? 1 : 0);
}

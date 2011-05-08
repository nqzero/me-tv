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

#ifndef __THREAD_H__
#define __THREAD_H__

#include <glibmm.h>

class Thread
{
private:
	gboolean				terminated;
	Glib::Thread*			thread;
	Glib::StaticRecMutex	mutex;
	Glib::ustring			name;
	gboolean				join_on_destroy;
	gboolean				started;

	void on_run();

public:
	Thread(const Glib::ustring& name, gboolean join_on_destroy = true);
	virtual ~Thread();

	virtual void run() = 0;

	void start();
	gboolean is_started();

	void terminate();
	gboolean is_terminated();

	void join(gboolean set_terminate = false);
};

#endif

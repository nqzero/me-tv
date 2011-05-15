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

#ifndef __DVB_SCANNER_H__
#define __DVB_SCANNER_H__

#include <vector>
#include "dvb_frontend.h"
#include "dvb_service.h"
#include "dvb_transponder.h"

namespace Dvb
{	
	class Scanner
	{
	private:
		gboolean terminated;
		TransponderList transponders;
			
		void tune_to(Frontend& frontend, const Transponder& transponder, const String& text_encoding, guint read_timeout);
		void atsc_tune_to(Frontend& frontend, const Transponder& transponder, const String& text_encoding, guint read_timeout);
	public:
		Scanner();

		void start(Frontend& frontend, TransponderList& transponders, const String& text_encoding, guint timeout);
		void terminate();
			
		sigc::signal<void,const struct dvb_frontend_parameters&, guint, const String&, const guint, guint> signal_service;
		sigc::signal<void, guint, gsize> signal_progress;
		sigc::signal<void> signal_complete;
	};
}

#endif

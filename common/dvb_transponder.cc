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
 
#include "dvb_transponder.h"
#include "dvb_service.h"
#include "exception.h"
#include "i18n.h"

using namespace Dvb;

Transponder::Transponder() : polarisation(0), satellite_number(0), hi_band(0)
{
	frontend_type = FE_OFDM;
	memset(&frontend_parameters, 0, sizeof(frontend_parameters));
}

gboolean TransponderList::exists(const Transponder& transponder)
{
	for (const_iterator i = begin(); i != end(); i++)
	{
		if (transponder == *i)
		{
			return true;
		}
	}
	
	return false;
}

void TransponderList::add(const Transponder& transponder)
{
	if (!exists(transponder))
	{
		push_back(transponder);
	}
}

bool Transponder::operator==(const Transponder& transponder) const
{
	return transponder == frontend_parameters;
}

bool Transponder::operator!=(const Transponder& transponder) const
{
	return transponder != frontend_parameters;
}

bool Transponder::operator==(struct dvb_frontend_parameters p) const
{
	return frontend_parameters.frequency == p.frequency && (
		(frontend_type == FE_OFDM && frontend_parameters.u.ofdm.bandwidth == p.u.ofdm.bandwidth) ||
		(frontend_type == FE_ATSC && frontend_parameters.u.vsb.modulation == p.u.vsb.modulation) ||
		(frontend_type == FE_QAM && frontend_parameters.u.qam.modulation == p.u.qam.modulation) ||
	    frontend_type == FE_QPSK
	);
}

bool Transponder::operator!=(struct dvb_frontend_parameters p) const
{
	return !(*this == p);
}

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

#include "dvb_service.h"
#include "dvb_transponder.h"

using namespace Dvb;

Service::Service(Transponder& service_transponder) : transponder(service_transponder)
{
	id = 0;
}

gboolean Service::operator ==(const Service& service) const
{
	return service.id == id && service.transponder == transponder;
}

gboolean Service::operator !=(const Service& service) const
{
	return !(*this == service);
}


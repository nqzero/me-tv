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


#ifndef __ME_TV_I18N_H__
#define __ME_TV_I18N_H__

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef ENABLE_NLS
#	include <glibmm/i18n.h>
#else /* NLS is disabled */
	#define textdomain(String) (String)
	#define gettext(String) (String)
	#define dgettext(Domain,String) (String)
	#define dcgettext(Domain,String,Type) (String)
	#define bindtextdomain(Domain,Directory) (Domain)
	#define ngettext(StringSingular,StringPlural,Number) \
		((Number) == 1 ? (StringSingular) : (StringPlural))
	#define dngettext(StringDomain,StringSingular,StringPlural,Number) \
		((Number) == 1 ? (StringSingular) : (StringPlural))
	#define dncgettext(StringDomain,StringSingular,StringPlural,Number,Type) \
		((Number) == 1 ? (StringSingular) : (StringPlural))
	#define bind_textdomain_codeset(Domain,Codeset) ((Codeset))
	#define _(String) (String)
	#define Q_(String) g_strip_context ((String), (String))
	#define N_(String) (String)
	#define C_(Context,String) (String)
	#define NC_(Context,String) (String)
#endif /* ENABLE_NLS */

#endif //__ME_TV_I18N_H__

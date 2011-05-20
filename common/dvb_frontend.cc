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

#include "dvb_frontend.h"
#include "dvb_transponder.h"
#include "exception.h"
#include "i18n.h"

using namespace Dvb;

Frontend::Frontend(const Adapter& frontend_adapter, guint frontend_index)
	: adapter(frontend_adapter)
{
	fd = -1;
	memset(&frontend_parameters, 0, sizeof(struct dvb_frontend_parameters));
	frontend = frontend_index;
}

Frontend::~Frontend()
{
	close();
	g_debug("Frontend destroyed");
}

void Frontend::open()
{
	if (fd == -1)
	{
		String path = adapter.get_frontend_path(frontend);
		g_debug("Opening frontend device: %s", path.c_str());
		if ( (fd = ::open( path.c_str(), O_RDWR | O_NONBLOCK) ) < 0 )
		{
			throw SystemException(_("Failed to open tuner"));
		}
	
		if (ioctl(fd, FE_GET_INFO, &frontend_info) < 0)
		{
			throw SystemException(_("Failed to get tuner info"));
		}
	}
}

void Frontend::close()
{
	if (fd != -1)
	{
		String path = adapter.get_frontend_path(frontend);
		g_debug("Closing frontend device: %s", path.c_str());
		
		::close(fd);
		fd = -1;
	}
}

void Frontend::tune_to(const Transponder& transponder, guint timeout)
{
	g_message(_("Frontend::tune_to(%d)"), transponder.frontend_parameters.frequency);

	if (fd == -1)
	{
		throw Exception("Frontend has not been opened");
	}

	frontend_parameters.frequency = 0;
	
	struct dvb_frontend_parameters parameters = transponder.frontend_parameters;
	struct dvb_frontend_event ev;
	
	if (frontend_info.type == FE_QPSK)
	{
		guint hi_band = 0;
		
		if(LNB_HIGH_VALUE > 0 && LNB_SWITCH_VALUE > 0 && parameters.frequency >= LNB_SWITCH_VALUE)
		{
			hi_band = 1;
		}
		
		diseqc(0, transponder.polarisation, hi_band);
		
		if(hi_band == 1)
		{
			parameters.frequency = abs(transponder.frontend_parameters.frequency - LNB_HIGH_VALUE);
		}
		else
		{
			parameters.frequency = abs(transponder.frontend_parameters.frequency - LNB_LOW_VALUE);
		}

		g_debug("Tuning DVB-S device to %d with symbol rate %d and inner fec %d", parameters.frequency, parameters.u.qpsk.symbol_rate, parameters.u.qpsk.fec_inner);
		g_debug("Diseqc: Hiband: %d, polarisation: %d - new freq: %d", hi_band, transponder.polarisation, parameters.frequency);
		usleep(500000);
	}

	// Discard stale events
	do {} while (ioctl(fd, FE_GET_EVENT, &ev) != -1);
		
	gint return_code = ioctl ( fd, FE_SET_FRONTEND, &parameters );
	if (return_code < 0)
	{
		throw SystemException(_("Failed to tune device") );
	}
	
	g_message(_("Waiting for signal lock ..."));
	wait_lock(timeout);
	g_message(_("Got signal lock"));
	
	frontend_parameters = transponder.frontend_parameters;
}

void Frontend::diseqc(int satellite_number, int polarisation, int hi_band)
{	
	struct dvb_diseqc_master_cmd cmd = { {0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4};
	cmd.msg[3] = 0xf0 | (((satellite_number * 4) & 0x0f) | (hi_band ? 1 : 0) | (polarisation ? 0 : 2));
	
	fe_sec_voltage_t	voltage	= (polarisation == POLARISATION_VERTICAL) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	fe_sec_tone_mode_t	tone	= (hi_band == 1) ? SEC_TONE_ON : SEC_TONE_OFF;
	fe_sec_mini_cmd_t	burst	= (satellite_number / 4) % 2 ? SEC_MINI_B : SEC_MINI_A;

	if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) == -1)
	{
		throw SystemException(_("Failed to set tone off"));
	}
	
	if (ioctl(fd, FE_SET_VOLTAGE, voltage) == -1)
	{
		throw SystemException(_("Failed to set voltage"));
	}

	usleep(15 * 1000);
	if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1)
	{
		throw SystemException(_("Failed to send master command"));
	}

	usleep(19 * 1000);
	if (ioctl(fd, FE_DISEQC_SEND_BURST, burst) == -1)
	{
		throw SystemException(_("Failed to send burst"));
	}
	
	usleep(15 * 1000);
	if (ioctl(fd, FE_SET_TONE, tone) == -1)
	{
		throw SystemException(_("Failed to set tone"));
	}
}

const struct dvb_frontend_info& Frontend::get_frontend_info() const
{
	return frontend_info;
}

void Frontend::wait_lock(guint timeout)
{
	fe_status_t	status;
	guint count = 0;
	
	while (count < timeout)
	{
		if (!ioctl(fd, FE_READ_STATUS, &status))
		{
			if (status & FE_HAS_LOCK)
			{
				break;
			}
		}

		usleep(100000);
		count += 100;
	}

	if (!(status & FE_HAS_LOCK))
	{
		g_debug("Status: %d", status);
		throw Exception(_("Failed to lock to channel"));
	}
}

guint Frontend::get_signal_strength()
{
	guint result = 0;
	if (ioctl(fd, FE_READ_SIGNAL_STRENGTH, &result) == -1)
	{
		throw SystemException(_("Failed to get signal strength"));
	}
	return result;
}

guint Frontend::get_snr()
{
	guint result = 0;
	if (ioctl(fd, FE_READ_SNR, &result) == -1)
	{
		throw SystemException(_("Failed to get signal to noise ratio"));
	}
	return result;
}

const struct dvb_frontend_parameters& Frontend::get_frontend_parameters() const
{
	return frontend_parameters;
}

/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

#include "kernel_wintun_api.h"

#include <utils/debug.h>

#define RESOLVE(api, module, member, symbol) \
	do { \
		(api)->member = (typeof((api)->member))(void*) \
						GetProcAddress((module), (symbol)); \
		if (!(api)->member) \
		{ \
			DBG1(DBG_LIB, "resolving %s from wintun.dll failed: " \
				 "Windows error %lu", (symbol), GetLastError()); \
			goto failed; \
		} \
	} while (FALSE)

bool kernel_wintun_api_load(HMODULE module, kernel_wintun_api_t *api)
{
	memset(api, 0, sizeof(*api));

	RESOLVE(api, module, create_adapter, "WintunCreateAdapter");
	RESOLVE(api, module, open_adapter, "WintunOpenAdapter");
	RESOLVE(api, module, close_adapter, "WintunCloseAdapter");
	RESOLVE(api, module, delete_driver, "WintunDeleteDriver");
	RESOLVE(api, module, get_adapter_luid, "WintunGetAdapterLUID");
	RESOLVE(api, module, get_running_driver_version,
			"WintunGetRunningDriverVersion");
	RESOLVE(api, module, set_logger, "WintunSetLogger");
	RESOLVE(api, module, start_session, "WintunStartSession");
	RESOLVE(api, module, end_session, "WintunEndSession");
	RESOLVE(api, module, get_read_wait_event, "WintunGetReadWaitEvent");
	RESOLVE(api, module, receive_packet, "WintunReceivePacket");
	RESOLVE(api, module, release_receive_packet,
			"WintunReleaseReceivePacket");
	RESOLVE(api, module, allocate_send_packet, "WintunAllocateSendPacket");
	RESOLVE(api, module, send_packet, "WintunSendPacket");

	return TRUE;

failed:
	memset(api, 0, sizeof(*api));
	return FALSE;
}

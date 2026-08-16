/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/**
 * @defgroup kernel_wintun_api kernel_wintun_api
 * @{ @ingroup kernel_wintun
 */

#ifndef KERNEL_WINTUN_API_H_
#define KERNEL_WINTUN_API_H_

#include <utils/utils.h>

#include <ifdef.h>

typedef struct kernel_wintun_api_t kernel_wintun_api_t;

typedef struct kernel_wintun_adapter_t *kernel_wintun_adapter_handle_t;
typedef struct kernel_wintun_session_t *kernel_wintun_session_handle_t;

typedef enum {
	KERNEL_WINTUN_LOG_INFO,
	KERNEL_WINTUN_LOG_WARN,
	KERNEL_WINTUN_LOG_ERR,
} kernel_wintun_logger_level_t;

typedef void (WINAPI *kernel_wintun_logger_t)(
	kernel_wintun_logger_level_t level, DWORD64 timestamp, LPCWSTR message);

/**
 * Typed Wintun API entry points resolved from wintun.dll.
 *
 * Keeping calls behind this table allows tests in later patches to substitute
 * a mock implementation without loading the DLL.
 */
struct kernel_wintun_api_t {
	kernel_wintun_adapter_handle_t (WINAPI *create_adapter)(
		LPCWSTR name, LPCWSTR tunnel_type, const GUID *requested_guid);
	kernel_wintun_adapter_handle_t (WINAPI *open_adapter)(LPCWSTR name);
	void (WINAPI *close_adapter)(kernel_wintun_adapter_handle_t adapter);
	BOOL (WINAPI *delete_driver)(void);
	void (WINAPI *get_adapter_luid)(kernel_wintun_adapter_handle_t adapter,
		NET_LUID *luid);
	DWORD (WINAPI *get_running_driver_version)(void);
	void (WINAPI *set_logger)(kernel_wintun_logger_t logger);
	kernel_wintun_session_handle_t (WINAPI *start_session)(
		kernel_wintun_adapter_handle_t adapter, DWORD capacity);
	void (WINAPI *end_session)(kernel_wintun_session_handle_t session);
	HANDLE (WINAPI *get_read_wait_event)(
		kernel_wintun_session_handle_t session);
	BYTE *(WINAPI *receive_packet)(kernel_wintun_session_handle_t session,
		DWORD *packet_size);
	void (WINAPI *release_receive_packet)(
		kernel_wintun_session_handle_t session, const BYTE *packet);
	BYTE *(WINAPI *allocate_send_packet)(
		kernel_wintun_session_handle_t session, DWORD packet_size);
	void (WINAPI *send_packet)(kernel_wintun_session_handle_t session,
		const BYTE *packet);
};

/**
 * Resolve the Wintun API from a loaded module.
 *
 * On failure, the table is cleared and the caller retains ownership of the
 * module.
 *
 * @param module	loaded wintun.dll module
 * @param api		API table to populate
 * @return		TRUE if every required entry point was resolved
 */
bool kernel_wintun_api_load(HMODULE module, kernel_wintun_api_t *api);

#endif /** KERNEL_WINTUN_API_H_ @}*/

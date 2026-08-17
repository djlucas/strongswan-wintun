/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

#include <test_suite.h>

#include "../kernel_wintun_device.h"

struct kernel_wintun_adapter_t {
	int unused;
};

struct kernel_wintun_session_t {
	int unused;
};

typedef enum {
	TRACE_END_SESSION,
	TRACE_CLOSE_ADAPTER,
} trace_event_t;

typedef struct {
	kernel_wintun_context_t context;
	struct kernel_wintun_adapter_t adapter;
	struct kernel_wintun_session_t session;
	BYTE send_buffer[2048];
	DWORD allocation_size;
	trace_event_t trace[2];
	u_int active_at_trace[2];
	int trace_count;
	int create_calls;
	int start_calls;
	int allocate_calls;
	int send_calls;
	int end_calls;
	int close_calls;
	bool fail_create;
	bool fail_start;
	bool fail_allocate;
} mock_state_t;

static mock_state_t *state;

/**
 * Record a destruction callback and the device count at the API boundary.
 */
static void record_trace(trace_event_t event)
{
	ck_assert(state->trace_count < countof(state->trace));
	state->trace[state->trace_count] = event;
	/* The device destructor invokes these callbacks while holding the lifecycle
	 * mutex.  Read the counter directly; attempting to lock the non-recursive
	 * mutex here would deadlock. */
	state->active_at_trace[state->trace_count] =
		state->context.active_devices;
	state->trace_count++;
}

static kernel_wintun_adapter_handle_t WINAPI mock_create_adapter(
	LPCWSTR name, LPCWSTR tunnel_type, const GUID *requested_guid)
{
	state->create_calls++;
	return state->fail_create ? NULL : &state->adapter;
}

static void WINAPI mock_close_adapter(
	kernel_wintun_adapter_handle_t adapter)
{
	ck_assert(adapter == &state->adapter);
	state->close_calls++;
	record_trace(TRACE_CLOSE_ADAPTER);
}

static kernel_wintun_session_handle_t WINAPI mock_start_session(
	kernel_wintun_adapter_handle_t adapter, DWORD capacity)
{
	ck_assert(adapter == &state->adapter);
	state->start_calls++;
	return state->fail_start ? NULL : &state->session;
}

static void WINAPI mock_end_session(kernel_wintun_session_handle_t session)
{
	ck_assert(session == &state->session);
	state->end_calls++;
	record_trace(TRACE_END_SESSION);
}

static BYTE *WINAPI mock_allocate_send_packet(
	kernel_wintun_session_handle_t session, DWORD packet_size)
{
	ck_assert(session == &state->session);
	state->allocate_calls++;
	state->allocation_size = packet_size;
	if (state->fail_allocate || packet_size > sizeof(state->send_buffer))
	{
		return NULL;
	}
	return state->send_buffer;
}

static void WINAPI mock_send_packet(kernel_wintun_session_handle_t session,
	const BYTE *packet)
{
	ck_assert(session == &state->session);
	ck_assert(packet == state->send_buffer);
	state->send_calls++;
}

/**
 * Initialize a device context backed by the mock Wintun API.
 */
static void init_state(mock_state_t *mock)
{
	memset(mock, 0, sizeof(*mock));
	state = mock;
	mock->context.lifecycle = mutex_create(MUTEX_TYPE_DEFAULT);
	mock->context.api.create_adapter = mock_create_adapter;
	mock->context.api.close_adapter = mock_close_adapter;
	mock->context.api.start_session = mock_start_session;
	mock->context.api.end_session = mock_end_session;
	mock->context.api.allocate_send_packet = mock_allocate_send_packet;
	mock->context.api.send_packet = mock_send_packet;
}

static void destroy_state(mock_state_t *mock)
{
	mock->context.lifecycle->destroy(mock->context.lifecycle);
	state = NULL;
}

START_TEST(test_send_packet)
{
	mock_state_t mock;
	tun_device_t *device;
	uint8_t data[] = { 0x45, 0x00, 0x00, 0x04 };

	init_state(&mock);
	device = kernel_wintun_device_create(&mock.context, "ipsec%d");
	ck_assert(device != NULL);
	ck_assert_int_eq(mock.context.active_devices, 1);

	ck_assert(device->write_packet(device, chunk_create(data, sizeof(data))));
	ck_assert_int_eq(mock.allocate_calls, 1);
	ck_assert_int_eq(mock.send_calls, 1);
	ck_assert_int_eq(mock.allocation_size, sizeof(data));
	ck_assert(memeq(mock.send_buffer, data, sizeof(data)));

	device->destroy(device);
	ck_assert_int_eq(mock.end_calls, 1);
	ck_assert_int_eq(mock.close_calls, 1);
	ck_assert_int_eq(mock.trace_count, 2);
	ck_assert_int_eq(mock.trace[0], TRACE_END_SESSION);
	ck_assert_int_eq(mock.trace[1], TRACE_CLOSE_ADAPTER);
	ck_assert_int_eq(mock.active_at_trace[0], 1);
	ck_assert_int_eq(mock.active_at_trace[1], 1);
	ck_assert_int_eq(mock.context.active_devices, 0);
	destroy_state(&mock);
}
END_TEST

START_TEST(test_allocate_failure)
{
	mock_state_t mock;
	tun_device_t *device;
	uint8_t data[] = { 0x45 };

	init_state(&mock);
	mock.fail_allocate = TRUE;
	device = kernel_wintun_device_create(&mock.context, NULL);
	ck_assert(device != NULL);
	ck_assert(!device->write_packet(device, chunk_create(data, sizeof(data))));
	ck_assert_int_eq(mock.allocate_calls, 1);
	ck_assert_int_eq(mock.send_calls, 0);
	device->destroy(device);
	ck_assert_int_eq(mock.context.active_devices, 0);
	destroy_state(&mock);
}
END_TEST

START_TEST(test_oversized_packet)
{
	mock_state_t mock;
	tun_device_t *device;
	chunk_t packet = { .len = 0x10000 };

	init_state(&mock);
	device = kernel_wintun_device_create(&mock.context, NULL);
	ck_assert(device != NULL);
	ck_assert(!device->write_packet(device, packet));
	ck_assert_int_eq(mock.allocate_calls, 0);
	ck_assert_int_eq(mock.send_calls, 0);
	device->destroy(device);
	ck_assert_int_eq(mock.context.active_devices, 0);
	destroy_state(&mock);
}
END_TEST

START_TEST(test_teardown_rejects_send)
{
	mock_state_t mock;
	tun_device_t *device;
	uint8_t data[] = { 0x45 };

	init_state(&mock);
	device = kernel_wintun_device_create(&mock.context, NULL);
	ck_assert(device != NULL);
	mock.context.lifecycle->lock(mock.context.lifecycle);
	mock.context.teardown_requested = TRUE;
	mock.context.lifecycle->unlock(mock.context.lifecycle);

	ck_assert(!device->write_packet(device, chunk_create(data, sizeof(data))));
	ck_assert_int_eq(mock.allocate_calls, 0);
	ck_assert_int_eq(mock.send_calls, 0);
	device->destroy(device);
	ck_assert_int_eq(mock.context.active_devices, 0);
	destroy_state(&mock);
}
END_TEST

START_TEST(test_adapter_creation_failure)
{
	mock_state_t mock;
	tun_device_t *device;

	init_state(&mock);
	mock.fail_create = TRUE;
	device = kernel_wintun_device_create(&mock.context, NULL);
	ck_assert(device == NULL);
	ck_assert_int_eq(mock.create_calls, 1);
	ck_assert_int_eq(mock.start_calls, 0);
	ck_assert_int_eq(mock.end_calls, 0);
	ck_assert_int_eq(mock.close_calls, 0);
	ck_assert_int_eq(mock.context.active_devices, 0);
	ck_assert(!mock.context.device_was_created);
	destroy_state(&mock);
}
END_TEST

START_TEST(test_session_creation_failure)
{
	mock_state_t mock;
	tun_device_t *device;

	init_state(&mock);
	mock.fail_start = TRUE;
	device = kernel_wintun_device_create(&mock.context, NULL);
	ck_assert(device == NULL);
	ck_assert_int_eq(mock.create_calls, 1);
	ck_assert_int_eq(mock.start_calls, 1);
	ck_assert_int_eq(mock.end_calls, 0);
	ck_assert_int_eq(mock.close_calls, 1);
	ck_assert_int_eq(mock.trace_count, 1);
	ck_assert_int_eq(mock.trace[0], TRACE_CLOSE_ADAPTER);
	ck_assert_int_eq(mock.active_at_trace[0], 0);
	ck_assert_int_eq(mock.context.active_devices, 0);
	ck_assert(!mock.context.device_was_created);
	destroy_state(&mock);
}
END_TEST

Suite *device_suite_create(void)
{
	Suite *suite;
	TCase *tc;

	suite = suite_create("Wintun device");
	tc = tcase_create("send path");
	tcase_add_test(tc, test_send_packet);
	tcase_add_test(tc, test_allocate_failure);
	tcase_add_test(tc, test_oversized_packet);
	tcase_add_test(tc, test_teardown_rejects_send);
	suite_add_tcase(suite, tc);
	tc = tcase_create("lifecycle accounting");
	tcase_add_test(tc, test_adapter_creation_failure);
	tcase_add_test(tc, test_session_creation_failure);
	suite_add_tcase(suite, tc);
	return suite;
}

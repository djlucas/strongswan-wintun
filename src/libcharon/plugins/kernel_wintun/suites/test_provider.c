/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

#include <test_suite.h>

#include <utils/debug.h>

#include "../kernel_wintun_api.h"
#include "../kernel_wintun_provider.h"

static char message[BUF_LEN];

/**
 * Capture the provider's absent-DLL diagnostic.
 */
static void capture_log(debug_t group, level_t level, char *fmt, ...)
{
	va_list args;
	size_t len;

	len = strlen(message);
	if (len && len < sizeof(message) - 1)
	{
		message[len++] = '\n';
		message[len] = '\0';
	}
	va_start(args, fmt);
	vsnprintf(message + len, sizeof(message) - len, fmt, args);
	va_end(args);
}

START_TEST(test_absent_dll)
{
	kernel_wintun_provider_t *provider;
	void (*previous_dbg)(debug_t group, level_t level, char *fmt, ...);

	/* This test requires the clean-checkout invariant: no wintun.dll may be
	 * placed beside the test executable.  If it starts returning a provider,
	 * first check the build tree for a stray DLL. */
	message[0] = '\0';
	previous_dbg = dbg;
	dbg = capture_log;
	provider = kernel_wintun_provider_create();
	dbg = previous_dbg;

	ck_assert(provider == NULL);
	ck_assert_msg(strstr(message, "loading wintun.dll from the application "
								 "directory failed") != NULL,
				  "unexpected diagnostic: %s", message);
}
END_TEST

START_TEST(test_missing_exports)
{
	kernel_wintun_api_t api, empty = {};
	void (*previous_dbg)(debug_t group, level_t level, char *fmt, ...);
	HMODULE module;
	bool loaded;

	module = GetModuleHandleW(L"kernel32.dll");
	ck_assert(module != NULL);
	memset(&api, 0xA5, sizeof(api));
	message[0] = '\0';
	previous_dbg = dbg;
	dbg = capture_log;
	loaded = kernel_wintun_api_load(module, &api);
	dbg = previous_dbg;

	ck_assert(!loaded);
	ck_assert_msg(strstr(message, "resolving WintunCreateAdapter") != NULL,
				  "unexpected diagnostic: %s", message);
	ck_assert(memeq(&api, &empty, sizeof(api)));
}
END_TEST

Suite *provider_suite_create(void)
{
	Suite *suite;
	TCase *tc;

	suite = suite_create("Wintun provider");
	tc = tcase_create("absent DLL");
	tcase_add_test(tc, test_absent_dll);
	suite_add_tcase(suite, tc);
	tc = tcase_create("API resolution");
	tcase_add_test(tc, test_missing_exports);
	suite_add_tcase(suite, tc);
	return suite;
}

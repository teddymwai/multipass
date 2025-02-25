/*
 * Copyright (C) 2018-2022 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MULTIPASS_MOCK_SSH_CLIENT
#define MULTIPASS_MOCK_SSH_CLIENT

#include <premock.hpp>

#include <libssh/libssh.h>

DECL_MOCK(ssh_channel_request_shell);
DECL_MOCK(ssh_channel_request_pty);
DECL_MOCK(ssh_channel_change_pty_size);

#endif // MULTIPASS_MOCK_SSH_CLIENT

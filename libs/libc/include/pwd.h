/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

#pragma once

// includes
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

struct passwd {
    char* pw_name;
    char* pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char* pw_gecos;
    char* pw_dir;
    char* pw_shell;
};
typedef struct passwd passwd_t;

void setpwent();
void endpwent();
passwd_t* getpwent();
passwd_t* getpwuid(uid_t uid);
passwd_t* getpwnam(const char* name);

__END_DECLS

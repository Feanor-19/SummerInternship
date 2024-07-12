#pragma once

extern "C" { 
#include <sss_nss_idmap.h> 
}

bool dlink_open_sss_nss_idmap(const char **err_msg_p = nullptr);

bool dlink_close_sss_nss_idmap(const char **err_msg_p = nullptr);
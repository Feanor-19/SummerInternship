#pragma once

#include <stddef.h>

extern "C"
{
#include <sss_nss_idmap.h>
}

//! @brief Finds entry with the given `key` in the `kv_list`.
//! @return Found entry or NULL, if not found.
sss_nss_kv *find_kv_entry_by_key(sss_nss_kv *kv_list, const char *key);

//! @brief Parses `distinguished name` into key-value list (`sss_nss_kv`).
//! @example "CN=Administrator,CN=Users,DC=NUMENOR,DC=online" will be parsed into
//!          [ {"CN", "Administrator"}, {"CN", "Users"}, {"DC", "NUMENOR"}, {"DC", "online"} ]
//! @attention Returned `kv_list` must be freed using `sss_nss_free_kv`.
//! @note If given `dist_name` isn't properly formed, NULL is returned AND err_code = EPROTO.
//! @note Error text can be obtained with strerror(error code) (see string.h)
sss_nss_kv *parse_dist_name( const char *dist_name, int *err_code_p = NULL );
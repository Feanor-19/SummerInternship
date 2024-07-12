#pragma once

#include <stddef.h>

extern "C"
{
#include <sss_nss_idmap.h>
}

//! @brief Finds entry with the given `key` in the `kv_list`.
//! @return Found entry or NULL, if not found.
sss_nss_kv *find_kv_entry_by_key(sss_nss_kv *kv_list, const char *key);


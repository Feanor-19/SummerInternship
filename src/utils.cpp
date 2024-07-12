#include "utils.h"

#include <sss_nss_idmap.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

sss_nss_kv *find_kv_entry_by_key(sss_nss_kv *kv_list, const char *key)
{
    assert(kv_list);
    assert(key);

    while (kv_list->key != NULL && strcmp(kv_list->key, key) != 0) kv_list++;

    if (kv_list->key != NULL)
        return kv_list;

    return NULL;
}

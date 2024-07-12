#include "link.h"

#include <dlfcn.h>

int (*DL_sss_nss_getorigbyname)(const char*,struct sss_nss_kv**,enum sss_id_type*) = nullptr;
typedef int (*DL_sss_nss_getorigbyname_t)(const char*,struct sss_nss_kv**,enum sss_id_type*);

void (*DL_sss_nss_free_kv)(struct sss_nss_kv*) = nullptr;
typedef void (*DL_sss_nss_free_kv_t)(struct sss_nss_kv*);

int (*DL_sss_nss_getsidbyuid)(uint32_t,char **,enum sss_id_type*) = nullptr;
typedef int (*DL_sss_nss_getsidbyuid_t)(uint32_t,char **,enum sss_id_type*);


void *DL_handle_sss_nss_idmap = nullptr;


bool dlink_open_sss_nss_idmap(const char **err_msg_p)
{
    void *handle = dlopen("libsss_nss_idmap.so", RTLD_LAZY);
    if (!handle) goto Failed;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconditionally-supported"
    DL_sss_nss_getorigbyname = (DL_sss_nss_getorigbyname_t) dlsym(handle, "sss_nss_getorigbyname");
    if (!DL_sss_nss_getorigbyname) goto Failed;

    DL_sss_nss_free_kv = (DL_sss_nss_free_kv_t) dlsym(handle, "sss_nss_free_kv");
    if (!DL_sss_nss_free_kv) goto Failed;

    DL_sss_nss_getsidbyuid = (DL_sss_nss_getsidbyuid_t) dlsym(handle, "sss_nss_getsidbyuid");
    if (!DL_sss_nss_getsidbyuid) goto Failed;
#pragma GCC diagnostic pop

    DL_handle_sss_nss_idmap = handle;

    return true;

Failed:
    if (err_msg_p) 
        *err_msg_p = dlerror();
    if (handle) dlclose(handle);
    return false;
}

bool dlink_close_sss_nss_idmap(const char **err_msg_p)
{
    if (DL_handle_sss_nss_idmap)
    {
        if (dlclose(DL_handle_sss_nss_idmap) != 0)
        {
            if (err_msg_p)
                *err_msg_p = dlerror();

            return false;
        }

        DL_handle_sss_nss_idmap = nullptr;
    }

    return true;
}

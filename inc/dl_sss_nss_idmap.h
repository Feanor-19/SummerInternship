#pragma once

extern "C" {
 #include <sss_nss_idmap.h>
 }


extern int (*DL_sss_nss_getorigbyname)(const char*,struct sss_nss_kv**,enum sss_id_type*);

extern void (*DL_sss_nss_free_kv)(struct sss_nss_kv*);

extern int (*DL_sss_nss_getsidbyuid)(uint32_t,char **,enum sss_id_type*);

extern int (*DL_sss_nss_getsidbygid)(uint32_t id, char **sid, enum sss_id_type *type);


#pragma once

#include <stdint.h>
#include <errno.h>

// see https://github.com/SSSD/sssd/blob/4e95d6f6c1bdd90bd1ca666fa2989bcda443f0a4/src/sss_client/idmap/sss_nss_idmap.h#L36
enum sss_id_type {
    SSS_ID_TYPE_NOT_SPECIFIED = 0,
    SSS_ID_TYPE_UID,
    SSS_ID_TYPE_GID,
    SSS_ID_TYPE_BOTH /* used for user or magic private groups */
};


// see https://github.com/SSSD/sssd/blob/4e95d6f6c1bdd90bd1ca666fa2989bcda443f0a4/src/sss_client/idmap/sss_nss_idmap.h#L122
/**
 * @brief Find SID by a POSIX UID
 *
 * @param[in] uid      POSIX UID
 * @param[out] sid     String representation of the SID of the requested user,
 *                     must be freed by the caller
 * @param[out] type    Type of the object related to the given ID
 *
 * @return 
 *  - 0 (EOK): success, sid contains the requested SID
 *  - ENOENT: requested object was not found in the domain extracted from the given name
 *  - ENETUNREACH: SSSD does not know how to handle the domain extracted from the given name
 *  - ENOSYS: this call is not supported by the configured provider
 *  - EINVAL: input cannot be parsed
 *  - EIO: remote servers cannot be reached
 *  - EFAULT: any other error
 */
extern int sss_nss_getsidbyuid(uint32_t uid, char **sid, enum sss_id_type *type);

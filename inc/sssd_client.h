#pragma once

#include <unistd.h>

extern "C"
{
#include <sss_nss_idmap.h>
}


//! @brief Converts given UID to SID.
//! @param[in]  uid         POSIX UID;
//! @param[out] SID         String representation of the SID of the requested user;
//! @attention  `SID` MUST BE FREED BY CALLER!
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @param[out] error_text  Text, explaining error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. 
//! @note Error text can be also obtained with strerror(error code) (see string.h)
bool uid_to_sid(uid_t UID, char **SID_p, int* error_code_p = NULL, const char **error_text_p = NULL);

//! @brief Checks if given UID is found in the domain.
//! @attention If `true` is returned, given UID IS found in the domain.
//!            If `false` is returned AND *error_code_p == 0, given UID IS NOT found.
//!            If `false` is returned AND *error_code_p != 0, some error occured.
//! @param[in]  uid         POSIX UID;
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @param[out] error_text  Text, explaining error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. 
//! @note Error text can be also obtained with strerror(error code) (see string.h)
bool is_domain_uid(uid_t UID, int* error_code_p = NULL, const char **error_text_p = NULL);

//! @brief Parses `distinguished name` into key-value list (`sss_nss_kv`).
//! @example "CN=Administrator,CN=Users,DC=NUMENOR,DC=online" will be parsed into
//!          [ {"CN", "Administrator"}, {"CN", "Users"}, {"DC", "NUMENOR"}, {"DC", "online"} ]
//! @attention Returned `kv_list` must be freed by caller using `sss_nss_free_kv`.
//! @note If given `dist_name` isn't properly formed, NULL is returned AND err_code = EPROTO.
//! @note Error text can be obtained with strerror(error code) (see string.h)
sss_nss_kv *parse_dist_name( const char *dist_name, int *err_code_p = NULL );

//! @brief Returns current domain's name, using libnss.
//! @attention This function tries to get domain's name from current user 'original data',
//!            fetched from libnss, joining fields of type `DC` with dots. It can give wrong 
//!            answer in some cases, be careful.
//! @param[out] name_p      Domain name; !MUSTE BE FREED BY CALLER!
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. 
//! @note Error text can be obtained with strerror(error code) (see string.h)
bool get_own_domain_name_nss(char **name_p, int *error_code_p = NULL);

//NOTE --------------------------------------DEPRECATED--------------------------------------------
// //! @brief Returns current domain's name, using D-Bus to ask SSSD for list of all domains.
// //! @attention This function tries to get domain's name, asking SSSD using D-Bus for list 
// //!            of _all_ domains, assuming there is only one, and assuming that this is the 
// //!            one current user belongs to. It can give wrong 
// //!            answer in some cases, be careful. REQUIRES SUDO.
// //! @param[out] name_p      Domain name; !MUSTE BE FREED BY CALLER!
// //! @param[out] error_code  Error code (optional, changed only if `false` is returned);
// //! @return `true` if everything is okay, `false` if some error occurs. 
// //! @note Error text can be obtained with strerror(error code) (see string.h)
// //! @note If there is some error concerning D-Bus, err_code = EPROTO and err_dbus_msg is filled.
// bool get_own_domain_name_dbus(char **name_p, int *error_code_p = NULL, const char **err_dbus_msg_p = NULL);

//! @brief Returns current domain's SID.
//! @param[out] SID_p      Domain SID; !MUSTE BE FREED BY CALLER!
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. 
//! @note Error text can be obtained with strerror(error code) (see string.h)
bool get_own_domain_sid(char **SID_p, int *error_code_p = NULL);

//! @brief Executes "ping -c 1 *domain_name*", trying to check whether domain controller 
//!        is online or not.
//! @param[in] domain_name Full name of the domain, which cab ne pinged.
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` AND err_code == 0 if ping is unsuccessful,
//!         `false` AND err_code != 0 if some error occurs. 
//! @attention Uses `ping` return codes, which are platform dependent. See enum `PingRetCode`.
//!            If `ping` returns something which is not part of the enum, err_code == EPROTO.
//! @note To suppress output to stdout, '>nul 2>nul' is used.
bool ping_domain(const char *domain_name, bool suppress_output, int *error_code_p = NULL);
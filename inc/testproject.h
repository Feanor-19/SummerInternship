#pragma once

#include <unistd.h>

//! @brief Prints current user groups into stdout.
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. 
//! @note Error text can be obtained with strerror(error code) (see string.h)
bool print_groups(int *error_code_p = nullptr);

//! @brief Prints current user SID.
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. (e.g. if user doesn't have SID)
//! @note Error text can be obtained with strerror(error code) (see string.h)
bool print_sid(int *error_code_p = nullptr);

//! @brief Converts given UID to SID.
//! @param[in]  uid         POSIX UID;
//! @param[out] SID         String representation of the SID of the requested user;
//! @attention  `SID` MUST BE FREED BY CALLER!
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @param[out] error_text  Text, explaining error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. 
//! @note Error text can be also obtained with strerror(error code) (see string.h)
bool uid_to_sid(uid_t UID, char **SID_p, int* error_code_p = nullptr, char **error_text_p = nullptr);

//! @brief Checks if given UID is found in the domain.
//! @attention If `true` is returned, given UID IS found in the domain.
//!            If `false` is returned AND *error_code_p == 0, given UID IS NOT found.
//!            If `false` is returned AND *error_code_p != 0, some error occured.
//! @param[in]  uid         POSIX UID;
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @param[out] error_text  Text, explaining error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. 
//! @note Error text can be also obtained with strerror(error code) (see string.h)
bool is_domain_uid(uid_t UID, int* error_code_p = nullptr, char **error_text_p = nullptr);
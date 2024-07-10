#pragma once

#include "sssd_client.h"


//! @brief Prints current user groups into stdout.
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. 
//! @note Error text can be obtained with strerror(error code) (see string.h)
bool print_groups(int *error_code_p = NULL);

//! @brief Prints current user SID.
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. (e.g. if user doesn't have SID)
//! @note Error text can be obtained with strerror(error code) (see string.h)
bool print_sid(int *error_code_p = NULL);

//! @brief Prints current user's domain info.
//! @param[out] error_code  Error code (optional, changed only if `false` is returned);
//! @return `true` if everything is okay, `false` if some error occurs. (e.g. if user doesn't have SID)
//! @note Error text can be obtained with strerror(error code) (see string.h)
bool print_domain_info(int *error_code_p = NULL);

#pragma once

//! @brief Prints current user groups into stdout.
//! @return 0 if no errors occured, not 0 otherwise.
int print_groups();

//! @brief Prints current user SID 
//! @return 0 if no errors occured, not 0 otherwise (e.g. if user doesn't have SID)
int print_sid();

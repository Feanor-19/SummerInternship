#include "io.h"
#include "utils.h"

#include "dl_sss_nss_idmap.h"

#include <stdio.h>
#include <sys/types.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <assert.h>

bool print_domain_groups(int *error_code_p)
{
    int err = 0;
    bool res = true;
    
    char *SID = NULL;
    DomainGroups dom_groups = {};

    if ( !(uid_to_sid(getuid(), &SID, &err) ) )
        goto CleanUp;

    if ( !(get_domain_groups_by_user_sid(SID, &dom_groups, &err)) )
        goto CleanUp;

    if (dom_groups.n_groups == 0)
    {
        err = ENOENT;
        goto CleanUp;
    }

    printf("Domain Groups:\n");
    for (size_t ind = 0; ind < dom_groups.n_groups; ind++ )
    {
        printf("Group name: %s\nGID: %u\nSID: %s\n\n", dom_groups.list[ind].name,
                                                       dom_groups.list[ind].gid,
                                                       dom_groups.list[ind].sid);
    }

CleanUp:
    if (err != 0) res = false;
    free_DomainGroups(&dom_groups);
    free(SID);
    if (*error_code_p && res == false) *error_code_p = err;
    return res;
}

bool print_sid(int *error_code_p)
{
    char *SID = NULL;
    bool res = false;

    if ( !(res = uid_to_sid(getuid(), &SID, error_code_p) ) )
        goto CleanUp;

    printf("User SID: %s\n", SID);

CleanUp:
    free(SID);

    return res;
}

bool print_domain_info(int *error_code_p)
{
    int err = 0;
    bool res = true;

    char *name = NULL;
    if (get_own_domain_name_nss(&name, &err))
    {
        printf("Domain name using NSS: %s\n", name);
        free(name);
    }
    else
    {
        res = false;
        printf("Failed to get own domain info using NSS: %s\n", strerror(err));
    }

    //NOTE This method of finding current domain name turned out to have more cons than pros.
    // const char *dbus_err_msg = NULL;
    // if (get_own_domain_name_dbus(&name, &err, &dbus_err_msg))
    // {
    //     printf("Domain name using D-Bus: %s\n", name);
    //     free(name);
    // }
    // else
    // {
    //     res = false;
    //     printf("Failed to get own domain info using D-Bus: %s\n", strerror(err));
    //     if (err == EPROTO) printf("D-Bus error: %s\n", dbus_err_msg);
    // }

    char *SID = NULL;
    if (get_own_domain_sid(&SID, &err))
    {
        printf("Domain SID: %s\n", SID);
        free(SID);
    }
    else
    {
        res = false;
        printf("Failed to get own domain SID: %s\n", strerror(err));
    }

    if (error_code_p && err != 0)
        *error_code_p = err;

    return res;
}
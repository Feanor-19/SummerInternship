#include "io.h"
#include "utils.h"

#include <stdio.h>
#include <sys/types.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <assert.h>

bool print_groups(int *error_code_p)
{
    bool res = true;
    
    printf("Groups:\n");
    
    int n_groups = getgroups(0, NULL);
    if (n_groups == 0)
    {
        printf("No groups found.\n");
        return true;
    }

    gid_t *groups = (gid_t*) calloc( (size_t) n_groups, sizeof(gid_t) );
    if (!getgroups(n_groups, groups))
    {
        printf("Can't get groups' ids.\n");
        *error_code_p = errno;
        res = false;
        goto CleanUp;
    }

    for (int ind  = 0; ind < n_groups; ind++)
    {
        putchar('\n');
        struct group *gr = NULL;
        if ( (gr = getgrgid(groups[ind])) )
        {
            printf("Group name: %s\n"
                   "GID:        %u\n",
                   gr->gr_name, gr->gr_gid);


            ////TODO
            char *SID = NULL;
            sss_id_type type = SSS_ID_TYPE_NOT_SPECIFIED;

            if (sss_nss_getsidbygid(gr->gr_gid, &SID, &type) == 0)
                printf("SID: %s\n", SID);

            free(SID);
            ////
        }
    }

CleanUp:
    free(groups);

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
    if (get_own_domain_name_nss(&name, &err)) //TODO
    {
        printf("Domain name using NSS: %s\n", name);
        free(name);
    }
    else
    {
        res = false;
        printf("Failed to get own domain info using NSS: %s\n", strerror(err));
    }

    const char *dbus_err_msg = NULL;
    if (get_own_domain_name_dbus(&name, &err, &dbus_err_msg))
    {
        printf("Domain name using D-Bus: %s\n", name);
        free(name);
    }
    else
    {
        res = false;
        printf("Failed to get own domain info using D-Bus: %s\n", strerror(err));
        if (err == EPROTO) printf("D-Bus error: %s\n", dbus_err_msg);
    }

    if (error_code_p && err != 0)
        *error_code_p = err;

    return res;
}
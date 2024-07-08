#include "testproject.h"

extern "C"
{
#include "sss_nss_idmap.h"
}

#include <stdio.h>
#include <sys/types.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


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
        }
    }

CleanUp:
    free(groups);

    return res;
}

bool print_sid(int *error_code_p)
{
    char *SID = nullptr;
    bool res = false;

    if ( !(res = uid_to_sid(getuid(), &SID, error_code_p) ) )
        goto CleanUp;

    printf("SID: %s\n", SID);

CleanUp:
    free(SID);

    return res;
}

bool uid_to_sid(uid_t UID, char **SID_p, int* error_code_p, char **error_text_p)
{
    enum sss_id_type type = SSS_ID_TYPE_NOT_SPECIFIED;
    int err = sss_nss_getsidbyuid(UID, SID_p, &type);

    if (err)
    {
        if (error_code_p)
            *error_code_p = err;
        
        if (error_text_p)
            *error_text_p = strerror(err);

        return false;
    }

    return true;
}

bool is_domain_uid(uid_t UID, int* error_code_p, char **error_text_p)
{
    char *SID = nullptr;
    char *error_text_old_val = nullptr;
    
    if (error_text_p)
        error_text_old_val = *error_text_p;

    bool res = uid_to_sid(UID, &SID, error_code_p, error_text_p);
    if (error_code_p && (res == false) && *error_code_p == ENOENT)
    {
        *error_code_p = 0;
        if (error_text_p)
            *error_text_p = error_text_old_val;
    }

    free(SID);

    return res;
}
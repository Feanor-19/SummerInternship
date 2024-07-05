extern "C"{
#include "sss_nss_idmap.h"
}

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <string.h>
#include <errno.h>

#include <stdint.h>

//! @brief Prints current user groups into stdout.
//! @return 0 if no errors occured, not 0 otherwise.
int print_groups();

//! @brief Prints current user SID 
//! @return 0 if no errors occured, not 0 otherwise (e.g. if user doesn't have SID)
int print_sid();

int main()
{
    int err = 0;

    printf("Current user info:\n");

    printf("UID:      %u\n"
           "Username: %s\n", 
           getuid(), getlogin());

    if ( (err = print_sid()) != 0)
    {
        printf("Some non-crtical error occured during printing current user SID:\n");
        perror(strerror(err));
    }

    if ( (err = print_groups()) != 0)
    {
        printf("Some non-crtical error occured during printing current user groups:\n");
        perror(strerror(err));
    }
    
    return 0;
}

int print_groups()
{
    int err = 0;
    
    printf("Groups:\n");
    
    int n_groups = getgroups(0, NULL);
    if (n_groups == 0)
    {
        printf("No groups found.\n");
        return 0;
    }

    gid_t *groups = (gid_t*) calloc( (size_t) n_groups, sizeof(gid_t) );
    if (!getgroups(n_groups, groups))
    {
        printf("Can't get groups' ids.\n");
        err = errno;
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

    return err;
}

int print_sid()
{
    char *sid = NULL;
    enum sss_id_type type = SSS_ID_TYPE_NOT_SPECIFIED;
    int err = sss_nss_getsidbyuid(getuid(), &sid, &type);

    if (err)
        goto CleanUp;

    printf("SID: %s\n", sid);

CleanUp:
    free(sid);

    return err;
}
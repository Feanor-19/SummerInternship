#include "testproject.h"

extern "C"
{
#include "sss_nss_idmap.h"
}

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


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
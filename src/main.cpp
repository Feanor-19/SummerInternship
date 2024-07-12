#include "io.h"
#include "link.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    const char *dlink_err_msg = NULL;
    if (!dlink_open_sss_nss_idmap(&dlink_err_msg))
    {
        printf("DINAMIC LINK ERROR: Failed to open: %s", dlink_err_msg);
        exit(-1);
    }    

    int err = 0;

    printf("Current user info:\n");

    printf("UID:      %u\n"
           "Username: %s\n", 
           getuid(), getlogin()); 
    
    if (is_domain_uid(getuid(), &err))
    {
        if (!print_sid(&err))
        {
            printf("Some non-crtical error occured during printing current user SID:\n");
            perror(strerror(err)); 
        }
        
        if (!print_domain_info(&err))
        {
            printf("Some non-crtical error occured during printing current domain info:\n");
            perror(strerror(err));
        }

        printf("Trying to ping domain contoller...\n");
        char *domain_name = NULL;
        if (get_own_domain_name_nss(&domain_name, &err))
        {
            if (ping_domain(domain_name, true, &err))
            {
                printf("Domain contoller is online!\n");
            }
            else printf("Failed to ping %s: %s\n", domain_name, (err == 0 ? "no answer" : strerror(err)));

            free(domain_name);
        }
        else printf("Failed to get domain name: %s\n", strerror(err));
    }

    
    else if (err == 0)
    {
        printf("This UID doesn't belong to current domain, so no SID exists.\n");
    }
    else
    {
        printf("Some non-crtical error occured during checking if current user is in the domain:\n");
        perror(strerror(err)); 
    }

    if ( !print_groups(&err))
    {
        printf("Some non-crtical error occured during printing current user groups:\n");
        perror(strerror(err));
    }


    if (!dlink_close_sss_nss_idmap(&dlink_err_msg))
    {
        printf("DINAMIC LINK ERROR: Failed to close: %s", dlink_err_msg);
        exit(-1);
    }
    
    return 0;
}

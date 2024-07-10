#include "io.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main()
{
    //test();

    int err        = 0;
    //char *err_text = NULL;

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
    
    return 0;
}

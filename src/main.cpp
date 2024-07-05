#include "testproject.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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

#include "testproject.h"
#include "utils.h"

extern "C"
{
#include <sss_nss_idmap.h>
}

#include <stdio.h>
#include <sys/types.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <assert.h>



// void test()
// {

// }






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
    bool res = false;

    char *name = NULL;
    char *SID  = NULL;
    if (get_own_domain_name(&name, &err) && true) //TODO
    {
        res = true;
        printf("Domain name: %s\nDomain SID: %s\n", name, SID);
    }

    if (error_code_p && err != 0)
        *error_code_p = err;

    free(name);
    free(SID);

    return res;
}

bool uid_to_sid(uid_t UID, char **SID_p, int* error_code_p, char **error_text_p)
{
    assert(SID_p);

    sss_id_type type = SSS_ID_TYPE_NOT_SPECIFIED;
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
    char *SID = NULL;
    char *error_text_old_val = NULL;
    
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

//! @brief Forms domain name, joining entries with key 'DC' with '.'
static char *form_domain_name(sss_nss_kv *kv_list)
{
    assert(kv_list);

    const char *DC = "DC";

    // compute the resulting length
    size_t cnt = 0;
    size_t len = 0;
    sss_nss_kv *last = NULL;
    for (size_t ind = 0; kv_list[ind].key != NULL; ind++)
    {
        if (strcmp(kv_list[ind].key, DC) == 0)
        {
            len += strlen(kv_list[ind].value);
            cnt++;
            last = &kv_list[ind];
        }
    }    

    if (cnt == 0) return NULL;

    len += cnt-1; //accounting for separating '.'

    char *name = (char*) calloc(len+1, sizeof(char));
    if (!name) return NULL;

    char *name_cur = name;

    for (size_t ind = 0; kv_list[ind].key != NULL; ind++)
    {
        if (strcmp(kv_list[ind].key, DC) == 0)
        {
            strcpy(name_cur, kv_list[ind].value);
            name_cur += strlen(kv_list[ind].value);
            
            if (&kv_list[ind] != last) 
            {
                strcpy(name_cur, ".");
                name_cur += 1;
            }
        }
    } 

    *name_cur = '\0';

    return name;
}

bool get_own_domain_name(char **name_p, int *error_code_p)
{
    assert(name_p);

    int err = 0;

    sss_nss_kv *kv_orig_list = NULL;
    sss_id_type type         = SSS_ID_TYPE_NOT_SPECIFIED;
    sss_nss_kv *original_DN  = NULL;

    sss_nss_kv *kv_from_orig = NULL;

    err = sss_nss_getorigbyname( getlogin(), &kv_orig_list, &type );
    if (err != 0) goto CleanUp; 

    original_DN = find_kv_entry_by_key(kv_orig_list, KV_LIST_ORIGINAL_DN);
    if (!original_DN) 
    {
        err = ENOKEY;
        goto CleanUp;
    }

    //DEBUG
    //printf("orginial_DN: %s\n", original_DN->value);

    kv_from_orig = parse_dist_name(original_DN->value, &err);
    if (err != 0) goto CleanUp;

    //DEBUG
    // for (size_t ind = 0; kv_from_orig[ind].key != NULL; ind++)
    // {
    //     printf("%s\t\t: %s\n", kv_from_orig[ind].key, kv_from_orig[ind].value);
    // }

    *name_p = form_domain_name(kv_from_orig);
    if ( !(*name_p) )
    {
        err = ENOMEM;
        goto CleanUp;
    }

CleanUp:
    if (kv_orig_list) sss_nss_free_kv(kv_orig_list);
    if (kv_from_orig) sss_nss_free_kv(kv_from_orig);
    
    if (error_code_p) *error_code_p = err;
    if (err != 0) return false;
    else          return true;
}
#include "sssd_client.h"
#include "utils.h"

// extern "C"
// {
// #include <sss_nss_idmap.h>
// //#include <systemd/sd-bus.h> //NOTE - for deprecated `get_own_domain_name_dbus()`
// }

#include "dl_sss_nss_idmap.h"

#include <stdio.h>
#include <sys/types.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <assert.h>

bool uid_to_sid(uid_t UID, char **SID_p, int* error_code_p, const char **error_text_p)
{
    assert(SID_p);

    sss_id_type type = SSS_ID_TYPE_NOT_SPECIFIED;
    int err = DL_sss_nss_getsidbyuid(UID, SID_p, &type);

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

bool is_domain_uid(uid_t UID, int* error_code_p, const char **error_text_p)
{
    char *SID = NULL;
    const char *error_text_old_val = NULL;
    
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

    const char * const DC = "DC";

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

//REVIEW - можно использовать готовые алгоритмы из C++, но стоит ли ради этого
// мешать очень Сишный код с C++?
//! @brief Counts number of occurences of `c` in `str`.
inline size_t count_char_in_str(const char *str, char c)
{
    assert(str);

    size_t ans = 0;
    while (*str != '\0')
    {
        if (c == *str) ans++;
        str++;
    }

    return ans;
}

sss_nss_kv *parse_dist_name( const char *dist_name, int *err_code_p)
{
    enum parse_dist_state
    {
        PD_START, //< start state
        PD_ATTR,  //< attribute
        PD_EQU,   //< between `attribute` and `value`, corresponds to `=`
        PD_VAL,   //< value
    };

    assert(dist_name);

    int err = 0;
    const char *cur = dist_name;
    parse_dist_state st = PD_START;
    const char *substr_start = NULL;
    size_t substr_cnt = 0;
    size_t kv_ind = 0;

    size_t kv_size = count_char_in_str(dist_name, '=');

    sss_nss_kv *kv_list = (sss_nss_kv*) calloc(kv_size+1, sizeof(sss_nss_kv));
    if (!kv_list)
    {
        err = ENOMEM;
        goto Error;
    }

    while (*cur != '\0')
    {
        if (*cur == '\\')
        {
            if (st == PD_ATTR || st == PD_VAL) substr_cnt+=2;
            cur+=2;
            continue;
        }

        switch (st)
        {
        case PD_START:
            st = PD_ATTR;
            substr_start = cur;
            substr_cnt = 1;
            break;
        case PD_ATTR:
            if (*cur == '=') 
            {   
                st = PD_EQU;
                kv_list[kv_ind].key = strndup(substr_start, substr_cnt);
            }
            else
                substr_cnt++;
            break;
        case PD_EQU:
            st = PD_VAL;
            substr_start = cur;
            substr_cnt = 1;
            break;
        case PD_VAL:
            if (*cur == ',') 
            {   
                st = PD_START;
                kv_list[kv_ind].value = strndup(substr_start, substr_cnt);
                kv_ind++;
            }
            else
                substr_cnt++;
            break;
        default:
            assert(0 && "Unreacheable line!");
            break;
        }

        cur++;
    }    

    if (kv_ind + 1 != kv_size)
        goto ParseError;

    if (st == PD_VAL)
        kv_list[kv_ind].value = strndup(substr_start, substr_cnt);
    else
        goto ParseError;

    kv_list[kv_size].key   = NULL;
    kv_list[kv_size].value = NULL;
    
    return kv_list;

ParseError:
    err = EPROTO;
Error:
    if (err_code_p) *err_code_p = err;
    if (kv_list) DL_sss_nss_free_kv(kv_list);
    return NULL;
}

bool get_own_domain_name_nss(char **name_p, int *error_code_p)
{
    assert(name_p);

    const char * const KV_LIST_ORIGINAL_DN = "originalDN";

    int err = 0;

    sss_nss_kv *kv_orig_list = NULL;
    sss_id_type type         = SSS_ID_TYPE_NOT_SPECIFIED;
    sss_nss_kv *original_DN  = NULL;

    sss_nss_kv *kv_from_orig = NULL;

    err = DL_sss_nss_getorigbyname( getlogin(), &kv_orig_list, &type );
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
    if (kv_orig_list) DL_sss_nss_free_kv(kv_orig_list);
    if (kv_from_orig) DL_sss_nss_free_kv(kv_from_orig);
    
    if (error_code_p && err != 0) *error_code_p = err;
    if (err != 0) return false;
    else          return true;
}

//NOTE --------------------------------------DEPRECATED--------------------------------------------
// //! @brief Used only in `get_own_domain_name_dbus()`.
// #define SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP(_msg) { 
//     if (err_dbus_msg_p) *err_dbus_msg_p = (_msg); 
//     err = EPROTO;                                 
//     goto CleanUp;                                 
// } 

// //! @brief Used only in `get_own_domain_name_dbus()`.
// #define FREE_DOMAINS_LIST(_list) do{                        
//     for (size_t ind = 0; _list[ind] != NULL; ind++)  
//         free(_list[ind]);                            
//     free(_list);                                     
// }while(0)       

// bool get_own_domain_name_dbus(char **name_p, int *error_code_p, const char **err_dbus_msg_p)
// {
//     assert(name_p);

//     int err = 0;

//     sd_bus *bus           = NULL;
//     sd_bus_error error    = SD_BUS_ERROR_NULL;
//     sd_bus_message *reply = NULL;
//     char **domains_list   = NULL;
//     char *domain_name     = NULL;

//     int r = 0;
//     r = sd_bus_open_system(&bus);
//     if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to open system D-Bus.");

//     r = sd_bus_call_method(bus, "org.freedesktop.sssd.infopipe",
//                                 "/org/freedesktop/sssd/infopipe",
//                                 "org.freedesktop.sssd.infopipe",
//                                 "ListDomains", &error, &reply, "");
//     if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to call D-Bus method ListDomains. (maybe sudo is needed)")

//     r = sd_bus_message_read_strv(reply, &domains_list);
//     if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to read D-Bus reply of ListDomains");

//     if (domains_list == NULL) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("No domains are returned by ListDomains");

//     if (domains_list[1] != NULL) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("There is more than one domain returned by ListDomains");

//     //DEBUG
//     //printf("Domain object: %s\n", domains_list[0]);

//     r = sd_bus_call_method(bus, "org.freedesktop.sssd.infopipe",
//                                 domains_list[0],
//                                 "org.freedesktop.DBus.Properties",
//                                 "Get", &error, &reply, "ss", 
//                                 "org.freedesktop.sssd.infopipe.Domains",
//                                 "name");
//     if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to call D-Bus method Get (property 'name'). (maybe sudo is needed)");

//     r = sd_bus_message_read(reply, "v", "s", &domain_name);
//     if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to read D-Bus reply of Get (property 'name')");

//     *name_p = strdup(domain_name);
//     if (!(*name_p))
//     {
//         err = ENOMEM;
//         goto CleanUp;
//     }
//     // domain_name must not be freed on its own

// CleanUp:
//     sd_bus_flush_close_unrefp(&bus);
//     sd_bus_error_free(&error);
//     sd_bus_message_unrefp(&reply);

//     FREE_DOMAINS_LIST(domains_list);

//     if (error_code_p && err != 0) *error_code_p = err;
//     if (err != 0) return false;
//     else          return true;
// }

bool get_own_domain_sid(char **SID_p, int *error_code_p)
{
    assert(SID_p);

    int err = 0;
    char *sid = NULL;
    if (!uid_to_sid(getuid(), &sid, &err))
        goto Failed;

    // User SID without the last section, separated with '-', is the domain SID.
    // That's why it's enough to change the last occurence of '-' with '\0'.
    *(strrchr(sid, '-')) = '\0';

    *SID_p = sid;
    return true;

Failed:
    free(sid);
    if (*error_code_p) *error_code_p = err;
    return false;
}

//! @brief Return values of command `ping`. Platform dependent.
enum PingRetCode
{
    PING_OK = 0,
    PING_NO_ANS = 1,
    PING_ERR = 2
};

bool ping_domain(const char *domain_name, bool suppress_output, int *error_code_p)
{
    assert(domain_name);

    int err = 0;
    bool res = false;
    int ans = -1;

    const char CMD_PING_BASE[] = "ping -c 1 ";
    const size_t CMD_PING_BASE_LEN = sizeof(CMD_PING_BASE) - 1;

    const char CMD_PING_END[] = " >nul 2>nul";
    const size_t CMD_PING_END_LEN = sizeof(CMD_PING_END) - 1;

    char *cmd = (char*) calloc(CMD_PING_BASE_LEN + strlen(domain_name) + CMD_PING_END_LEN + 1, sizeof(char));
    if (!cmd)
    {
        err = ENOMEM;
        goto CleanUp;
    }

    strcpy(cmd, CMD_PING_BASE);
    strcpy(cmd + CMD_PING_BASE_LEN, domain_name);
    if (suppress_output) 
        strcpy(cmd + CMD_PING_BASE_LEN + strlen(domain_name), CMD_PING_END);

    //DEBUG
    //printf("cmd: %s\n", cmd);

    ans = system(cmd);
    switch (ans)
    {
    case PING_OK:
        res = true;
        break;
    case PING_NO_ANS:
        res = false;
        break;
    case PING_ERR:
    default:
        res = false;
        err = EPROTO;
        break;
    }

CleanUp:
    free(cmd);
    if (*error_code_p) *error_code_p = err;
    return res;
}

static void free_DomainGroup(DomainGroup *gr)
{
    if (gr)
    {
        free(gr->name);
        free(gr->sid);
    }
}

void free_DomainGroups(DomainGroups *grs)
{
    if (grs)
    {
        for (size_t ind = 0; ind < grs->n_groups; ind++)
        {
            free_DomainGroup(&(grs->list[ind]));
        }
    }
}

bool get_domain_groups_by_user_sid(const char *user_SID, DomainGroups *groups_p, int *error_code_p)
{
    assert(user_SID);
    assert(groups_p);

    int err = 0;

    gid_t *unix_groups = NULL;
    DomainGroups domain_groups = {};
    size_t d_ind = 0;
    
    int n_unix_groups = getgroups(0, NULL);
    if (n_unix_groups == 0)
    {
        err = errno;
        goto Failed;
    }

    unix_groups = (gid_t*) calloc( (size_t) n_unix_groups, sizeof(gid_t) );

    // actually n_domain_groups can be less than n_unix_groups, but it's not so important
    domain_groups.list = (DomainGroup*) calloc( (size_t) n_unix_groups, sizeof(DomainGroup) );

    if (!getgroups(n_unix_groups, unix_groups))
    {
        err = errno;
        goto Failed;
    }

    for (int u_ind  = 0; u_ind < n_unix_groups; u_ind++)
    {
        struct group *gr = NULL;
        if ( (gr = getgrgid(unix_groups[u_ind])) )
        {            
            char *SID = NULL;
            sss_id_type type = SSS_ID_TYPE_NOT_SPECIFIED;

            int sid_err = DL_sss_nss_getsidbygid(gr->gr_gid, &SID, &type); 
            if (sid_err == 0)
            {
                domain_groups.list[d_ind].gid = gr->gr_gid;
                domain_groups.list[d_ind].name = strdup(gr->gr_name);
                domain_groups.list[d_ind].sid = SID;
                d_ind++;
            }
            else if (sid_err == ENOENT) // not a domain group
                continue;
            else
            {
                err = sid_err;
                goto Failed;
            }
        }
    }

    domain_groups.n_groups = d_ind;

    *groups_p = domain_groups;
    return true;

Failed:
    free(unix_groups);
    free_DomainGroups(&domain_groups);
    if (*error_code_p) *error_code_p = err;
    return false;
}
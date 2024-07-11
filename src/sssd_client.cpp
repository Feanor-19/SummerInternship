#include "sssd_client.h"
#include "utils.h"

extern "C"
{
#include <sss_nss_idmap.h>
#include <systemd/sd-bus.h>
}

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

bool get_own_domain_name_nss(char **name_p, int *error_code_p)
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
    
    if (error_code_p && err != 0) *error_code_p = err;
    if (err != 0) return false;
    else          return true;
}

//! @brief Used only in `get_own_domain_name_dbus()`.
#define SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP(_msg) { \
    if (err_dbus_msg_p) *err_dbus_msg_p = (_msg); \
    err = EPROTO;                                 \
    goto CleanUp;                                 \
} 

//! @brief Used only in `get_own_domain_name_dbus()`.
#define FREE_DOMAINS_LIST(_list) do{                        \
    for (size_t ind = 0; _list[ind] != NULL; ind++)  \
        free(_list[ind]);                            \
    free(_list);                                     \
}while(0)       

bool get_own_domain_name_dbus(char **name_p, int *error_code_p, const char **err_dbus_msg_p)
{
    assert(name_p);

    int err = 0;

    sd_bus *bus           = NULL;
    sd_bus_error error    = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    char **domains_list   = NULL;
    char *domain_name     = NULL;

    int r = 0;
    r = sd_bus_open_system(&bus);
    if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to open system D-Bus.");

    r = sd_bus_call_method(bus, "org.freedesktop.sssd.infopipe",
                                "/org/freedesktop/sssd/infopipe",
                                "org.freedesktop.sssd.infopipe",
                                "ListDomains", &error, &reply, "");
    if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to call D-Bus method ListDomains. (maybe sudo is needed)")

    r = sd_bus_message_read_strv(reply, &domains_list);
    if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to read D-Bus reply of ListDomains");

    if (domains_list == NULL) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("No domains are returned by ListDomains");

    if (domains_list[1] != NULL) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("There is more than one domain returned by ListDomains");

    //DEBUG
    //printf("Domain object: %s\n", domains_list[0]);

    r = sd_bus_call_method(bus, "org.freedesktop.sssd.infopipe",
                                domains_list[0],
                                "org.freedesktop.DBus.Properties",
                                "Get", &error, &reply, "ss", 
                                "org.freedesktop.sssd.infopipe.Domains",
                                "name");
    if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to call D-Bus method Get (property 'name'). (maybe sudo is needed)");

    r = sd_bus_message_read(reply, "v", "s", &domain_name);
    if (r < 0) SET_DBUS_ERR_MSG_AND_GOTO_CLEANUP("Failed to read D-Bus reply of Get (property 'name')");

    *name_p = strdup(domain_name);
    // domain_name must not be freed on its own

CleanUp:
    sd_bus_flush_close_unrefp(&bus);
    sd_bus_error_free(&error);
    sd_bus_message_unrefp(&reply);

    FREE_DOMAINS_LIST(domains_list);

    if (error_code_p && err != 0) *error_code_p = err;
    if (err != 0) return false;
    else          return true;
}

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

enum PingAnswer
{
    PA_SUCCESS,
    PA_NO_ANS,
    PA_NOT_KNOWN,
    PA_ERROR,
    PA_UNEXPECTED,
};

const size_t BUFLEN = 1024;
const char *EXEC_PING_ERR_MSG = "exec_err: %d";
const int EXEC_PING_ERR_MSG_TRUE_SCANF_RES = 1;

static PingAnswer parse_ping_answer(const char *ping_answer, int *error_code_p)
{
    assert(ping_answer);
    assert(error_code_p);

    // checking for one-line answers
    int err = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    int scanf_res = sscanf(ping_answer, EXEC_PING_ERR_MSG, &err);
#pragma GCC diagnostic pop
    if (scanf_res == EXEC_PING_ERR_MSG_TRUE_SCANF_RES)
    {
        *error_code_p = err;
        return PA_ERROR;
    }

    const char WORD_KNOWN[] = "known";
    char word_known_buf[sizeof(WORD_KNOWN)] = "";
    scanf_res = sscanf(ping_answer, "%*s %*s Name or service not %5s", word_known_buf);
    if (scanf_res == 1 && strcmp(word_known_buf, WORD_KNOWN))
        return PA_NOT_KNOWN;

    const char WORD_RESOLUTION[] = "resolution";
    char word_resolution_buf[sizeof(WORD_RESOLUTION)] = "";
    scanf_res = sscanf(ping_answer, "%*s %*s Temporary failure in name %5s", word_resolution_buf);
    if (scanf_res == 1 && strcmp(word_resolution_buf, WORD_RESOLUTION))
        return PA_NO_ANS;

    // maybe answer is multi-lined
    const char *cur = ping_answer;
    int skipped_lines = 0;
    while (skipped_lines < 3 && *cur != '\0')
    {
        while (*cur != '\n') cur++;
        skipped_lines++;
        cur++;
    }

    if (*cur == '\0')
        cur = ping_answer;

    int transmitted = 0, received = 0;
    // expected: "1 packets transmitted, 1"
    scanf_res = sscanf(cur, "%d %*s %*s %d", &transmitted, &received);
    if (scanf_res == 2 && transmitted == received)
        return PA_SUCCESS;
    else if (received == 0 && transmitted != 0)
        return PA_NO_ANS;

    return PA_UNEXPECTED;
}

bool ping_domain(const char *domain_name, int *error_code_p)
{
    assert(domain_name);

    int err = 0;
    PingAnswer ping_ans = PA_ERROR;

    char buf[BUFLEN] = "";
    
    pid_t pid = 0;

    int pipe_arr[2];
    int res = pipe(pipe_arr);
    if (res == -1)
    {
        err = errno;
        goto Failed;
    }

    if( (pid = fork()) == 0)
    {
        dup2(pipe_arr[1], STDOUT_FILENO);
        int execres = execl("/bin/ping", "/bin/ping", "-c", "1", "-q", domain_name, (char*)NULL);
        if (execres == -1)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            printf(EXEC_PING_ERR_MSG, errno); // writing into pipe
#pragma GCC diagnostic pop
        exit(errno);
    }

    waitpid(pid, NULL, 0);
    if (read(pipe_arr[0], buf, BUFLEN) < 0)
    {
        err = errno;
        goto Failed;
    }

    close(pipe_arr[0]);
    close(pipe_arr[1]);

    ping_ans = parse_ping_answer(buf, &err);
    switch (ping_ans)
    {
    case PA_SUCCESS:
        return true;
        break;
    case PA_NO_ANS:
    case PA_NOT_KNOWN:
        return false;
        break;
    case PA_UNEXPECTED:
        err = EPROTO;
        goto Failed;
        break;
    case PA_ERROR:
        goto Failed; // err already set
    default:
        assert(0 && "Unreacheable line!");
        break;
    }

    assert(0 && "Unreacheable line!");
    return false;

Failed:
    if (*error_code_p) *error_code_p = err;
    return false;
}
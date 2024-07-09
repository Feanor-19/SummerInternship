#include "utils.h"

#include <sss_nss_idmap.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

sss_nss_kv *find_kv_entry_by_key(sss_nss_kv *kv_list, const char *key)
{
    assert(kv_list);
    assert(key);

    while (kv_list->key != NULL && strcmp(kv_list->key, key) != 0) kv_list++;

    if (kv_list->key != NULL)
        return kv_list;

    return NULL;
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

enum parse_dist_state
{
    PD_START, //< start state
    PD_ATTR,  //< attribute
    PD_EQU,   //< between `attribute` and `value`, corresponds to `=`
    PD_VAL,   //< value
};

sss_nss_kv *parse_dist_name( const char *dist_name, int *err_code_p)
{
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
    if (kv_list) sss_nss_free_kv(kv_list);
    return NULL;
}
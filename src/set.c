/**
 * @file set.c
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief Generic set routines implementations
 *
 * Copyright (c) 2015 - 2018 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#include "libyang.h"
#include "common.h"

API struct ly_set *
ly_set_new(void)
{
    struct ly_set *new;

    new = calloc(1, sizeof(struct ly_set));
    LY_CHECK_ERR_RET(!new, LOGMEM(NULL), NULL);
    return new;
}

API void
ly_set_clean(struct ly_set *set, void (*destructor)(void *obj))
{
    unsigned int u;

    LY_CHECK_ARG_RET(NULL, set,);

    if (destructor) {
        for (u = 0; u < set->count; ++u) {
            destructor(set->objs[u]);
        }
    }
    set->count = 0;
}

API void
ly_set_erase(struct ly_set *set, void (*destructor)(void *obj))
{
    LY_CHECK_ARG_RET(NULL, set,);

    ly_set_clean(set, destructor);

    free(set->objs);
    set->size = 0;
    set->objs = NULL;
}

API void
ly_set_free(struct ly_set *set, void (*destructor)(void *obj))
{
    LY_CHECK_ARG_RET(NULL, set,);

    ly_set_erase(set, destructor);

    free(set);
}

API int
ly_set_contains(const struct ly_set *set, void *object)
{
    unsigned int i;

    LY_CHECK_ARG_RET(NULL, set, -1);

    for (i = 0; i < set->count; i++) {
        if (set->objs[i] == object) {
            /* object found */
            return i;
        }
    }

    /* object not found */
    return -1;
}

API struct ly_set *
ly_set_dup(const struct ly_set *set)
{
    struct ly_set *new;

    LY_CHECK_ARG_RET(NULL, set, NULL);

    new = malloc(sizeof *new);
    LY_CHECK_ERR_RET(!new, LOGMEM(NULL), NULL);
    new->count = set->count;
    new->size = set->size;
    new->objs = malloc(new->size * sizeof *(new->objs));
    LY_CHECK_ERR_RET(!new->objs, LOGMEM(NULL); free(new), NULL);
    memcpy(new->objs, set->objs, new->size * sizeof *(new->objs));

    return new;
}

API int
ly_set_add(struct ly_set *set, void *object, int options)
{
    unsigned int i;
    void **new;

    LY_CHECK_ARG_RET(NULL, set, object, -1);

    if (!(options & LY_SET_OPT_USEASLIST)) {
        /* search for duplication */
        for (i = 0; i < set->count; i++) {
            if (set->objs[i] == object) {
                /* already in set */
                return i;
            }
        }
    }

    if (set->size == set->count) {
        new = realloc(set->objs, (set->size + 8) * sizeof *(set->objs));
        LY_CHECK_ERR_RET(!new, LOGMEM(NULL), -1);
        set->size += 8;
        set->objs = new;
    }

    set->objs[set->count++] = object;

    return set->count - 1;
}

API int
ly_set_merge(struct ly_set *trg, struct ly_set *src, int options)
{
    unsigned int i, ret;
    void **new;

    LY_CHECK_ARG_RET(NULL, trg, -1);
    LY_CHECK_ARG_RET(NULL, src, 0);

    if (!(options & LY_SET_OPT_USEASLIST)) {
        /* remove duplicates */
        i = 0;
        while (i < src->count) {
            if (ly_set_contains(trg, src->objs[i]) > -1) {
                ly_set_rm_index(src, i);
            } else {
                ++i;
            }
        }
    }

    /* allocate more memory if needed */
    if (trg->size < trg->count + src->count) {
        new = realloc(trg->objs, (trg->count + src->count) * sizeof *(trg->objs));
        LY_CHECK_ERR_RET(!new, LOGMEM(NULL), -1);
        trg->size = trg->count + src->count;
        trg->objs = new;
    }

    /* copy contents from src into trg */
    memcpy(trg->objs + trg->count, src->objs, src->count * sizeof *(src->objs));
    ret = src->count;
    trg->count += ret;

    /* cleanup */
    ly_set_free(src, NULL);
    return ret;
}

API LY_ERR
ly_set_rm_index(struct ly_set *set, unsigned int index)
{
    LY_CHECK_ARG_RET(NULL, set, LY_EINVAL);
    LY_CHECK_ERR_RET(((index + 1) > set->count), LOGARG(NULL, set), LY_EINVAL);

    if (index == set->count - 1) {
        /* removing last item in set */
        set->objs[index] = NULL;
    } else {
        /* removing item somewhere in a middle, so put there the last item */
        set->objs[index] = set->objs[set->count - 1];
        set->objs[set->count - 1] = NULL;
    }
    set->count--;

    return LY_SUCCESS;
}

API LY_ERR
ly_set_rm(struct ly_set *set, void *object)
{
    unsigned int i;

    LY_CHECK_ARG_RET(NULL, set, object, LY_EINVAL);

    /* get index */
    for (i = 0; i < set->count; i++) {
        if (set->objs[i] == object) {
            break;
        }
    }
    LY_CHECK_ERR_RET((i == set->count), LOGARG(NULL, set), LY_EINVAL); /* object is not in set */

    return ly_set_rm_index(set, i);
}


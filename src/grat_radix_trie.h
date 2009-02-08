/*
 * Copyright (c) 2009, Elliot Foster (elliot dash source at grat dot net)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * 
 * * Neither the name of Gratuitous, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef _GRAT_RADIX_TRIE_H_
#define _GRAT_RADIX_TRIE_H_ 1

#ifdef __cplusplus
extern "C" {
#endif // #ifdef __cplusplus

//#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// FIXME: make an init function that allocates four bytes to use as an "EMPTY" flag
//        instead of using NULL for everything
// TODO: differentiate between internal 'longest match' and public 'longest match' as the public
//       'longest match' should never return an empty/branch node

// STRING UTIL PROTOTYPES
// FIXME: make this conditional on having/not having strlcat & co
size_t _grat_trie_strlcpy(char *dst, const char *src, size_t siz);
size_t _grat_trie_strlcat(char *dst, const char *src, size_t siz);
char * _grat_trie_strndup(const char *input, size_t max);

#define strlcpy _grat_trie_strlcpy
#define strlcat _grat_trie_strlcat
#define strndup _grat_trie_strndup

// END STRING UTILS

typedef struct radix_node {
    char *key;
    void *val;

    struct radix_node *parent;
    struct radix_node *child;
    struct radix_node *left;
    struct radix_node *right;
} radix_t;

typedef struct {
    radix_t *root_node;
    radix_t *node;
    int     depth;
    int     count;
} radix_iterator_t;

typedef void(*trie_value_callback)(void *value);

// PUBLIC METHOD DEFINITIONS/PROTOTYPES
radix_t * trie_new ();
void trie_destroy (radix_t *trie);
void * trie_set_val (radix_t *root_node, const char *key, void *val );
void * trie_get_val (radix_t *root_node, const char *key);
void * trie_delete_key (radix_t *root_node, const char *key);
void * trie_get_longest_match (radix_t *root_node, const char *key, char **remainder);
int trie_recurse_prefix (radix_t *root_node, const char *prefix, trie_value_callback callback);
int trie_recurse (radix_t *root_node, trie_value_callback callback);

// PRIVATE METHODS

radix_t *
_trie_new_node (const char *key) {
    radix_t *node;

    node = (radix_t *)malloc(sizeof(radix_t));

    // set key, all else to NULL
    node->key = strdup(key);
    node->val = NULL;
    node->parent = NULL;
    node->child = NULL;
    node->left = NULL;
    node->right = NULL;

    return node;
}

inline void
_trie_free_node (radix_t *node) {
    free(node->key);
    free(node);

    return;
}

inline int
_trie_string_cmp (const char *a, const char *b) {
    int bytes = 0;

    while (a[bytes] != '\0' && b[bytes] != '\0' && a[bytes] == b[bytes]) {
        bytes++;
    }

    return bytes;
}

// returns the amount of the input that did not match, returns by reference:
//      node that fully matched,
//      node that partially matched,
//      how much of the partial match node matched
char *
_trie_get_longest_match (
        radix_t *root_node,
        const char *key_input,
        radix_t **full_match_node,
        radix_t **partial_match_node,
        int *len_match
    ) {
    int match_len, i;
    char *path;
    
    path = (char *)key_input;
    match_len = 0;

    // at least the root_node will match
    *partial_match_node = root_node;
    *full_match_node = root_node;

    // start with the root node's child
    radix_t *node = root_node->child;

    while (node != NULL) {
        match_len = _trie_string_cmp(node->key, path);

        if (match_len) {
            *partial_match_node = node; // record the (at least) partial match

            for (i = 0; i < match_len; i++) {
                (void)*path++;
            }

            if (node->key[match_len] == 0) {
                *full_match_node = node; // record the full match(!)

                if (path[0] == 0) {
                    // full match of path, so we can stop now
                    break;
                }
                node = node->child;
            } else {
                // this node matched partially
                break;
            }

        } else {
            if (node->right != NULL && node->right->key[0] > path[0]) {
                node = node->right;
            } else {
                break;
            }
        }
    }

    len_match = &match_len;
    return path;
}

// merge a child with no siblings into its parent
void
_trie_merge_node_with_child(radix_t *node) {
    radix_t *child;
    char *new_key;
    size_t len;

    if (
            // node is not root node
            node->parent != NULL &&
            // node has one child (and only one)
            node->child != NULL && node->child->left == NULL && node->child->right == NULL
    ) {
        // make a new key for the merged node
        len = strlen(node->key) + strlen(node->child->key) + 1;
        new_key = (char *)malloc(len);
        strlcpy(new_key, node->key, len);
        strlcat(new_key, node->child->key, len);

        // set the new key
        free(node->key);
        node->key = new_key;
    
        // take ownership of the grandchildren and free the child
        child = node->child;
        node->child = child->child;
        _trie_free_node(child);
    }

    return;
}

// delete a node and clean up if necessary
void *
_trie_delete_node (radix_t *node) {
    void *val = node->val;

    node->val = NULL;

    // don't delete the root node
    if (node->parent == NULL) {
        return val;
    }

    // merge with its child node if necessary
    if (node->child != NULL) {
        _trie_merge_node_with_child(node);
    } else {
        // we have no children, so just introduce our siblings to one another
        if (node->left != NULL) {
            node->left->right = node->right;
        }
        if (node->right != NULL) {
            node->right->left = node->left;
        }
        node->right = NULL;
        node->left = NULL;

        _trie_free_node(node);
    }

    return val;
}

/*
int
_trie_value_recurse (radix_t *start_node, trie_value_callback callback) {
    int num_found, depth;
    radix_t *node;
    node = start_node;

    num_found = 0;
    depth = 0;

    // deep dive into tree
    while (node != NULL) {

        if (node->val != NULL) {
            num_found++;
            // call the function on the current node
            callback(node->val);
        }

        if (node->child != NULL) {
            // look at children first
            node = node->child;
            depth++;
        } else if (node->right != NULL) {
            // then siblings
            node = node->right;
        } else if (node->parent != NULL) {
            // then try to find an ancestor's sibling
            if (node->parent->right != NULL) {
                node = node->parent->right;
                depth--;
            } else {
                while (node != start_node && node->parent != NULL && node->parent->right == NULL) {
                    // keep going up until we find either the root node or a parent that has a
                    // sibling
                    depth--;
                    node = node->parent;
                }
                // using the sibling of parent
                if (node->right) {
                    node = node->right;
                } else {
                    // we've reached the root node
                    break; 
                }
            }
        }
        if (depth < 0) {
            // make sure we don't go above the root_node
            break;
        }
    }
    return num_found;
}

//_trie_node_recurse (radix_t *root_node, void(*trie_node_callback)(radix_t *node)) {

radix_iterator_t *
_trie_new_iterator (radix_t *root_node) {
    radix_iterator_t *iter = (radix_iterator_t*)malloc(sizeof(radix_iterator_t));

    iter->depth = 0;
    iter->root_node = root_node;
    iter->node = root_node;

    return iter;
}

void
_trie_destroy_iterator (radix_iterator_t *iter) {
    free(iter);
}

radix_t *
_trie_next_node (radix_iterator_t *iter) {
    int num_found, depth;
    radix_t *node = iter->node;

    num_found = depth = 0;

    // begin ugly if/else if/else if/else loop!
    if (iter->node == NULL) {
        return NULL;
    } else if (iter->count++ == 0) {
        // use the starter node the first time
        return iter->node;
    } else if (node->child != NULL) {
        // look at children first
        iter->node = node->child;
        depth++;
    } else if (node->right != NULL) {
        // then siblings
        node = node->right;
    } else if (node->parent != NULL) {
        // then try to find an ancestor's sibling
        if (node->parent->right != NULL) {
            node = node->parent->right;
            depth--;
        } else {
            while (node->parent != NULL && node->parent->right == NULL) {
                // keep going up until we find either the root node or a parent that has a
                // sibling
                depth--;
                node = node->parent;
            }
            // using the sibling of parent
            node = node->right;
        }
    }

    if (node->parent == NULL) {
        iter->node = NULL;
    }

    if (iter->depth < 0) {
        // make sure we don't go above the root_node
        iter->node = NULL;
    }

    return iter->node;
}
*/

// PUBLIC METHOD IMPLEMENTATIONS

// get a new trie root
radix_t *
trie_new () {
    return _trie_new_node("");
}

void
trie_destroy (radix_t *root_node) {
    if (root_node->parent == NULL) {
        // recursively free nodes
    } else {
        // FIXME: EXCEPTION HERE
    }
}

// returns the value that was set
void *
trie_set_val (radix_t *root_node, const char *key, void *val ) {
    // FIXME: if node is being set to NULL, treat as delete(?)
    return NULL;
}

// returns the value associated with the key, or NULL
void *
trie_get_val (radix_t *root_node, const char *key) {
    char *remainder;
    void *val;

    val = trie_get_longest_match(root_node, key, &remainder);
    if (remainder[0] == 0) {
        return val;
    }
    return NULL;
}

// returns the value contained by key after deleting node
void *
trie_delete_key (radix_t *root_node, const char *key) {
    return NULL;
}

// returns the value that was found and key remainder
void *
trie_get_longest_match (radix_t *root_node, const char *key, char **remainder) {
    return NULL;
}

// returns how many nodes matching a given prefix were affected by a functionn pointer calback that
// takes in the node's value.  Should have another function that calls this with no prefix in order
// to apply callback to all nodes
int
trie_recurse_prefix (radix_t *root_node, const char *prefix, trie_value_callback callback) {
    // get the longest partial match for prefix, use prefix as the root node

    return 0;
}

int
trie_recurse (radix_t *root_node, trie_value_callback callback) {
    return 0;
}

// STRING UTIL IMPLEMENTATIONS

/* $OpenBSD: strlcpy.c,v 1.5 2001/05/13 15:40:16 deraadt Exp $ */

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

size_t
_grat_trie_strlcpy(char *dst, const char *src, size_t siz) {
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';	  /* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1); /* count does not include NUL */
}

/*      $OpenBSD: strlcat.c,v 1.13 2005/08/08 08:05:37 espie Exp $        */

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

size_t
_grat_trie_strlcat(char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;
        size_t dlen;

        /* Find the end of dst and adjust bytes left but don't go past end */
        while (n-- != 0 && *d != '\0')
                d++;
        dlen = d - dst;
        n = siz - dlen;

        if (n == 0)
                return(dlen + strlen(s));
        while (*s != '\0') {
                if (n != 1) {
                        *d++ = *s;
                        n--;
                }
                s++;
        }
        *d = '\0';

        return(dlen + (s - src));        /* count does not include NUL */
}

// END STRING UTIL IMPLEMENTATIONS

#ifdef __cplusplus
} // extern "C"

// define C++ class(es) here, or make the C++ header file a different one and include the C header
// file from the C++ header file?

#endif // #ifdef __cplusplus

#endif // #ifndef _GRAT_RADIX_TRIE_H_

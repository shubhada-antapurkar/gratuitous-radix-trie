#include <stdio.h>
#include "../src/grat_radix_trie.h"

/*
 * minimal unit testing, from http://www.jera.com/techinfo/jtns/jtn002.html
 * License: http://www.jera.com/techinfo/jtns/jtn002.html#License
 * License states:
 *      You may use the code in this tech note for any purpose, with the
 *      understanding that it comes with NO WARRANTY.
 */
#define mu_assert(message, test) do { if (!(test)) { fprintf(stdout, \
        "ERROR in %s at line %d (%s): %s\n", __FILE__, __LINE__, __func__, message); \
        return message;} } while (0)
//#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
                                     if (message) return message; } while (0)
extern int tests_run;

/* end unit testing framework */

int tests_run = 0;

radix_t *trie = NULL;
radix_t *trie2 = NULL;

static char *
test_new_trie() {
    trie = trie_new();
    mu_assert("", trie != NULL);
    return 0;
}

static char *
test_set_get_and_delete() {
    char *key1     = "key1";
    char *val1_in  = "val1";
    char *val1_out = NULL;

    trie_set_key(trie, key1, val1_in);

    val1_out = trie_get_key(trie, key1);
    mu_assert("", strcmp(val1_in, val1_out) == 0);
    mu_assert("", strcmp("not_val1", val1_out) != 0);

    val1_out = NULL;
    val1_out = trie_delete_key(trie, key1);
    mu_assert("", strcmp(val1_in, val1_out) == 0);

    val1_out = trie_get_key(trie, key1);
    mu_assert("", val1_out == NULL);

    return 0;
}

static char *
all_tests() {
    mu_run_test(test_new_trie);
    mu_run_test(test_set_get_and_delete);
    return 0;
}

int
main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("SOME TESTS FAILED\n");
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}

/* gcc -g -Wall test.c -o test */
/* g++ -g -Wall test.c -o test */

/*
int
main (int argc, char **argv) {
    radix_t *trie = trie_new();
    char *in_val = strdup("This");
    char *out_val;

    trie_set_key(trie, "This", in_val);
    out_val = trie_get_key(trie, "This");
    printf("Key: 'This' has value '%s'\n", out_val);

    return 0;
}
*/

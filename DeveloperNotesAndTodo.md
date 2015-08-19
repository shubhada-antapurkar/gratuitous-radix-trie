# Development Notes #
What to do, how to do it, problems that might be encountered while writing this library

## interfaces ##
  * set
  * get
    * get longest matching, discard if any remainder in lookup key
  * longest matching
  * delete
  * iterator (reentrant?)
    * what happens when user's current/next node is/are deleted?
    * depth-first (to provide alphabetical order)
  * (not likely) node->key
    * create on demand, client has to free(?)
    * collect array of elements, walking node and its parents
    * get length of combined elements, allocate
    * concatenate elements together

## todo ##
  * tests (once more complete)
  * performance integration
  * single header file (no strl\_string\_utils.[ch](ch.md))

## features ##
  * in-order insertion
    * during comparison, if input[i](i.md) != node->key[i](i.md)
    * find the first node->key[i](i.md) that input[i](i.md) is greater than
    * insert there

## problems ##
  * using NULL everywhere is dumb; how to differentiate between a key that was set with a NULL value and an empty node?
  * how to return a 'key not found' err to the caller?
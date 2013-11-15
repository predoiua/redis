/*
 * Cube operation
 */

#include "redis.h"

/*
 * Call : VVINIT cube max_length
 *
 * It is based on :
SETBIT key offset bitvalue
void setbitCommand(redisClient *c)
 *
 */

void vvinit(redisClient *c) {
	// Prepare a SETBIT command
	// Set new values in redisClient
	robj **argv = c->argv;
	int setbit_argc = 4;
    c->argv = zmalloc(sizeof(robj*)*setbit_argc);

    c->argc = 0;
    c->argv[c->argc] = createStringObject("SETBIT", 6);
    zfree(argv[0]);

    ++c->argc;
    c->argv[c->argc] = argv[1];

    ++c->argc;
    c->argv[c->argc] = argv[2];

    c->argc = 0;
    c->argv[c->argc] = createStringObject("0", 1);

    zfree(argv);

    // Call
    setbitCommand(c);
}

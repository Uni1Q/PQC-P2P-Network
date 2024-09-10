//
// Created by rokas on 02/09/2024.
//

#ifndef ARBITRARILYPRECISEINTEGER_H
#define ARBITRARILYPRECISEINTEGER_H

#include <stdint.h>

struct ArbitrarilyPreciseInteger
{
    uint64_t *value;
    short sign;
    int size;
    char *hexadecimal;
};

struct ArbitrarilyPreciseInteger arbitrarily_precise_integer_constructor(short sign, int size, ...);
void arbitrarily_precise_integer_destructor(struct ArbitrarilyPreciseInteger *bignum);


#endif //ARBITRARILYPRECISEINTEGER_H

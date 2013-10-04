#ifndef SENDTO_H_
#define SENDTO_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

/*
 * Author: jrahm
 * created: 2013/10/04
 * sendto.h: <description>
 */

void init_net_lib( double f1, unsigned int seed );

int sendto_( int i1, void* c1, int i2, int i3, struct sockaddr* sa, int i4 );

#endif /* SENDTO_H_ */

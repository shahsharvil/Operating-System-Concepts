/*
 * rdtsc.h - function definition for reading time stamp
 */

#ifndef _RDTSC_H_
#define _RDTSC_H_

uint64_t tsc(void)   
{
	uint32_t a, d;
	asm volatile("rdtsc" : "=a" (a), "=d" (d));
	return (( (uint64_t)a)|( (uint64_t)d)<<32 );
}

#endif /* _RDTSC_H_ */

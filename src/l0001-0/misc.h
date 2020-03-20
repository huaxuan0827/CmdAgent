#ifndef __L0001_0_MISC_H__
#define __L0001_0_MISC_H__

#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <termios.h>

#define TIMESTAMP_FORMAT				"%Y/%m/%d %H:%M:%S"
#define TIMESTAMPFORMAT_MAX				64

struct l0001_iterative_variance {
	double square_distance_sum;
	double expectation;
	double square_variance;
	int n;
};

void *zmalloc(size_t sz);
int timestamp2str(unsigned long timestamp_s, char* timestamp_str, unsigned long nlen);
int get_exec_dir(char* execdir, size_t len);
int get_exec_fullname(char* fullname, size_t len);
int get_ch(void);
int get_localmac(char* mac);
double stddevi(double *data, int count);
uint32_t crc32(uint32_t crc, const unsigned char *buf, uint32_t size);
uint32_t l0001_second_elapsed(struct timespec *ts_now, struct timespec *ts_before);
uint64_t l0001_millisecond_elapsed(struct timespec *ts_now, struct timespec *ts_before);
uint32_t l0001_millisec_elapsed32(uint32_t ticks_before, uint32_t ticks_after);
uint64_t l0001_timespecelapsed(struct timespec *now, struct timespec *before);
uint64_t l0001_millisecond_diff(struct timespec *ts1, struct timespec *ts2);
int l0001_get_local_ip(const char *eth, char *ip);

#endif

#include "misc.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

double l0001_square_variance_iterative(double data, struct l0001_iterative_variance *itvar, int iterative_max)
{
	if (itvar->n == 0) {
		itvar->expectation = data;
		itvar->square_distance_sum = 0.0f;
		itvar->square_variance = 0.0f;
	}
	else {
		double exp_n_1 = itvar->expectation;
		itvar->expectation = ((itvar->n - 1) * exp_n_1 + data) / itvar->n;
		itvar->square_distance_sum = itvar->square_distance_sum + (data - itvar->expectation) * (data - exp_n_1);
		itvar->square_variance = itvar->square_distance_sum / itvar->n;
	}
	itvar->n ++;

	if (itvar->n > iterative_max) {
		itvar->n = 0;
	}

	return itvar->square_variance;
}

double l0001_standard_variance_iterative(double data, struct l0001_iterative_variance *itvar, int iterative_max)
{
	double square_variance = l0001_square_variance_iterative(data, itvar, iterative_max);

	return sqrt(square_variance);
}
double l0001_mean_iterative(double data, struct l0001_iterative_variance *itvar, int iterative_max)
{
	if (itvar->n == 0) {
		itvar->expectation = data;
	}
	else {
		double exp_n_1 = itvar->expectation;
		itvar->expectation = ((itvar->n - 1) * exp_n_1 + data) / itvar->n;
	}
	itvar->n ++;

	if (itvar->n > iterative_max) {
		itvar->n = 0;
	}
	
	return itvar->expectation;
}

double stddevi(double *data, int count)
{
	int i;
	double sum;
	double avg; 

	sum = 0.0f;
	for (i = 0;i < count;i ++) {
		sum += data[i];
	}
	avg = sum / count;
	sum = 0.0f;
	for (i = 0;i < count;i ++) {
		double diff = data[i] - avg;
		sum += diff * diff;
	}

	return sqrt(sum / count);
}

int timestamp2str(unsigned long timestamp_s, char* timestamp_str, unsigned long nlen)
{
	struct tm tm;
	time_t t = (time_t)timestamp_s;

	memset(timestamp_str, 0 ,nlen);
	if (localtime_r(&t, &tm) != NULL)
	{
		strftime(timestamp_str,nlen - 1,TIMESTAMP_FORMAT,&tm);
		return 0;
	}

	return -1;
}

void *zmalloc(size_t sz)
{
	unsigned char *m = malloc(sz);
	if(m)
		memset(m,0,sz);

	return (void*)m;
}

int get_exec_fullname(char* fullname, size_t len)
{
	memset(fullname, 0, len);
	if(readlink("/proc/self/exe", fullname, len) <= 0)
		return -1;
	return 0;
}

int get_exec_dir(char* execdir, size_t len)
{
	char* path_end;
	memset(execdir, 0, len);
	if(readlink("/proc/self/exe", execdir, len) <= 0)
		return -1;
	path_end = strrchr(execdir,  '/');
	if(path_end == NULL)
		return -1;
	path_end ++;
	*path_end = 0;
	return 0;
}

int get_ch(void)
{
	struct termios tm, tm_old;
	int fd = STDIN_FILENO,c;
	if (tcgetattr(fd, &tm) < 0) {
		return -1;
	}

	tm_old = tm;
	cfmakeraw(&tm);

	if (tcsetattr(fd, TCSANOW, &tm) < 0) {
		return -1;
	}

	c = fgetc(stdin);

	if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {
		return -1;
	}

	return c;
}

int get_localmac(char* mac)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char buf[1024];
	int retval = -1;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		/* handle error*/ 
	};

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
		/* handle error */ 
	}

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	for (; it != end; ++it) {
		strcpy(ifr.ifr_name, it->ifr_name);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
			if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
					if (strncmp(ifr.ifr_name, "wlan0", 5) == 0) {
						/* not a wlan0 interface */
						sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",  
							(unsigned char)ifr.ifr_hwaddr.sa_data[0],	
							(unsigned char)ifr.ifr_hwaddr.sa_data[1],	
							(unsigned char)ifr.ifr_hwaddr.sa_data[2],	
							(unsigned char)ifr.ifr_hwaddr.sa_data[3],	
							(unsigned char)ifr.ifr_hwaddr.sa_data[4],	
							(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
							retval = 0;
					}
				}
			}
		}
	}
	return retval;
}

int l0001_get_local_ip(const char *eth, char *ip)
{
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd)
    {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, eth, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    // if error: No such device
    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0)
    {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    snprintf(ip, 16, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;

}


uint32_t l0001_second_elapsed(struct timespec *ts_now, struct timespec *ts_before)
{
	uint64_t sec = ts_now->tv_sec - ts_before->tv_sec;
	uint64_t usec;

	if (ts_now->tv_nsec > ts_before->tv_nsec) {
		usec = sec * 1000000 + ts_now->tv_nsec / 1000 - ts_before->tv_nsec / 1000;
	}
	else {
		usec = (sec - 1) * 1000000 + (1000000 + ts_now->tv_nsec / 1000) - ts_before->tv_nsec / 1000;
	}

	return (uint32_t)(usec / 1000000);
}

uint64_t l0001_millisecond_elapsed(struct timespec *ts_now, struct timespec *ts_before)
{
	uint32_t sec = ts_now->tv_sec - ts_before->tv_sec;
	uint64_t usec;
	
	if (ts_now->tv_nsec > ts_before->tv_nsec) {
		usec = sec * 1000000 + ts_now->tv_nsec / 1000 - ts_before->tv_nsec / 1000;
	}
	else {
		usec = (sec - 1) * 1000000 + (1000000 + ts_now->tv_nsec / 1000) - ts_before->tv_nsec / 1000;
	}

	return (uint32_t)(usec / 1000);
}

uint64_t l0001_millisecond_diff(struct timespec *ts1, struct timespec *ts2)
{
	uint64_t sec = abs(ts1->tv_sec - ts2->tv_sec);
	uint64_t msec = 0;

	if (ts1->tv_sec > ts2->tv_sec) {		
		msec = (sec - 1) * 1000 + (1000 + ts1->tv_nsec / 1000000) - ts2->tv_nsec / 1000000;
	}
	else if (ts1->tv_sec == ts2->tv_sec){
		msec = abs(ts1->tv_nsec - ts2->tv_nsec)/ 1000000;
	}
	else {
		msec = (sec - 1) * 1000 + (1000 + ts2->tv_nsec / 1000000) - ts1->tv_nsec / 1000000;
	}

	return (uint32_t)(sec * 1000 + msec);
}


uint32_t l0001_millisec_elapsed32(uint32_t ticks_before, uint32_t ticks_after)
{
	if (ticks_after >= ticks_before) {
		return (ticks_after - ticks_before) * 1;
	}

	return ((0xFFFFFFFF - ticks_before) + ticks_after) * 1;
}

uint64_t l0001_timespecelapsed(struct timespec *now, struct timespec *before)
{
	uint64_t to = (now->tv_sec - before->tv_sec) * 1000 + (now->tv_nsec - before->tv_nsec) / 1000000UL;
	return to;
}



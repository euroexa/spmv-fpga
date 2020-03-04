#include "util.h"

double getTimestamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec + tv.tv_sec*1e6;
}
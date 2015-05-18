#include "utils.h"


long get_timestamp(){
    static struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec*1000000 + time.tv_usec;
}

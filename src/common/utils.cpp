#include "utils.h"

#include "boost/date_time/posix_time/posix_time.hpp"
namespace ptime = boost::posix_time;

long get_timestamp(){
	ptime::ptime epoch(boost::gregorian::date(1970, 1, 1));
	ptime::ptime local = ptime::microsec_clock::local_time();
	ptime::time_duration diff = local - epoch;
	return diff.total_microseconds();
}

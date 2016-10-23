/*
 *  Copyright 2016 Bjango Pty Ltd. All rights reserved.
 *  Copyright 2010 William Tisäter. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    1.  Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *    2.  Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 *    3.  The name of the copyright holder may not be used to endorse or promote
 *        products derived from this software without specific prior written
 *        permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL WILLIAM TISÄTER BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "StatsUptime.h"

using namespace std;

#if defined(USE_UPTIME_HPUX)

long StatsUptime::getUptime()
{
	struct pst_static pstat_static;
	if (pstat_getstatic(&pstat_static, sizeof(pstat_static), 1, 0) == -1) {
		return;
	}
	return time(NULL) - pstat_static.boot_time;
}

#elif defined(HAVE_LIBKSTAT) && defined(USE_UPTIME_KSTAT)

long StatsUptime::getUptime()
{
	kstat_t *ksp;
	kstat_named_t *kn;
	static time_t boottime;
	
	if(0 == boottime)
	{
		kstat_chain_update(ksh);
		if(NULL == (ksp = kstat_lookup(ksh, (char*)"unix", -1, (char*)"system_misc"))) return -1;
		if(-1 == kstat_read(ksh, ksp, NULL)) return -1;
		if(NULL == (kn = (kstat_named_t *) kstat_data_lookup(ksp, (char*)"boot_time"))) return -1;
		boottime = (time_t) ksgetull(kn);
	}
	return time(NULL) - boottime;
}

/*USE_UPTIME_KSTAT*/
# elif defined(USE_UPTIME_PROCFS)

long StatsUptime::getUptime()
{
	long uptime;
	FILE * fp = NULL;

	if (!(fp = fopen("/proc/uptime", "r"))) return -1;

	if(1 != fscanf(fp, "%ld", &uptime))
	{
		return -1;
	}

	fclose(fp);

	return uptime;
}

#elif defined(USE_UPTIME_PERFSTAT)

long StatsUptime::getUptime()
{
	perfstat_cpu_total_t cpustats;
	if (perfstat_cpu_total(NULL, &cpustats, sizeof cpustats, 1)!=1)
		return 0;

	int hertz = sysconf(_SC_CLK_TCK);
	if (hertz <= 0)
		hertz = HZ;

	return cpustats.lbolt / hertz;
}/*USE_UPTIME_PERFSTAT*/

#elif defined(USE_UPTIME_GETTIME)

long StatsUptime::getUptime()
{
	struct timespec tp;

	tp.tv_sec = 0;
    tp.tv_nsec = 0;

    clock_gettime(CLOCK_UPTIME, &tp);

    return (long)(tp.tv_sec);
}/*USE_UPTIME_GETTIME*/

#elif defined(USE_UPTIME_SYSCTL)

long StatsUptime::getUptime()
{
	struct timeval tm;
	time_t now;
	int mib[2];
	size_t size;
	
	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	size = sizeof(tm);
	now = time(NULL);
	if(-1 != sysctl(mib, 2, &tm, &size, NULL, 0) && 0 != tm.tv_sec)
	{
		return (long)(now - tm.tv_sec);
	}
	return 0;
}
#else

long StatsUptime::getUptime()
{
	return 0;
}

#endif

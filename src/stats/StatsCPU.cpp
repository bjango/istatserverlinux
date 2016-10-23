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

#include "StatsCPU.h"

using namespace std;

#if defined(USE_CPU_HPUX)

void StatsCPU::init()
{
	_init();

	struct pst_dynamic pstat_dynamic;
	if (pstat_getdynamic(&pstat_dynamic, sizeof(pstat_dynamic), 1, 0) == -1) {
		return;
	}

	int i;
	for (i = 0; i < PST_MAX_CPUSTATES; i++) {
		last_ticks[i] = (unsigned long long)pstat_dynamic.psd_cpu_time[i];
	}
}

void StatsCPU::update(long long sampleID)
{
	struct pst_dynamic pstat_dynamic;
	if (pstat_getdynamic(&pstat_dynamic, sizeof(pstat_dynamic), 1, 0) == -1) {
		return;
	}

	unsigned long long user = (unsigned long long)pstat_dynamic.psd_cpu_time[CP_USER];
	unsigned long long sys = (unsigned long long)pstat_dynamic.psd_cpu_time[CP_SSYS] + (unsigned long long)pstat_dynamic.psd_cpu_time[CP_SYS];
	unsigned long long nice = (unsigned long long)pstat_dynamic.psd_cpu_time[CP_NICE];
	unsigned long long idle = (unsigned long long)pstat_dynamic.psd_cpu_time[CP_IDLE];
	unsigned long long wait = (unsigned long long)pstat_dynamic.psd_cpu_time[CP_WAIT];

	unsigned long long total = 0;
	int i;
	for (i = 0; i < PST_MAX_CPUSTATES; i++) {
		total += (unsigned long long)pstat_dynamic.psd_cpu_time[i] - last_ticks[i];
	}

	processSample(sampleID, user, nice, sys, idle, wait, total);

	for (i = 0; i < PST_MAX_CPUSTATES; i++) {
		last_ticks[i] = (unsigned long long)pstat_dynamic.psd_cpu_time[i];
	}
}

#elif defined(USE_CPU_SYSCTL)

void StatsCPU::init()
{
	_init();

	int mib[2] = {CTL_HW, HW_NCPU};
	size_t size = sizeof(cpu_count);
	if(sysctl(mib, 2, &cpu_count, &size, NULL, 0) < 0)
		cpu_count = 1;
}

void StatsCPU::update(long long sampleID)
{
	#if defined(__OpenBSD__) || defined(__OPENBSD)
	unsigned long long cp_time[CPUSTATES];
	#else
	long cp_time[CPUSTATES];
	#endif

	int x;
	for(x=0;x<CPUSTATES;x++)
		cp_time[x] = 0;

	if(cpu_count == 1)
	{
		int mib[2] = { CTL_KERN, KERN_CPTIME };
		size_t len = sizeof(cp_time);

		if (sysctl(mib, 2, &cp_time, &len, NULL, 0) < 0) {
			return;
		}
	}
	else 
	{
		int x;
		for(x=0;x<cpu_count;x++)
		{
			#if defined(__OpenBSD__) || defined(__OPENBSD)
			unsigned long long cp_time2[CPUSTATES];
			#else
			long cp_time2[CPUSTATES];
			#endif

			int x;
			for(x=0;x<CPUSTATES;x++)
				cp_time2[x] = 0;

			int mib[3] = {CTL_KERN, KERN_CPTIME2, x};
			size_t len = sizeof(cp_time2);
			if(sysctl(mib, 3, &cp_time2, &len, NULL, 0) < 0)
				continue;
		
			for(x=0;x<CPUSTATES;x++)
				cp_time[x] += cp_time2[x];
		}
	}

	processSample(sampleID, cp_time[CP_USER], cp_time[CP_NICE], cp_time[CP_SYS], cp_time[CP_IDLE], 0, 0);
} /*USE_CPU_SYSCTL*/

#elif defined(USE_CPU_SYSCTL_BYNAME)

void StatsCPU::init()
{
	_init();
}

void StatsCPU::update(long long sampleID)
{
	size_t len;
#if defined(__NetBSD__) || defined(__NetBSD)
    u_int64_t cp_time[CPUSTATES];
#elif defined(__DragonFly__)
    unsigned long long cp_time[CPUSTATES];
#else
    long cp_time[CPUSTATES];
#endif

	len = sizeof(cp_time);

    if (sysctlbyname("kern.cp_time", cp_time, &len, NULL, 0) < 0)
	{
		return;
    }

	processSample(sampleID, cp_time[CP_USER], cp_time[CP_NICE], cp_time[CP_SYS], cp_time[CP_IDLE], 0, 0);
} /*USE_CPU_SYSCTL_BYNAME*/

#elif defined(HAVE_LIBKSTAT) && defined(USE_CPU_KSTAT)

void StatsCPU::init()
{
	_init();
}

void StatsCPU::update(long long sampleID)
{
	unsigned long long user = 0;
	unsigned long long kernel = 0;
	unsigned long long nice = 0;
	unsigned long long idle = 0;
	unsigned long long wait = 0;

	kstat_t *ksp;
	static int ncpu;
	int c;
	cpu_stat_t cs;
	
	ncpu = sysconf(_SC_NPROCESSORS_CONF);
	kstat_chain_update(ksh);
	for(c = 0; c < ncpu; c++)
	{
		if(p_online(c, P_STATUS) != P_ONLINE)
		{
			continue;
		}
		if(NULL == (ksp = kstat_lookup(ksh, (char*)"cpu_stat", c, NULL))) return;
		if(-1 == kstat_read(ksh, ksp, &cs)) return;

		user += cs.cpu_sysinfo.cpu[CPU_USER];
		kernel += cs.cpu_sysinfo.cpu[CPU_KERNEL];
		idle += cs.cpu_sysinfo.cpu[CPU_IDLE];
		wait += cs.cpu_sysinfo.cpu[CPU_WAIT];
	}

	processSample(sampleID, user, nice, kernel, idle, wait, 0);
}/*USE_CPU_KSTAT*/
# elif defined(USE_CPU_PROCFS)

void StatsCPU::init()
{
	_init();

	hasIOWait = false;

	char buf[320];
	FILE * fp = NULL;
	
	if (!(fp = fopen("/proc/stat", "r"))) return;

	if (!fgets(buf, sizeof(buf), fp))
	{
		fclose(fp);
		return;
	}

	unsigned long long iowait;
	if(sscanf(buf, "cpu %*d %*d %*d %*d %llu", &iowait) == 1)
	{
		hasIOWait = true;
	}
	fclose(fp);
}

void StatsCPU::update(long long sampleID)
{
	unsigned long long user = 0;
	unsigned long long kernel = 0;
	unsigned long long nice = 0;
	unsigned long long idle = 0;
	unsigned long long wait = 0;

	char buf[320];
	FILE * fp = NULL;
	
	if (!(fp = fopen("/proc/stat", "r"))) return;
	
	if (!fgets(buf, sizeof(buf), fp)){
		fclose(fp);
		return;	
	}
	
	if(hasIOWait)
		sscanf(buf, "cpu %llu %llu %llu %llu %llu", &user, &nice, &kernel, &idle, &wait);
	else
		sscanf(buf, "cpu %llu %llu %llu %llu", &user, &nice, &kernel, &idle);
	
	processSample(sampleID, user, nice, kernel, idle, wait, 0);

	fclose(fp);
}
#elif defined(HAVE_LIBPERFSTAT) && defined(USE_CPU_PERFSTAT)

void StatsCPU::init()
{
	_init();
	hasLpar = false;
    aixEntitlement = 1;

    perfstat_partition_total_t lparstats;
    if(!perfstat_partition_total(NULL, &lparstats, sizeof(perfstat_partition_total_t), 1))
    	return;

    if (lparstats.type.b.shared_enabled)
    {
    	hasLpar = true;
    	aixEntitlement = lparstats.entitled_proc_capacity / 100.0;
    }
}

void StatsCPU::update(long long sampleID)
{
	perfstat_cpu_total_t cpustats;
    perfstat_partition_total_t lparstats;

    if(!perfstat_partition_total(NULL, &lparstats, sizeof(perfstat_partition_total_t), 1))
    {
    	return;
    }  

	if(!perfstat_cpu_total(NULL, &cpustats, sizeof cpustats, 1))
	{
		return;
	}

	if(last_ticks_lpar[0] == 0)
	{
		last_ticks_lpar[0] = lparstats.puser;
		last_ticks_lpar[1] = 0;
		last_ticks_lpar[2] = lparstats.psys;
		last_ticks_lpar[3] = lparstats.pidle;
		last_ticks_lpar[4] = lparstats.pwait;
		last_time = lparstats.timebase_last;
	}

	if(last_ticks[0] == 0)
	{
		last_ticks[0] = cpustats.user;
		last_ticks[1] = 0;
		last_ticks[2] = cpustats.sys;
		last_ticks[3] = cpustats.idle;
		last_ticks[4] = cpustats.wait;
	}

    unsigned long long dlt_pcpu_user  = lparstats.puser - last_ticks_lpar[0]; 
    unsigned long long dlt_pcpu_sys   = lparstats.psys  - last_ticks_lpar[2];
    unsigned long long dlt_pcpu_idle  = lparstats.pidle - last_ticks_lpar[3];
    unsigned long long dlt_pcpu_wait  = lparstats.pwait - last_ticks_lpar[4];

    unsigned long long dlt_lcpu_user  = cpustats.user - last_ticks[0]; 
    unsigned long long dlt_lcpu_sys   = cpustats.sys  - last_ticks[2];
    unsigned long long dlt_lcpu_idle  = cpustats.idle - last_ticks[3];
    unsigned long long dlt_lcpu_wait  = cpustats.wait - last_ticks[4];

    unsigned long long delta_purr, pcputime, entitled_purr, unused_purr, delta_time_base;

    double entitlement = (double)lparstats.entitled_proc_capacity / 100.0;
    delta_time_base = lparstats.timebase_last - last_time;

    delta_purr = pcputime = dlt_pcpu_user + dlt_pcpu_sys + dlt_pcpu_idle + dlt_pcpu_wait;

    if (lparstats.type.b.shared_enabled)
    {
        entitled_purr = delta_time_base * entitlement;
        if (entitled_purr < delta_purr) {
            /* when above entitlement, use consumption in percentages */
            entitled_purr = delta_purr;
        }
        unused_purr = entitled_purr - delta_purr;
       
        /* distribute unused purr in wait and idle proportionally to logical wait and idle */
        dlt_pcpu_wait += unused_purr * ((double)dlt_lcpu_wait / (double)(dlt_lcpu_wait + dlt_lcpu_idle));
        dlt_pcpu_idle += unused_purr * ((double)dlt_lcpu_idle / (double)(dlt_lcpu_wait + dlt_lcpu_idle));
       
        pcputime = entitled_purr;
    }

    double phys_proc_consumed = 0;
    double percent_ent = 0;
    if (lparstats.type.b.shared_enabled)
    {   
        /* Physical Processor Consumed */  
        phys_proc_consumed = (double)delta_purr / (double)delta_time_base;

        /* Percentage of Entitlement Consumed */
        percent_ent = (double)((phys_proc_consumed / entitlement) * 100);
	}

//	ticks = (double)(dlt_lcpu_user + dlt_lcpu_sys + dlt_lcpu_idle + dlt_lcpu_wait);

	last_ticks_lpar[0] = lparstats.puser;
	last_ticks_lpar[1] = 0;
	last_ticks_lpar[2] = lparstats.psys;
	last_ticks_lpar[3] = lparstats.pidle;
	last_ticks_lpar[4] = lparstats.pwait;

	last_ticks[0] = cpustats.user;
	last_ticks[1] = 0;
	last_ticks[2] = cpustats.sys;
	last_ticks[3] = cpustats.idle;
	last_ticks[4] = cpustats.wait;

	last_time = lparstats.timebase_last;

	cpu_data _cpu;
	_cpu.u = (double)dlt_pcpu_user * 100.0 / (double)pcputime;
	_cpu.n = 0;
	_cpu.s = (double)dlt_pcpu_sys  * 100.0 / (double)pcputime;
	_cpu.i = (double)dlt_pcpu_wait * 100.0 / (double)pcputime;
	_cpu.io = (double)dlt_pcpu_wait * 100.0 / (double)pcputime;
	_cpu.ent = percent_ent;
	_cpu.phys = phys_proc_consumed;

	_cpu.sampleID = sampleIndex[0].sampleID;
	_cpu.time = sampleIndex[0].time;


	samples[0].push_front(_cpu);	
	if (samples[0].size() > HISTORY_SIZE) samples[0].pop_back();
}/*USE_CPU_PERFSTAT*/

#else

void StatsCPU::init()
{
	_init();
}

void StatsCPU::update(long long sampleID)
{
}

#endif

void StatsCPU::_init()
{	
	initShared();

	last_ticks[0] = 0; last_ticks[1] = 0; last_ticks[2] = 0; last_ticks[3] = 0; last_ticks[4] = 0;

	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		databaseType = 0;
		databasePrefix = "cpu_";

		int x;
		for(x=1;x<8;x++)
		{
			string table = databasePrefix + tableAtIndex(x);
			if(!_database.tableExists(table))
			{
				string sql = "create table " + table + " (sample double PRIMARY KEY NOT NULL, time double NOT NULL DEFAULT 0, empty integer NOT NULL DEFAULT 0, user double NOT NULL DEFAULT 0, system double NOT NULL DEFAULT 0, nice double NOT NULL DEFAULT 0, wait double NOT NULL DEFAULT 0)";		
				DatabaseItem dbItem = _database.databaseItem(sql);
				dbItem.executeUpdate();
			}
		}
		loadPreviousSamples();
		fillGaps();
	}
	#endif
}

#ifdef USE_SQLITE
void StatsCPU::loadPreviousSamples()
{
	loadPreviousSamplesAtIndex(1);
	loadPreviousSamplesAtIndex(2);
	loadPreviousSamplesAtIndex(3);
	loadPreviousSamplesAtIndex(4);
	loadPreviousSamplesAtIndex(5);
	loadPreviousSamplesAtIndex(6);
	loadPreviousSamplesAtIndex(7);
}

void StatsCPU::loadPreviousSamplesAtIndex(int index)
{
	string table = databasePrefix + tableAtIndex(index);
	double sampleID = sampleIdForTable(table);

	string sql = "select * from " + table + " where sample >= @sample order by sample asc limit 602";
	DatabaseItem query = _database.databaseItem(sql);
	sqlite3_bind_double(query._statement, 1, sampleID - 602);

	while(query.next())
	{
		cpu_data sample;
		sample.u = query.doubleForColumn("user");
		sample.s = query.doubleForColumn("system");
		sample.n = query.doubleForColumn("nice");
		sample.io = query.doubleForColumn("wait");
		sample.sampleID = (long long)query.doubleForColumn("sample");
		sample.time = query.doubleForColumn("time");
		samples[index].insert(samples[index].begin(), sample);
	}
	if(samples[index].size() > 0)
	{
		sampleIndex[index].sampleID = samples[index][0].sampleID;
		sampleIndex[index].time = samples[index][0].time;
		sampleIndex[index].nextTime = sampleIndex[index].time + sampleIndex[index].interval;
	}
}
#endif

void StatsCPU::processSample(long long sampleID, unsigned long long user, unsigned long long nice, unsigned long long kernel, unsigned long long idle, unsigned long long wait, unsigned long long total)
{
	if(total > 0)
		ticks = total;
	else
	{
		if(last_ticks[0] == 0)
		{
			last_ticks[0] = user;
			last_ticks[1] = nice;
			last_ticks[2] = kernel;
			last_ticks[3] = idle;
			last_ticks[4] = wait;
		}
		ticks = (double)(user + kernel + nice + idle) - (last_ticks[0] + last_ticks[1] + last_ticks[2] + last_ticks[3]);
	}

	cpu_data _cpu;

	if(ticks > 0)
	{
		_cpu.u = ((double)(user - last_ticks[0]) / ticks) * 100;
		_cpu.n = ((double)(nice - last_ticks[1]) / ticks) * 100;
		_cpu.s = ((double)(kernel - last_ticks[2]) / ticks) * 100;
		_cpu.i = ((double)(idle - last_ticks[3]) / ticks) * 100;
		_cpu.io = ((double)(wait - last_ticks[4]) / ticks) * 100;
	}
	else
	{
		_cpu.u = 0;
		_cpu.n = 0;
		_cpu.s = 0;
		_cpu.i = 0;
		_cpu.io = 0;	
	}

	_cpu.ent = -1;
	_cpu.phys = -1;

	_cpu.sampleID = sampleIndex[0].sampleID;
	_cpu.time = sampleIndex[0].time;

	if(total == 0)
	{
		last_ticks[0] = user;
		last_ticks[1] = nice;
		last_ticks[2] = kernel;
		last_ticks[3] = idle;
		last_ticks[4] = wait;
	}

	samples[0].push_front(_cpu);	
	if (samples[0].size() > HISTORY_SIZE) samples[0].pop_back();
}

#ifdef USE_SQLITE
void StatsCPU::updateHistory()
{
	int x;
	for (x = 1; x < 8; x++)
	{
		if(sampleIndex[0].time >= sampleIndex[x].nextTime)
		{
			double now = get_current_time();
			double earlistTime = now - (HISTORY_SIZE * sampleIndex[x].interval);
			while(sampleIndex[x].nextTime < now)
			{
				sampleIndex[x].sampleID = sampleIndex[x].sampleID + 1;
				sampleIndex[x].time = sampleIndex[x].nextTime;
				sampleIndex[x].nextTime = sampleIndex[x].nextTime + sampleIndex[x].interval;

				if(sampleIndex[x].time < earlistTime)
					continue;

				cpu_data sample = historyItemAtIndex(x);
				samples[x].push_front(sample);	
				if (samples[x].size() > HISTORY_SIZE) samples[x].pop_back();

				if(sample.empty)
					continue;

				string table = databasePrefix + tableAtIndex(x);
				string sql = "insert into " + table + " (empty, sample, time, user, system, nice, wait) values(?, ?, ?, ?, ?, ?, ?)";

				DatabaseItem dbItem = _database.databaseItem(sql);
				sqlite3_bind_int(dbItem._statement, 1, 0);
				sqlite3_bind_double(dbItem._statement, 2, (double)sample.sampleID);
				sqlite3_bind_double(dbItem._statement, 3, sample.time);
				sqlite3_bind_double(dbItem._statement, 4, sample.u);
				sqlite3_bind_double(dbItem._statement, 5, sample.s);
				sqlite3_bind_double(dbItem._statement, 6, sample.n);
				sqlite3_bind_double(dbItem._statement, 7, sample.io);
				databaseQueue.push_back(dbItem);	
			}
		}
	}
}

cpu_data StatsCPU::historyItemAtIndex(int index)
{
	std::deque<cpu_data> from = samples[sampleIndex[index].historyIndex];
	double minimumTime = sampleIndex[index].time - sampleIndex[index].interval;
	double maximumTime = sampleIndex[index].time;
	if(sampleIndex[index].historyIndex == 0)
		maximumTime += 0.99;

	double cpuUser = 0;
	double cpuSystem = 0;
	double cpuNice = 0;
	double cpuIO = 0;
	cpu_data sample;

	int count = 0;
	if(from.size() > 0)
	{
		for (deque<cpu_data>::iterator cur = from.begin(); cur != from.end(); ++cur)
		{
			if ((*cur).time > maximumTime)
				continue;

			if ((*cur).time < minimumTime)
				break;

			cpuUser += (*cur).u;
			cpuSystem += (*cur).s;
			cpuNice += (*cur).n;
			cpuIO += (*cur).io;
			count++;
		}
		if (count > 0)
		{
			cpuUser /= count;
			cpuSystem /= count;
			cpuNice /= count;
			cpuIO /= count;
		}
	}

	sample.u = cpuUser;
	sample.s = cpuSystem;
	sample.n = cpuNice;
	sample.io = cpuIO;

	sample.sampleID = sampleIndex[index].sampleID;
	sample.time = sampleIndex[index].time;
	sample.empty = false;
	if(count == 0)
		sample.empty = true;

	return sample;
}
#endif

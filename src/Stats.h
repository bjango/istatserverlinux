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

#ifndef _STATS_H
#define _STATS_H
 
#include "config.h"
#include <math.h>
#include <stdio.h>

#include <vector>
#include <limits.h>

#include "System.h"
#include "stats/StatsCPU.h"
#include "stats/StatsSensors.h"
#include "stats/StatsMemory.h"
#include "stats/StatsLoad.h"
#include "stats/StatsNetwork.h"
#include "stats/StatsDisks.h"
#include "stats/StatsUptime.h"
#include "stats/StatsActivity.h"
#include "stats/StatsBattery.h"
#include "stats/StatsProcesses.h"

#ifdef HAVE_KSTAT_H
# include <kstat.h>
#endif

#ifdef USE_SQLITE
#include "Database.h"
#endif

class Stats
{
	public:
		void start();
		void startStats();
		void update_system_stats();
		void updateNextTimes(double t);
		void close();
		void finalize();

		std::vector<battery_info> get_battery_history(long _pos);
		long uptime();
		long long sampleID;
		pthread_mutex_t lock;

		bool historyEnabled;
		bool debugLogging;

		#ifdef USE_SQLITE
		Database _database;
		void insertDatabaseItems(StatsBase *collector);
		#endif

		StatsCPU cpuStats;
		StatsSensors sensorStats;
		StatsMemory memoryStats;
		StatsLoad loadStats;
		StatsNetwork networkStats;
		StatsDisks diskStats;
		StatsUptime uptimeStats;
		StatsActivity activityStats;
		StatsBattery batteryStats;
		StatsProcesses processStats;

	private:
#ifdef HAVE_LIBKSTAT
		kstat_ctl_t *ksh;
#endif
		pthread_t _thread;
		double updateTime;
		double nextIPAddressTime;
};

#endif

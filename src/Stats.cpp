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

#include <vector>
#include <string.h>
#include <iostream>
#include <syslog.h>
#include <unistd.h>
#include <math.h>
#include "Utility.h"

#include "Stats.h"
#include "System.h"
#include <pthread.h>
#include <sys/time.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

using namespace std;

void* start_stats_thread(void*);
void* start_stats_thread(void*a)
{
	Stats *s = static_cast<Stats*>(a);
	s->startStats();
	return 0;
}

void Stats::finalize()
{
	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		_database.beginTransaction();
		insertDatabaseItems(&cpuStats);
		insertDatabaseItems(&loadStats);
		insertDatabaseItems(&memoryStats);
		insertDatabaseItems(&activityStats);
		insertDatabaseItems(&sensorStats);
		insertDatabaseItems(&networkStats);
		insertDatabaseItems(&diskStats);
		_database.commit();
	}
	#endif
}

void Stats::close()
{
	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		sqlite3_interrupt(_database._db);
	}
	#endif
}

void Stats::startStats()
{
#ifdef HAVE_LIBKSTAT
	if(NULL == (ksh = kstat_open()))
	{
		cout << "unable to open kstat " << strerror(errno) << endl;
		return;
	} else {
		cpuStats.ksh = ksh;
		memoryStats.ksh = ksh;
		loadStats.ksh = ksh;
		networkStats.ksh = ksh;
		uptimeStats.ksh = ksh;
		activityStats.ksh = ksh;
	}
#endif

	#ifdef HAVE_LIBKVM
	kvm_t *kd;
	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, NULL)) != NULL)
	{
		cpuStats.kd = kd;
		loadStats.kd = kd;
		memoryStats.kd = kd;
		activityStats.kd = kd;
		sensorStats.kd = kd;
		networkStats.kd = kd;
		diskStats.kd = kd;
		batteryStats.kd = kd;
		processStats.kd = kd;
	}
	#endif

	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		_database.init();
		_database.verify();
		cpuStats._database = _database;
		loadStats._database = _database;
		memoryStats._database = _database;
		activityStats._database = _database;
		sensorStats._database = _database;
		networkStats._database = _database;
		diskStats._database = _database;
		batteryStats._database = _database;
	}
	cpuStats.historyEnabled = historyEnabled;
	loadStats.historyEnabled = historyEnabled;
	memoryStats.historyEnabled = historyEnabled;
	activityStats.historyEnabled = historyEnabled;
	sensorStats.historyEnabled = historyEnabled;
	networkStats.historyEnabled = historyEnabled;
	diskStats.historyEnabled = historyEnabled;
	batteryStats.historyEnabled = historyEnabled;
	#endif

	cpuStats.debugLogging = debugLogging;
	loadStats.debugLogging = debugLogging;
	memoryStats.debugLogging = debugLogging;
	activityStats.debugLogging = debugLogging;
	sensorStats.debugLogging = debugLogging;
	networkStats.debugLogging = debugLogging;
	diskStats.debugLogging = debugLogging;
	batteryStats.debugLogging = debugLogging;

	if(debugLogging)
		cout << "Initiating cpu" << endl;
	cpuStats.init();

	if(debugLogging)
		cout << "Initiating load" << endl;
	loadStats.init();

	if(debugLogging)
		cout << "Initiating memory" << endl;
	memoryStats.init();

	if(debugLogging)
		cout << "Initiating activity" << endl;
	activityStats.init();

	if(debugLogging)
		cout << "Initiating sensors" << endl;
	sensorStats.init();

	if(debugLogging)
		cout << "Initiating network" << endl;
	networkStats.init();

	if(debugLogging)
		cout << "Initiating disks" << endl;
	diskStats.init();

	if(debugLogging)
		cout << "Initiating battery" << endl;
	batteryStats.init();

	if(debugLogging)
		cout << "Initiating processes" << endl;
	processStats.init();

	if(debugLogging)
		cout << "Init complete" << endl;

	processStats.aixEntitlement = cpuStats.aixEntitlement;

	if(debugLogging)
		cout << "Updating cpu" << endl;
	cpuStats.update(sampleID);

	if(debugLogging)
		cout << "Updating memory" << endl;
	memoryStats.update(sampleID);

	if(debugLogging)
		cout << "Updating load" << endl;
	loadStats.update(sampleID);

	if(debugLogging)
		cout << "Updating sensors" << endl;
	sensorStats.update(sampleID);

	if(debugLogging)
		cout << "Updating network" << endl;
	networkStats.update(sampleID);

	if(debugLogging)
		cout << "Updating disks" << endl;
	diskStats.update(sampleID);

	if(debugLogging)
		cout << "Updating activity" << endl;
	activityStats.update(sampleID);

	if(debugLogging)
		cout << "Updating battery" << endl;
	batteryStats.update(sampleID);

	if(debugLogging)
		cout << "Updating processes" << endl;
	processStats.update(sampleID, 0);

	activityStats.ready = 1;
	sensorStats.ready = 1;
	networkStats.ready = 1;
	diskStats.ready = 1;
	batteryStats.ready = 1;

	if(debugLogging)
		cout << "Updating network addresses" << endl;
	networkStats.updateAddresses();

	if(debugLogging)
		cout << "Initital loading complete" << endl;


	updateTime = get_current_time();
	double next = ceil(updateTime) - updateTime;
	updateTime = ceil(updateTime);
	nextIPAddressTime = 0;//updateTime + 600;
	double nextQueueTime = updateTime + 60;

	updateNextTimes(updateTime);
	usleep(next * 1000000);

	while(1){
		pthread_mutex_lock(&lock);
		update_system_stats();
		if(get_current_time() >= nextIPAddressTime)
		{
			nextIPAddressTime = updateTime + 600;
			networkStats.updateAddresses();
		}
		pthread_mutex_unlock(&lock);

		double now = get_current_time();
		double next = updateTime + 1;
		double interval = next - now;
		if(interval < 0)
		{
			interval = ceil(now) < now;
			next = ceil(now);
		}
		else if(interval > 2)
		{
			double n = next;
			while(n < now)
			{
				cpuStats.tickSample();
				loadStats.tickSample();
				memoryStats.tickSample();
				activityStats.tickSample();
				sensorStats.tickSample();
				networkStats.tickSample();
				diskStats.tickSample();
				batteryStats.tickSample();
				n += 1;
			}
			interval = ceil(now) < now;
			next = ceil(now);
		}

		updateTime = next;
	
		if(get_current_time() >= nextQueueTime)
		{
			#ifdef USE_SQLITE
			if(debugLogging)
				cout << "Running database queue" << endl;
			cpuStats.removeOldSamples();
			loadStats.removeOldSamples();
			memoryStats.removeOldSamples();
			activityStats.removeOldSamples();
			sensorStats.removeOldSamples();
			networkStats.removeOldSamples();
			diskStats.removeOldSamples();

			_database.beginTransaction();
			insertDatabaseItems(&cpuStats);
			insertDatabaseItems(&loadStats);
			insertDatabaseItems(&memoryStats);
			insertDatabaseItems(&activityStats);
			insertDatabaseItems(&sensorStats);
			insertDatabaseItems(&networkStats);
			insertDatabaseItems(&diskStats);
			_database.commit();
			if(debugLogging)
				cout << "Database queue complete" << endl;
			#endif
//			nextQueueTime = now + 60;
			nextQueueTime = now + 300;
		}

		updateNextTimes(updateTime);

		usleep(interval * 1000000);
	}
}

void Stats::start()
{
	updateTime = 0;
	pthread_mutex_init(&lock, NULL);
	pthread_create(&_thread, NULL, start_stats_thread, (void*)this);
}

void Stats::update_system_stats()
{	
#ifdef HAVE_LIBKSTAT
	kstat_chain_update(ksh);
#endif

	sampleID++;

	cpuStats.prepareUpdate();
	loadStats.prepareUpdate();
	memoryStats.prepareUpdate();
	activityStats.prepareUpdate();
	sensorStats.prepareUpdate();
	networkStats.prepareUpdate();

	// not updated
	processStats.prepareUpdate();

	if(debugLogging)
		cout << "Updating cpu" << endl;
	cpuStats.update(sampleID);

	if(debugLogging)
		cout << "Updating load" << endl;
	loadStats.update(sampleID);

	if(debugLogging)
		cout << "Updating memory" << endl;
	memoryStats.update(sampleID);

	if(debugLogging)
		cout << "Updating network" << endl;
	networkStats.update(sampleID);

	if(debugLogging)
		cout << "Updating activity" << endl;
	activityStats.update(sampleID);

	if(debugLogging)
		cout << "Updating processes" << endl;
	processStats.update(sampleID, cpuStats.ticks);

	processStats.finishUpdate();

    if((sampleID % 3) == 0)
    {
    	batteryStats.prepareUpdate();
		diskStats.prepareUpdate();
		sensorStats.prepareUpdate();

		if(debugLogging)
			cout << "Updating battery" << endl;
		batteryStats.update(sampleID);

		if(debugLogging)
			cout << "Updating disks" << endl;
		diskStats.update(sampleID);

		if(debugLogging)
			cout << "Updating sensors" << endl;
		sensorStats.update(sampleID);
	}
	else
	{
		sensorStats.tick();
		diskStats.tick();
		batteryStats.tick();		
	}

	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		if(debugLogging)
			cout << "Updating history" << endl;

		cpuStats.updateHistory();
		loadStats.updateHistory();
		memoryStats.updateHistory();
		activityStats.updateHistory();
		sensorStats.updateHistory();
		networkStats.updateHistory();
		diskStats.updateHistory();
	}
	#endif
	if(debugLogging)
		cout << "Updating complete" << endl;
}

#ifdef USE_SQLITE
void Stats::insertDatabaseItems(StatsBase *collector)
{
	for (vector<DatabaseItem>::iterator cur = collector->databaseQueue.begin(); cur != collector->databaseQueue.end(); ++cur)
	{
		(*cur).executeUpdate();
	}
	collector->databaseQueue.clear();
}
#endif

void Stats::updateNextTimes(double t)
{
	cpuStats.sampleIndex[0].nextTime = t;
	loadStats.sampleIndex[0].nextTime = t;
	memoryStats.sampleIndex[0].nextTime = t;
	activityStats.sampleIndex[0].nextTime = t;
	sensorStats.sampleIndex[0].nextTime = t;
	networkStats.sampleIndex[0].nextTime = t;
	diskStats.sampleIndex[0].nextTime = t;
	batteryStats.sampleIndex[0].nextTime = t;
	/*
	processStats.sampleIndex[0].nextTime = t;
	*/
}

long Stats::uptime()
{
	return uptimeStats.getUptime();
}

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

#include "StatsLoad.h"

using namespace std;

#ifdef USE_LOAD_HPUX

void StatsLoad::update(long long sampleID)
{
	struct pst_dynamic pstat_dynamic;
	if (pstat_getdynamic(&pstat_dynamic, sizeof(pstat_dynamic), 1, 0) == -1) {
		return;
	}

	struct load_data load;
	load.one = pstat_dynamic.psd_avg_1_min;
	load.two = pstat_dynamic.psd_avg_5_min;
	load.three = pstat_dynamic.psd_avg_15_min;

	addSample(load, sampleID);
} /*USE_LOAD_HPUX*/

#elif defined(USE_LOAD_PERFSTAT)

void StatsLoad::update(long long sampleID)
{
	struct load_data load;
	perfstat_cpu_total_t cpustats;
	perfstat_cpu_total(NULL, &cpustats, sizeof cpustats, 1);
	load.one = cpustats.loadavg[0]/(float)(1<<SBITS);
	load.two = cpustats.loadavg[1]/(float)(1<<SBITS);
	load.three = cpustats.loadavg[2]/(float)(1<<SBITS);

	addSample(load, sampleID);
} /*USE_LOAD_PERFSTAT*/

#elif defined(USE_LOAD_GETLOADAVG)

void StatsLoad::update(long long sampleID)
{
	struct load_data load;
	double loadavg[3];
	
	if(-1 == getloadavg(loadavg, 3)) return;
	load.one = (float) loadavg[0];
	load.two = (float) loadavg[1];
	load.three = (float) loadavg[2];

	addSample(load, sampleID);
} /*USE_LOAD_GETLOADAVG*/

#elif defined(USE_LOAD_PROCFS)

void StatsLoad::update(long long sampleID)
{
	struct load_data load;
	static FILE * fp = NULL;

	if (!(fp = fopen("/proc/loadavg", "r"))) return;

	fscanf(fp, "%f %f %f", &load.one, &load.two, &load.three);
	fclose(fp);
	
	addSample(load, sampleID);
} /* USE_LOAD_PROCFS */
#else

void StatsLoad::update(long long sampleID){}

#endif

void StatsLoad::init()
{	
	initShared();

	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		databaseType = 0;
		databasePrefix = "load_";

		int x;
		for(x=1;x<8;x++)
		{
			string table = databasePrefix + tableAtIndex(x);
			if(!_database.tableExists(table))
			{
				string sql = "create table " + table + " (sample double PRIMARY KEY NOT NULL, time double NOT NULL DEFAULT 0, empty integer NOT NULL DEFAULT 0, one double NOT NULL DEFAULT 0, five double NOT NULL DEFAULT 0, fifteen double NOT NULL DEFAULT 0)";		
				DatabaseItem dbItem = _database.databaseItem(sql);
				dbItem.executeUpdate();
			}
		}
		loadPreviousSamples();
		fillGaps();
	}
	#endif
}

void StatsLoad::addSample(load_data data, long long sampleID)
{
	data.sampleID = sampleIndex[0].sampleID;
	data.time = sampleIndex[0].time;

	samples[0].push_front(data);	
	if (samples[0].size() > HISTORY_SIZE) samples[0].pop_back();
}

#ifdef USE_SQLITE
void StatsLoad::loadPreviousSamples()
{
	loadPreviousSamplesAtIndex(1);
	loadPreviousSamplesAtIndex(2);
	loadPreviousSamplesAtIndex(3);
	loadPreviousSamplesAtIndex(4);
	loadPreviousSamplesAtIndex(5);
	loadPreviousSamplesAtIndex(6);
	loadPreviousSamplesAtIndex(7);
}

void StatsLoad::loadPreviousSamplesAtIndex(int index)
{
	string table = databasePrefix + tableAtIndex(index);
	double sampleID = sampleIdForTable(table);

	string sql = "select * from " + table + " where sample >= @sample order by sample asc limit 602";
	DatabaseItem query = _database.databaseItem(sql);
	sqlite3_bind_double(query._statement, 1, sampleID - 602);

	while(query.next())
	{
		load_data sample;
		sample.one = query.doubleForColumn("one");
		sample.two = query.doubleForColumn("five");
		sample.three = query.doubleForColumn("fifteen");
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

void StatsLoad::updateHistory()
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

				load_data sample = historyItemAtIndex(x);

				samples[x].push_front(sample);	
				if (samples[x].size() > HISTORY_SIZE) samples[x].pop_back();

				if(sample.empty)
					continue;

				string table = databasePrefix + tableAtIndex(x);
				string sql = "insert into " + table + " (empty, sample, time, one, five, fifteen) values(?, ?, ?, ?, ?, ?)";

				DatabaseItem dbItem = _database.databaseItem(sql);
				sqlite3_bind_int(dbItem._statement, 1, 0);
				sqlite3_bind_double(dbItem._statement, 2, (double)sample.sampleID);
				sqlite3_bind_double(dbItem._statement, 3, sample.time);
				sqlite3_bind_double(dbItem._statement, 4, sample.one);
				sqlite3_bind_double(dbItem._statement, 5, sample.two);
				sqlite3_bind_double(dbItem._statement, 6, sample.three);
				databaseQueue.push_back(dbItem);
			}
		}
	}
}

load_data StatsLoad::historyItemAtIndex(int index)
{
	std::deque<load_data> from = samples[sampleIndex[index].historyIndex];
	double minimumTime = sampleIndex[index].time - sampleIndex[index].interval;
	double maximumTime = sampleIndex[index].time;
	if(sampleIndex[index].historyIndex == 0)
		maximumTime += 0.99;

	double load1 = 0;
	double load5 = 0;
	double load15 = 0;
	load_data sample;

	int count = 0;
	if(from.size() > 0)
	{
		for (deque<load_data>::iterator cur = from.begin(); cur != from.end(); ++cur)
		{
			if ((*cur).time > maximumTime)
				continue;

			if ((*cur).time < minimumTime)
				break;

			load1 += (*cur).one;
			load5 += (*cur).two;
			load15 += (*cur).three;
			count++;
		}
		if (count > 0)
		{
			load1 /= count;
			load15 /= count;
			load15 /= count;
		}
	}

	sample.one = load1;
	sample.two = load15;
	sample.three = load15;

	sample.sampleID = sampleIndex[index].sampleID;
	sample.time = sampleIndex[index].time;
	sample.empty = false;
	if(count == 0)
		sample.empty = true;

	return sample;
}
#endif

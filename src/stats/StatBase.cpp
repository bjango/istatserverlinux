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

#include "StatBase.h"

using namespace std;

void StatsBase::tickSample()
{
	sampleIndex[0].sampleID = sampleIndex[0].sampleID + 1;
}

void StatsBase::tick()
{
	sampleIndex[0].time = sampleIndex[0].nextTime;
}

void StatsBase::prepareUpdate()
{
	sampleIndex[0].time = sampleIndex[0].nextTime;
	sampleIndex[0].sampleID = sampleIndex[0].sampleID + 1;
}

void StatsBase::initShared()
{
    int x;
    int storageIndexes[9] = {0, 0, 1, 1, 2, 2, 2, 2, 0};
    int intervals[9] = {1, 6, 144, 1008, 4320, 12960, 25920, 52560, 300};
    for(x=0;x<8;x++){
        sampleIndex[x].time = 0;
        sampleIndex[x].nextTime = 0;
        sampleIndex[x].sampleID = 1;
        sampleIndex[x].interval = intervals[x];
        sampleIndex[x].historyIndex = storageIndexes[x];
    }
    ready = 1;
    session = 0;
}

#ifdef USE_SQLITE
string StatsBase::tableAtIndex(int index)
{
	string tables[8] = {"", "hour", "day", "week", "month", "threemonth", "sixmonth", "year" };
	return tables[index];
}

double StatsBase::sampleIdForTable(string table)
{
	string sql = "select sample from " + table + " order by sample desc limit 1";
	DatabaseItem query = _database.databaseItem(sql);

	if(query.next())
	{
		double value = sqlite3_column_double(query._statement, 0);
		query.finalize();
		return value;
	}

	return 0;
}

void StatsBase::fillGaps()
{
	fillGapsAtIndex(1);
	fillGapsAtIndex(2);
	fillGapsAtIndex(3);
	fillGapsAtIndex(4);
	fillGapsAtIndex(5);
	fillGapsAtIndex(6);
	fillGapsAtIndex(7);
}

void StatsBase::fillGapsAtIndex(int index)
{
	double now = get_current_time();

	if (sampleIndex[index].nextTime == 0) {
		sampleIndex[index].nextTime = ceil(now) + sampleIndex[index].interval;
		sampleIndex[index].time = sampleIndex[index].nextTime - sampleIndex[index].interval;

		InsertInitialSample(index, sampleIndex[index].time, sampleIndex[index].sampleID);
		return;
	}

	if (sampleIndex[index].nextTime < now)
	{
		while (sampleIndex[index].nextTime < now)
		{
			sampleIndex[index].nextTime = sampleIndex[index].nextTime + sampleIndex[index].interval;
			sampleIndex[index].sampleID = sampleIndex[index].sampleID + 1;
		}
	}
}

int StatsBase::InsertInitialSample(int index, double t, long long sampleID)
{
	string table = databasePrefix + tableAtIndex(index);
	if (databaseType == 1)
		table += "_id";

	string sql = "insert into " + table + " (sample, time, empty) values(?, ?, 1)";

	DatabaseItem dbItem = _database.databaseItem(sql);
	sqlite3_bind_double(dbItem._statement, 1, (double)sampleID);
	sqlite3_bind_double(dbItem._statement, 2, t);
	dbItem.executeUpdate();

	return 0;
}

void StatsBase::removeOldSamples()
{
	int x;
    for(x=1;x<8;x++)
    {
		if (databaseType == 1)
		{
			string table = databasePrefix + tableAtIndex(x) + "_id";
			string sql = "delete from " + table + " WHERE sample < ?";
			DatabaseItem dbItem = _database.databaseItem(sql);
			sqlite3_bind_double(dbItem._statement, 1, sampleIndex[x].sampleID - 600);
			databaseQueue.push_back(dbItem);
		}
		string table = databasePrefix + tableAtIndex(x);
		string sql = "delete from " + table + " WHERE sample < ?";
		DatabaseItem dbItem = _database.databaseItem(sql);
		sqlite3_bind_double(dbItem._statement, 1, sampleIndex[x].sampleID - 600);
		databaseQueue.push_back(dbItem);
	}
}

#endif

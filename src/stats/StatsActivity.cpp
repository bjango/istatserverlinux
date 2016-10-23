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

#include "StatsActivity.h"

using namespace std;

#if defined(USE_ACTIVITY_HPUX)

void StatsActivity::init()
{
	_init();
}

void StatsActivity::update(long long sampleID)
{
    struct pst_diskinfo pstat_disk;
    int i = 0;

    while (pstat_getdisk(pstat_diskinfo, sizeof pstat_disk, 1, i) != -1) {
		i++;
		if(pstat_disk.psd_status == 0)
		{
			continue;
		}

		processDisk(string(pstat_disk.psd_hw_path.psh_name), sampleID, pstat_disk.psd_dkbyteread, pstat_disk.psd_dkbytewrite, pstat_disk.psd_dkread, pstat_disk.psd_dkwrite);
    }
}

#elif defined(USE_ACTIVITY_HWDS_NET)

#ifdef HW_IOSTATS
#define HW_DISKSTATS HW_IOSTATS
#define disk_sysctl	io_sysctl
#define dk_rbytes rbytes
#define dk_wbytes wbytes
#define dk_name	name
#define dk_rxfer rxfer
#define dk_wxfer wxfer
#endif

void StatsActivity::init()
{
	_init();
}

void StatsActivity::update(long long sampleID)
{
	int mib[3];
	int x;
	int diskn;
	size_t len;
	struct disk_sysctl *ds;

	mib[0] = CTL_HW;
	mib[1] = HW_DISKSTATS;
	mib[2] = sizeof(struct disk_sysctl);
	if (sysctl(mib, 3, NULL, &len, NULL, 0) == -1)
		return;

	diskn = len / sizeof(struct disk_sysctl);
	ds = (struct disk_sysctl *)malloc(len);
	if (sysctl(mib, 3, ds, &len, NULL, 0) == -1)
		return;

	for (x = 0; x < diskn; x++){
		if(!strncmp(ds[x].dk_name, "cd", 2))
			continue;
		if(!strncmp(ds[x].dk_name, "fd", 2))
			continue;
		processDisk(string(ds[x].dk_name), sampleID, ds[x].dk_rbytes, ds[x].dk_wbytes, ds[x].dk_rxfer, ds[x].dk_wxfer);
	}

	free(ds);
}

#elif defined(USE_ACTIVITY_HWDS_OPEN)

void StatsActivity::init()
{
	_init();
}

void StatsActivity::update(long long sampleID)
{
	int mib[3];
	int x;
	int diskn;
	size_t len;
	struct diskstats *ds;

	mib[0] = CTL_HW;
	mib[1] = HW_DISKCOUNT;
	len = sizeof(diskn);

	if (sysctl(mib, 2, &diskn, &len, NULL, 0) < 0)
		return;

	mib[0] = CTL_HW;
	mib[1] = HW_DISKSTATS;
	len = diskn * sizeof(struct diskstats);

	ds = (struct diskstats *)malloc(len);
	if (ds == NULL)
		return;

	if (sysctl(mib, 2, ds, &len, NULL, 0) < 0) {
		free(ds);
		return;
	}

	for (x = 0; x < diskn; x++){
		if(!strncmp(ds[x].ds_name, "cd", 2))
			continue;
		if(!strncmp(ds[x].ds_name, "fd", 2))
			continue;
		processDisk(string(ds[x].ds_name), sampleID, ds[x].ds_rbytes, ds[x].ds_wbytes, ds[x].ds_rxfer, ds[x].ds_wxfer);
	}

	free(ds);
}

#elif defined(USE_ACTIVITY_KSTAT)

void StatsActivity::init()
{
	_init();
}

void StatsActivity::update(long long sampleID)
{
    kstat_t *ksp;
    kstat_io_t kios;

    for (ksp = ksh->kc_chain; ksp; ksp = ksp->ks_next) {
		if (ksp->ks_type != KSTAT_TYPE_IO) 
			continue;
		if (strncmp(ksp->ks_class, "disk", 4) || !strcmp(ksp->ks_module, "fd"))
			continue;

		memset((void *)&kios, 0, sizeof(kstat_io_t));
		kstat_read(ksh, ksp, &kios);

		processDisk(string(ksp->ks_name), sampleID, kios.nread, kios.nwritten, kios.reads, kios.writes);
    }
}

#elif (defined(HAVE_DEVSTAT) || defined(HAVE_DEVSTAT_ALT)) && defined(USE_ACTIVITY_DEVSTAT)

void StatsActivity::init()
{
	_init();
}

void StatsActivity::update(long long sampleID)
{
	int devs_count, num_selected, num_selections, dn;
	struct device_selection *dev_select = NULL;
	long select_generation;
	static struct statinfo statinfo_cur;

	memset(&statinfo_cur, 0, sizeof(statinfo_cur));
	statinfo_cur.dinfo = (struct devinfo *)calloc(1, sizeof(struct devinfo));

#ifdef HAVE_DEVSTAT_ALT
	if (getdevs(&statinfo_cur) < 0) {
#else
	if (devstat_getdevs(NULL, &statinfo_cur) < 0) {
#endif
		free(statinfo_cur.dinfo);
		return;
	}

	devs_count = statinfo_cur.dinfo->numdevs;
#ifdef HAVE_DEVSTAT_ALT
	if (selectdevs(&dev_select, &num_selected, &num_selections,
#else
	if (devstat_selectdevs(&dev_select, &num_selected, &num_selections,
#endif
			&select_generation, statinfo_cur.dinfo->generation,
			statinfo_cur.dinfo->devices, devs_count, NULL, 0, NULL, 0,
			DS_SELECT_ONLY, 16, 1) >= 0) {
		for (dn = 0; dn < devs_count; dn++) {
			int di;
			struct devstat *dev;

			di = dev_select[dn].position;
			dev = &statinfo_cur.dinfo->devices[di];

			if (((dev->device_type & DEVSTAT_TYPE_MASK) == DEVSTAT_TYPE_DIRECT) && ((dev->device_type & DEVSTAT_TYPE_PASS) == 0) && ((dev->device_name[0] != '\0')))
			{
				stringstream n;
				n << "/dev/" << dev->device_name << dev->unit_number;

				#ifdef HAVE_DEVSTAT_ALT
				processDisk(n.str(), sampleID, dev->bytes_read, dev->bytes_written, dev->num_reads, dev->num_writes);
				#else
				processDisk(n.str(), sampleID, dev->bytes[DEVSTAT_READ], dev->bytes[DEVSTAT_WRITE], dev->operations[DEVSTAT_READ], dev->operations[DEVSTAT_WRITE]);
				#endif
			}
		}

		free(dev_select);
	}

	free(statinfo_cur.dinfo);
}

#elif defined(USE_ACTIVITY_PROCFS)

void StatsActivity::init()
{
	_init();
}

void StatsActivity::update(long long sampleID)
{
	FILE * fp = NULL;
	
	if (!(fp = fopen("/proc/diskstats", "r")))
	{
		return;
	}
	
	while (!feof(fp))
	{
		unsigned long long read;
		unsigned long long write;
		unsigned long long reads;
		unsigned long long writes;
		unsigned int major;
		unsigned int minor;
		char dev[32];

		if(fscanf(fp, "%u %u %s %llu %*u %llu %*u %llu %*u %llu %*[^\n]", &major, &minor, dev, &reads, &read, &writes, &write) > 0)
		{
			if(major == 1 || major == 58 || major == 43 || major == 7 || major == 253 || major == 11)
				continue;

			processDisk(string(dev), sampleID, read * 512, write * 512, reads, writes);
		}
		else
		{
	        if(fscanf(fp, "%*[^\n]")){}
		}
	}
	
	fclose(fp);
}

#elif defined(HAVE_LIBPERFSTAT) && defined(USE_ACTIVITY_PERFSTAT)

void StatsActivity::init()
{
	_init();
}

void StatsActivity::update(long long sampleID)
{
	int disks = perfstat_disk(NULL, NULL, sizeof(perfstat_disk_t), 0);
	if(disks < 0)
		return;

	perfstat_disk_t *storage = (perfstat_disk_t *)calloc(disks, sizeof(perfstat_disk_t));

	perfstat_id_t firstpath;
	disks = perfstat_disk(&firstpath, storage, sizeof(perfstat_disk_t), disks); 

	int i;
	for (i = 0; i < disks; i++)
	{
		processDisk(string(storage[i].name), sampleID, storage[i].rblks * storage[i].bsize, storage[i].wblks * storage[i].bsize, storage[i].__rxfers, storage[i].xfers - storage[i].__rxfers);
		for (vector<activity_info>::iterator curdisk = _items.begin(); curdisk != _items.end(); ++curdisk)
		{
			if(string(storage[i].name) == (*curdisk).device)
			{
				if((*curdisk).is_new == true)
				{
					(*curdisk).is_new = false;

				  	char path[2048];

				  	stringstream cmd;
				  	cmd << "/usr/sbin/lspv -l ";
				  	cmd << storage[i].name;

				  	FILE *fp = popen(cmd.str().c_str(), "r");
				 	if (fp != NULL) {
				 		stringstream output;
				  		while (fgets(path, sizeof(path)-1, fp) != NULL) {
				  			output << string(path) << endl;
					 	}
						vector<string> lines = explode(output.str(), "\n");

						unsigned int x;
						for(x=0;x<lines.size();x++){
							string line = lines[x];
							vector<string> segments = explode(line, " ");
							if(segments.size() == 5){
								string mount = string(segments[4]);
								if(mount.compare("N/A") != 0)
								{
									(*curdisk).mounts.push_back(mount);
								}
							}
						}
				  	}

				  	pclose(fp);		
			 	}
			}
		}
	}
	free(storage);
}

#else

void StatsActivity::init()
{
	_init();
}

void StatsActivity::update(long long sampleID)
{
}

#endif

void StatsActivity::createDisk(string key)
{
	if(_items.size() > 0)
	{
		for (vector<activity_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			activity_info interface = *cur;
			if (interface.device == key)
			{
				return;
			}
		}
	}

	activity_info item;
	item.last_r = 0;
	item.last_w = 0;
	item.last_rIOPS = 0;
	item.last_wIOPS = 0;
	item.active = false;
	item.is_new = true;

	item.device = key;

#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		int x;
		for(x=1;x<8;x++)
		{
			string table = databasePrefix + tableAtIndex(x);
			double sampleID = 0;
			if(samples[x].size() > 0)
				sampleID = samples[x][0].sampleID;


			string sql = "select * from " + table + " where sample >= @sample AND uuid = ? order by sample asc limit 602";
			DatabaseItem query = _database.databaseItem(sql);
			sqlite3_bind_double(query._statement, 1, sampleID - 602);
			sqlite3_bind_text(query._statement, 2, key.c_str(), -1, SQLITE_STATIC);

			while(query.next())
			{
				activity_data sample;
				sample.r = query.doubleForColumn("read");
				sample.w = query.doubleForColumn("write");
				sample.rIOPS = query.doubleForColumn("readiops");
				sample.wIOPS = query.doubleForColumn("writeiops");
				sample.sampleID = query.doubleForColumn("sample");
				sample.time = query.doubleForColumn("time");
				item.samples[x].push_front(sample);
			}
		}
	}
#endif

	session++;
	_items.insert(_items.begin(), item);	
}

void StatsActivity::prepareUpdate()
{
	if(ready == 0)
		return;

	StatsBase::prepareUpdate();

	if(_items.size() > 0)
	{
		for (vector<activity_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			(*cur).active = false;
		}
	}
}

void StatsActivity::processDisk(string key, long long sampleID, unsigned long long read, unsigned long long write, unsigned long long reads, unsigned long long writes)
{
	if(key == "pass" || key == "cd")
		return;

	createDisk(key);

	for (vector<activity_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
	{
		if (key == (*cur).device)
		{
			(*cur).active = true;

			if(ready == 0)
				continue;

			activity_data data;

			if((*cur).last_r == 0)
				(*cur).last_r = read;
			if((*cur).last_w == 0)
				(*cur).last_w = write;
			if((*cur).last_wIOPS == 0)
				(*cur).last_wIOPS = writes;
			if((*cur).last_rIOPS == 0)
				(*cur).last_rIOPS = reads;

			double w = (double)(write - (*cur).last_w);
			double r = (double)(read - (*cur).last_r);
			double wIOPS = (double)(writes - (*cur).last_wIOPS);
			double rIOPS = (double)(reads - (*cur).last_rIOPS);

			(*cur).last_w = write;
			(*cur).last_r = read;
			(*cur).last_wIOPS = writes;
			(*cur).last_rIOPS = reads;

			data.r = r;
			data.w = w;
			data.rIOPS = rIOPS;
			data.wIOPS = wIOPS;
			data.sampleID = sampleIndex[0].sampleID;
			data.time = sampleIndex[0].time;

			(*cur).samples[0].push_front(data);
			if ((*cur).samples[0].size() > HISTORY_SIZE)
				(*cur).samples[0].pop_back();

			break;
		}
	}
}

void StatsActivity::_init()
{	
	initShared();
	ready = 0;

	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		databaseType = 1;
		databasePrefix = "activity_";

		int x;
		for(x=1;x<8;x++)
		{
			string table = databasePrefix + tableAtIndex(x) + "_id";
			if(!_database.tableExists(table))
			{
				string sql = "create table " + table + " (sample double PRIMARY KEY NOT NULL, time double NOT NULL DEFAULT 0, empty integer NOT NULL DEFAULT 0)";		
				DatabaseItem dbItem = _database.databaseItem(sql);
				dbItem.executeUpdate();
			}

			table = databasePrefix + tableAtIndex(x);
			if(!_database.tableExists(table))
			{
				string sql = "create table " + table + " (sample double NOT NULL, time double NOT NULL DEFAULT 0, uuid varchar(255) NOT NULL, read double NOT NULL DEFAULT 0, write double NOT NULL DEFAULT 0, readiops double NOT NULL DEFAULT 0, writeiops double NOT NULL DEFAULT 0)";		
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
void StatsActivity::loadPreviousSamples()
{
	loadPreviousSamplesAtIndex(1);
	loadPreviousSamplesAtIndex(2);
	loadPreviousSamplesAtIndex(3);
	loadPreviousSamplesAtIndex(4);
	loadPreviousSamplesAtIndex(5);
	loadPreviousSamplesAtIndex(6);
	loadPreviousSamplesAtIndex(7);
}

void StatsActivity::loadPreviousSamplesAtIndex(int index)
{
	
	string table = databasePrefix + tableAtIndex(index) + "_id";
	double sampleID = sampleIdForTable(table);

	string sql = "select * from " + table + " where sample >= @sample order by sample asc limit 602";
	DatabaseItem query = _database.databaseItem(sql);
	sqlite3_bind_double(query._statement, 1, sampleID - 602);

	while(query.next())
	{
		sample_data sample;
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

void StatsActivity::updateHistory()
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

				if(_items.size() > 0)
				{
					for (vector<activity_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
					{
						activity_data sample = historyItemAtIndex(x, (*cur));

						(*cur).samples[x].push_front(sample);	
						if ((*cur).samples[x].size() > HISTORY_SIZE) (*cur).samples[x].pop_back();

						if(sample.empty)
							continue;

						string table = databasePrefix + tableAtIndex(x);
						string sql = "insert into " + table + " (sample, time, read, write, readiops, writeiops, uuid) values(?, ?, ?, ?, ?, ?, ?)";

						DatabaseItem dbItem = _database.databaseItem(sql);
						sqlite3_bind_double(dbItem._statement, 1, (double)sample.sampleID);
						sqlite3_bind_double(dbItem._statement, 2, sample.time);
						sqlite3_bind_double(dbItem._statement, 3, sample.r);
						sqlite3_bind_double(dbItem._statement, 4, sample.w);
						sqlite3_bind_double(dbItem._statement, 5, sample.rIOPS);
						sqlite3_bind_double(dbItem._statement, 6, sample.wIOPS);
						sqlite3_bind_text(dbItem._statement, 7, (*cur).device.c_str(), -1, SQLITE_STATIC);
						databaseQueue.push_back(dbItem);
					}
				}

				string table = databasePrefix + tableAtIndex(x) + "_id";
				string sql = "insert into " + table + " (empty, sample, time) values(?, ?, ?)";

				DatabaseItem dbItem = _database.databaseItem(sql);
				sqlite3_bind_int(dbItem._statement, 1, 0);
				sqlite3_bind_double(dbItem._statement, 2, (double)sampleIndex[x].sampleID);
				sqlite3_bind_double(dbItem._statement, 3, sampleIndex[x].time);
				databaseQueue.push_back(dbItem);
			}
		}
	}
}

activity_data StatsActivity::historyItemAtIndex(int index, activity_info item)
{
	activity_data sample;
	double r = 0;
	double w = 0;
	double rIOPS = 0;
	double wIOPS = 0;

	std::deque<activity_data> from = item.samples[sampleIndex[index].historyIndex];
	double minimumTime = sampleIndex[index].time - sampleIndex[index].interval;
	double maximumTime = sampleIndex[index].time;
	if(sampleIndex[index].historyIndex == 0)
		maximumTime += 0.99;

	int count = 0;
	if(from.size() > 0)
	{
		for (deque<activity_data>::iterator cursample = from.begin(); cursample != from.end(); ++cursample)
		{
			if ((*cursample).time > maximumTime)
				continue;

			if ((*cursample).time < minimumTime)
				break;

			r += (*cursample).r;
			w += (*cursample).w;
			rIOPS += (*cursample).rIOPS;
			wIOPS += (*cursample).wIOPS;
			count++;
		}
		if (count > 0)
		{
			r /= count;
			w /= count;
			rIOPS /= count;
			wIOPS /= count;
		}
	}

	sample.r = r;
	sample.w = w;
	sample.rIOPS = rIOPS;
	sample.wIOPS = wIOPS;

	sample.sampleID = sampleIndex[index].sampleID;
	sample.time = sampleIndex[index].time;
	sample.empty = false;
	if(count == 0)
		sample.empty = true;

	return sample;
}
#endif

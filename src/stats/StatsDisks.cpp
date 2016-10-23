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

#include "StatsDisks.h"

#ifndef _PATH_MOUNTED
#ifdef MNTTAB
#define _PATH_MOUNTED MNTTAB
#else
#define _PATH_MOUNTED "/etc/mtab"
#endif
#endif

using namespace std;

#ifdef USE_DISK_STATFS
int StatsDisks::get_sizes(const char *dev, struct disk_data *data)
{
#ifdef HAVE_STATVFS
	struct statvfs space;
	if (statvfs(dev, &space) == 0)
#else
	struct statfs space;
	if (statfs(dev, &space) == 0)
#endif

	{
		unsigned long long bsize;
#ifdef HAVE_STATVFS_FRSIZE
		bsize = space.f_frsize;
#else
		bsize = space.f_bsize;
#endif
		
		data->t = (double)(space.f_blocks * bsize);
		data->u = (double)((space.f_blocks - space.f_bavail) * bsize);
		data->f = data->t - data->u;
		if(data->t > 0)
			data->p = ((float) data->u / data->t) * 100;
		else
			data->p = 0;
		
		return 0;
	}
	
	return -1;
}

int StatsDisks::should_ignore_mount(char *mount)
{
	if(disableFiltering == 1)
		return 0;

	if(!strncmp(mount, "/export", 7))
		return 1;
//	if(!strncmp(mount, "/var", 4))
//		return 1;
	if(!strncmp(mount, "/boot", 5))
		return 1;
	if(!strncmp(mount, "/tmp", 4))
		return 1;
	if(!strncmp(mount, "/system", 7))
		return 1;
	if(!strncmp(mount, "/usr/obj", 8))
		return 1;
	if(!strncmp(mount, "/usr/src", 8))
		return 1;
	if(!strncmp(mount, "/usr/local", 10))
		return 1;
	if(!strncmp(mount, "/usr/X11", 8))
		return 1;
	if(!strcmp(mount, "/usr"))
		return 1;
	if(!strcmp(mount, "/kern"))
		return 1;
	if(!strcmp(mount, "/dev/pts"))
		return 1;
	if(!strcmp(mount, "/compat/linux/proc"))
		return 1;
	if(!strncmp(mount, "/usr/jails/.", 12))
		return 1;
	if(!strncmp(mount, "/usr/home", 9))
		return 1;
	if(!strcmp(mount, "/usr/jails"))
		return 1;
	if(!strcmp(mount, "/usr/ports"))
		return 1;
	return 0;
}

int StatsDisks::should_ignore_type(char *type)
{
	if(!strcmp(type, "rpc_pipefs"))
		return 1;
	if(!strcmp(type, "rootfs"))
		return 1;
	if(!strcmp(type, "configfs"))
		return 1;
	if(!strcmp(type, "hugetlbfs"))
		return 1;
	if(!strcmp(type, "nfsd"))
		return 1;
	if(!strcmp(type, "mqueue"))
		return 1;
	if(!strcmp(type, "selinuxfs"))
		return 1;
	if(!strcmp(type, "linprocfs"))
		return 1;
	if(!strcmp(type, "binfmt_misc"))
		return 1;
	if(!strcmp(type, "tmpfs"))
		return 1;
	if(!strcmp(type, "devpts"))
		return 1;
	if(!strcmp(type, "devtmpfs"))
		return 1;
	if(!strcmp(type, "pstore"))
		return 1;
	if(!strcmp(type, "prl_fs"))
		return 1;
	if(!strcmp(type, "cgroup"))
		return 1;
	if(!strcmp(type, "iso9660"))
		return 1;
	if(!strcmp(type, "securityfs"))
		return 1;
	if(!strcmp(type, "fusectl"))
		return 1;
	if(!strcmp(type, "proc"))
		return 1;
	if(!strcmp(type, "procfs"))
		return 1;
	if(!strcmp(type, "debugfs"))
		return 1;
	if(!strcmp(type, "fuse.gvfsd-fuse"))
		return 1;
	if(!strcmp(type, "sysfs"))
		return 1;
	if(!strcmp(type, "devfs"))
		return 1;
	if(!strcmp(type, "autofs"))
		return 1;
	if(!strcmp(type, "fd"))
		return 1;
	if(!strcmp(type, "lofs"))
		return 1;
	if(!strcmp(type, "sharefs"))
		return 1;
	if(!strcmp(type, "objfs"))
		return 1;
	if(!strcmp(type, "mntfs"))
		return 1;
	if(!strcmp(type, "dev"))
		return 1;
	return 0;			
}

void StatsDisks::update(long long sampleID)
{
	#ifdef USE_STRUCT_MNTENT
		struct mntent *entry;
		FILE *table;
		if (!(table = setmntent(_PATH_MOUNTED, "r"))) return;
		while ((entry = getmntent(table)) != 0)
		{
			processDisk(entry->mnt_fsname, entry->mnt_dir, entry->mnt_type);
		}
		endmntent(table);

	#elif defined(USE_STRUCT_MNTTAB)
		struct mnttab *entry, ebuf;
		FILE *table;
		if (!(table = fopen(_PATH_MOUNTED, "r"))) return;
		resetmnttab(table);

		entry = &ebuf;
		while (!getmntent(table, entry))
		{
			processDisk(entry->mnt_special, entry->mnt_mountp, entry->mnt_fstype);
		}

		fclose(table);
	#else
		#ifdef GETMNTINFO_STATVFS
			struct statvfs *entry;
		#else
			struct statfs *entry;
		#endif

		int mnts;
		if (!(mnts = getmntinfo(&entry, MNT_NOWAIT)))
			return;

        int x;
        for(x=0;x<mnts;x++)
		{
			processDisk(entry[x].f_mntfromname, entry[x].f_mntonname, entry[x].f_fstypename);
		}
	#endif
}

void StatsDisks::processDisk(char *name, char *mount, char *type)
{
	bool exists = false;
	if(_items.size() > 0)
	{
		for (vector<disk_info>::iterator curdisk = _items.begin(); curdisk != _items.end(); ++curdisk)
		{
			if(string(name) == (*curdisk).key)
			{
				exists = true;
				break;
			}
		}
	}

	if(!exists && (should_ignore_type(type) || should_ignore_mount(mount)))
		return;

	createDisk(name);
	
	for (vector<disk_info>::iterator curdisk = _items.begin(); curdisk != _items.end(); ++curdisk)
	{
		if(string(name) == (*curdisk).key)
		{
			if((*curdisk).is_new == true)
			{
				(*curdisk).is_new = false;

				if((*curdisk).uuid.size() == 0)
				{
					#ifdef USE_DISKUUID_PROCFS
					DIR *dpdf;
					struct dirent *epdf;
					dpdf = opendir("/dev/disk/by-uuid");
					if (dpdf != NULL){
						while ((epdf = readdir(dpdf))){
							string file = string(epdf->d_name);

							char buff[PATH_MAX];
							string fullpath = "/dev/disk/by-uuid/" + file;
							ssize_t len = ::readlink(fullpath.c_str(), buff, sizeof(buff)-1);
							if (len != -1) {
 								buff[len] = '\0';
								vector<string> components = explode(string(buff), "/");
								if(components.size() > 0)
								{
									string p = "/dev/" + components.back();
									if(p == (*curdisk).key)
				  						(*curdisk).uuid = file;
								}
						    }
						}
						closedir(dpdf);
					}
					#endif
					if((*curdisk).uuid.size() == 0)
  						(*curdisk).uuid = (*curdisk).key;
				}

				(*curdisk).name = string(mount);

				string disk_label, disk_label_custom;
				disk_label = "";
				disk_label_custom = "";
			
				// Read custom disk label from config
				vector<string> conf_label;
				for (vector<string>::iterator cur_label = customNames.begin(); cur_label != customNames.end(); ++cur_label)
				{
					conf_label = explode(*cur_label);
				
					if (conf_label[1].length())
					{
						if (conf_label[0] == (*curdisk).key || conf_label[0] == (*curdisk).name)
						{
							disk_label_custom = trim(conf_label[1], "\"");
							break;
						}
					}
				}
			
				if (useMountPaths)
					disk_label = (*curdisk).name;
				else
					disk_label = (*curdisk).key;
			
				// Set custom disk label if configured. Will override everything.
				if (disk_label_custom.length())
					disk_label = disk_label_custom;
			
				(*curdisk).displayName = disk_label;

				loadHistoryForDisk(&(*curdisk));
			}

			if(ready == 0)
			{
				(*curdisk).active = true;
				break;
			}

			disk_data data;
			if (get_sizes(mount, &data) == 0)
			{
				(*curdisk).active = true;

				data.sampleID = sampleIndex[0].sampleID;
				data.time = sampleIndex[0].time;

				(*curdisk).samples[0].push_front(data);
				if ((*curdisk).samples[0].size() > HISTORY_SIZE)
					(*curdisk).samples[0].pop_back();
			}
		}
	}
}

#else /*USE_DISK_STATFS*/

void StatsDisks::update(long long sampleID)
{

}

#endif

void StatsDisks::prepareUpdate()
{
	if(ready == 0)
		return;

	StatsBase::prepareUpdate();

	if(_items.size() > 0)
	{
		for (vector<disk_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			(*cur).active = false;
		}
	}
}

void StatsDisks::createDisk(string key)
{
	if(_items.size() > 0)
	{
		for (vector<disk_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			disk_info interface = *cur;
			if (interface.key == key)
			{
				return;
			}
		}
	}

	disk_info item;
	item.is_new = true;
	item.key = key;
	session++;

	_items.insert(_items.begin(), item);	
}

void StatsDisks::loadHistoryForDisk(disk_info *disk)
{
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
			sqlite3_bind_text(query._statement, 2, disk->uuid.c_str(), -1, SQLITE_STATIC);

			while(query.next())
			{
				disk_data sample;
				sample.t = query.doubleForColumn("total");
				sample.u = query.doubleForColumn("used");
				sample.f = query.doubleForColumn("free");
				sample.sampleID = (long long)query.doubleForColumn("sample");
				sample.time = query.doubleForColumn("time");
				disk->samples[x].push_front(sample);
			}
		}
	}
#endif
}

void StatsDisks::init()
{	
	initShared();
	ready = 0;

	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		databaseType = 1;
		databasePrefix = "disk_";

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
				string sql = "create table " + table + " (sample double NOT NULL, time double NOT NULL DEFAULT 0, uuid varchar(255) NOT NULL, total double NOT NULL DEFAULT 0, used double NOT NULL DEFAULT 0, free double NOT NULL DEFAULT 0)";		
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
void StatsDisks::loadPreviousSamples()
{
	loadPreviousSamplesAtIndex(1);
	loadPreviousSamplesAtIndex(2);
	loadPreviousSamplesAtIndex(3);
	loadPreviousSamplesAtIndex(4);
	loadPreviousSamplesAtIndex(5);
	loadPreviousSamplesAtIndex(6);
	loadPreviousSamplesAtIndex(7);
}

void StatsDisks::loadPreviousSamplesAtIndex(int index)
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

void StatsDisks::updateHistory()
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
					for (vector<disk_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
					{
						disk_data sample = historyItemAtIndex(x, (*cur));

						(*cur).samples[x].push_front(sample);	
						if ((*cur).samples[x].size() > HISTORY_SIZE) (*cur).samples[x].pop_back();

						if(sample.empty)
							continue;

						string table = databasePrefix + tableAtIndex(x);
						string sql = "insert into " + table + " (sample, time, total, used, free, uuid) values(?, ?, ?, ?, ?, ?)";

						DatabaseItem dbItem = _database.databaseItem(sql);
						sqlite3_bind_double(dbItem._statement, 1, (double)sample.sampleID);
						sqlite3_bind_double(dbItem._statement, 2, sample.time);
						sqlite3_bind_double(dbItem._statement, 3, sample.t);
						sqlite3_bind_double(dbItem._statement, 4, sample.u);
						sqlite3_bind_double(dbItem._statement, 5, sample.f);
						sqlite3_bind_text(dbItem._statement, 6, (*cur).uuid.c_str(), -1, SQLITE_STATIC);
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

disk_data StatsDisks::historyItemAtIndex(int index, disk_info item)
{
	disk_data sample;
	double u = 0;
	double f = 0;
	double t = 0;

	std::deque<disk_data> from = item.samples[sampleIndex[index].historyIndex];
	double minimumTime = sampleIndex[index].time - sampleIndex[index].interval;
	double maximumTime = sampleIndex[index].time;
	if(sampleIndex[index].historyIndex == 0)
		maximumTime += 0.99;

	int count = 0;
	if(from.size() > 0)
	{
		for (deque<disk_data>::iterator cursample = from.begin(); cursample != from.end(); ++cursample)
		{
			if ((*cursample).time > maximumTime)
				continue;

			if ((*cursample).time < minimumTime)
				break;

			u += (*cursample).u;
			f += (*cursample).f;
			t = (*cursample).t;
			count++;
		}
		if (count > 0)
		{
			u /= count;
			f /= count;
		}
	}

	sample.u = u;
	sample.f = f;
	sample.t = t;

	sample.sampleID = sampleIndex[index].sampleID;
	sample.time = sampleIndex[index].time;
	sample.empty = false;
	if(count == 0)
		sample.empty = true;

	return sample;
}
#endif

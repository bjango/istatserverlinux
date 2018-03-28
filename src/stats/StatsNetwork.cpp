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

#include "StatsNetwork.h"

using namespace std;

#if defined(USE_NET_GETIFADDRS) && defined(HAVE_GETIFADDRS)

void StatsNetwork::init()
{
	_init();
}

void StatsNetwork::update(long long sampleID)
{
	struct ifaddrs *ifap, *ifa;

	if (getifaddrs(&ifap) < 0)
	{
		return;
	}

	vector<string> keys;

	for (ifa = ifap; ifa; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_flags & IFF_UP)
		{
			struct ifaddrs *iftmp;

            if (ifa->ifa_addr == NULL) {
                continue;
			}

			if (ifa->ifa_addr->sa_family != AF_LINK) {
				continue;
			}

			for (iftmp = ifa->ifa_next; iftmp != NULL; iftmp = iftmp->ifa_next)
			{
				if(strcmp(ifa->ifa_name, iftmp->ifa_name) == 0)
				{					
					if(keys.size() > 0)
					{
						if (std::find(keys.begin(), keys.end(), string(ifa->ifa_name)) != keys.end())
							continue;
					}

					struct if_data *ifd = (struct if_data *) ifa->ifa_data;
					processInterface(string(ifa->ifa_name), sampleID, ifd->ifi_obytes, ifd->ifi_ibytes);
					keys.push_back(string(ifa->ifa_name));
				}
			}
		}
	}

	freeifaddrs(ifap);
}

#elif defined(HAVE_LIBPERFSTAT) && defined(USE_NET_PERFSTAT)

void StatsNetwork::init()
{
	_init();
}

void StatsNetwork::update(long long sampleID)
{
	perfstat_netinterface_total_t ninfo;
	perfstat_netinterface_total(NULL, &ninfo, sizeof ninfo, 1);
	processInterface(string("net"), sampleID, ninfo.obytes, ninfo.ibytes);
} /*NET_USE_PERFSTAT*/

#elif defined(USE_NET_PROCFS)

void StatsNetwork::init()
{
	_init();
}

void StatsNetwork::update(long long sampleID)
{
	char dev[255];
	FILE * fp = NULL;
	
	if (!(fp = fopen("/proc/net/dev", "r")))
	{
		cout << "fopen: /proc/net/dev" << endl;
		return;
	}

#ifdef HAVE_GETIFADDRS	
	struct ifaddrs *ifap, *ifa;
	vector<string> active_infs;

	if (getifaddrs(&ifap) >= 0) {
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_flags & IFF_UP && ifa->ifa_name)
			{
				active_infs.push_back(string(ifa->ifa_name));
			}
		}
		freeifaddrs(ifap);
	}
#endif

	if(fscanf(fp, "%*[^\n] %*[^\n] "))
	{
	}

	while (!feof(fp))
	{
		unsigned long long upload;
		unsigned long long download;

		if(fscanf(fp, "%16[^:]:%llu %*u %*u %*u %*u %*u %*u %*u %llu %*[^\n] ", dev, &download, &upload) > 0)
		{
#ifdef HAVE_GETIFADDRS
			if(active_infs.size() > 0)
			{
				if (std::find(active_infs.begin(), active_infs.end(), string(dev)) == active_infs.end())
					continue;
			}
#endif
			processInterface(string(dev), sampleID, upload, download);
		}
		else
		{
			if(fscanf(fp, "%*[^\n]")){}
		}
	}

	fclose(fp);
}

#elif defined(USE_NET_SYSCTL)

void StatsNetwork::init()
{
	_init();
}

int StatsNetwork::get_ifcount()
{
	size_t len;
	int name[5], count;

	name[0] = CTL_NET;
	name[1] = PF_LINK;
	name[2] = NETLINK_GENERIC;
	name[3] = IFMIB_SYSTEM;
	name[4] = IFMIB_IFCOUNT;

	len = sizeof(int);

	sysctl(name, 5, &count, &len, NULL, 0);

	return count;
}

void StatsNetwork::update(long long sampleID)
{
    int i;
	int name[6];
	size_t len;
	struct ifmibdata ifmd;
	int ifcount = get_ifcount();

	len = sizeof(ifmd);

	name[0] = CTL_NET;
	name[1] = PF_LINK;
	name[2] = NETLINK_GENERIC;
	name[3] = IFMIB_IFDATA;
	name[5] = IFDATA_GENERAL;

	for (i = 1; i <= ifcount; i++)
	{
		name[4] = i;

		sysctl(name, 6, &ifmd, &len, (void *) 0, 0);

		unsigned int flags = ifmd.ifmd_flags;
		
		if ((flags & IFF_UP) && (flags & IFF_RUNNING))
		{
			unsigned long long upload =  ifmd.ifmd_data.ifi_obytes;
			unsigned long long download = ifmd.ifmd_data.ifi_ibytes;

			processInterface(string(ifmd.ifmd_name), sampleID, upload, download);
		}
	}
} /*NET_USE_SYSCTL*/

#elif defined(HAVE_LIBKSTAT) && defined(USE_NET_KSTAT)

void StatsNetwork::init()
{
	_init();

	vector<string> infs = interfaces();
	if(infs.size() > 0)
	{
		for (vector<string>::iterator cur = infs.begin(); cur != infs.end(); ++cur)
		{
			createInterface((*cur));
		}		
	}
}

vector<string> StatsNetwork::interfaces()
{

	vector<string> interfaces;

	int		n;
	int		s;
	char		*buf;
	struct lifnum	lifn;
	struct lifconf	lifc;
	struct lifreq	*lifrp;
	int		numifs;
	size_t		bufsize;

	s = socket(AF_INET6, SOCK_DGRAM, 0);
	if (s < 0) {
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0)
			return interfaces;
	}

	lifn.lifn_family = AF_UNSPEC;
	lifn.lifn_flags = LIFC_NOXMIT | LIFC_TEMPORARY | LIFC_ALLZONES;
	if (ioctl(s, SIOCGLIFNUM, (char *)&lifn) < 0)
		return interfaces;
	numifs = lifn.lifn_count;

	bufsize = numifs * sizeof (struct lifreq);
	buf = (char*)malloc(bufsize);
	if (buf == NULL) {
		(void) close(s);
		return interfaces;
	}
	lifc.lifc_family = AF_UNSPEC;
	lifc.lifc_flags = LIFC_NOXMIT | LIFC_TEMPORARY | LIFC_ALLZONES;
	lifc.lifc_len = bufsize;
	lifc.lifc_buf = buf;
	if (ioctl(s, SIOCGLIFCONF, (char *)&lifc) < 0) {
		(void) close(s);
		free(buf);
		return interfaces;
	}

	lifrp = lifc.lifc_req;
	(void) close(s);

	for (n = numifs; n > 0; n--, lifrp++) {
		bool found = false;

		string infname = string(lifrp->lifr_name);

		if (infname.find(":") != std::string::npos)
		{
			infname = infname.substr(0, infname.find(":"));
		}

		if(interfaces.size() > 0)
		{
			for (vector<string>::iterator cur = interfaces.begin(); cur != interfaces.end(); ++cur)
			{
				if((*cur) == infname)
				{
					found = true;
				}
			}
		}


		if (found == false) {
			interfaces.push_back(infname);
		}
	}

	free(buf);

	return interfaces;
}

void StatsNetwork::update(long long sampleID)
{
	if(_items.size() > 0)
	{
		for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			kstat_t *ksp;
			kstat_named_t *kn;
			char name[32];
			char module[32];
			char *p;

			char *_dev = (char*)((*cur).device.c_str());

			unsigned long long upload;
			unsigned long long download;

			strcpy(module, (char*)"link");
			if((p = strchr(_dev, ':')) == NULL)
			{
				strncpy(name, _dev, sizeof(name) - 1);
				name[sizeof(name) - 1] = 0;
			}
			else
			{
				strncpy(name, p + 1, sizeof(name) - 1);
				name[sizeof(name) - 1]=0;
				strncpy(module, _dev, sizeof(module) - 1);
				module[p - _dev] = 0;
			}
			if(NULL == (ksp = kstat_lookup(ksh, module, -1, name)))
			{
				(*cur).active = false;
				continue;
			}

			if(-1 == kstat_read(ksh, ksp, NULL))
			{
				(*cur).active = false;
				continue;				
			}

			kn = (kstat_named_t *) ksp->ks_data;
			if(NULL == (kn = (kstat_named_t *) kstat_data_lookup(ksp, (char*)"obytes64")))
			{
				(*cur).active = false;
				continue;
			}

			upload = ksgetull(kn);
	
			if(NULL == (kn = (kstat_named_t *) kstat_data_lookup(ksp, (char*)"rbytes64")))
			{
				(*cur).active = false;
				continue;
			}

			download = ksgetull(kn);

			processInterface((*cur).device, sampleID, upload, download);
		}
	}
}

# else

void StatsNetwork::init()
{
	_init();
}

void StatsNetwork::update(long long sampleID)
{

}

#endif

#define INT_TO_ADDR(_addr) \
(_addr & 0xFF), \
(_addr >> 8 & 0xFF), \
(_addr >> 16 & 0xFF), \
(_addr >> 24 & 0xFF)

// IPS_IOCTL - We dont use getifaddrs on solaris even if its available. Doesn't seem to work correctly
#if defined(HAVE_GETIFADDRS) && !defined(IPS_IOCTL)
void StatsNetwork::updateAddresses()
{
	if(_items.size() == 0)
		return;

	struct ifaddrs *ifap, *ifa;

	if (getifaddrs(&ifap) < 0) {
		return;
	}

	for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
	{
		(*cur).addresses.clear();
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_flags & IFF_UP)
		{
			struct ifaddrs *iftmp;

            if (ifa->ifa_addr == NULL) {
                continue;
			}

			if (ifa->ifa_addr->sa_family != AF_LINK) {
				continue;
			}

			for (iftmp = ifa->ifa_next; iftmp != NULL; iftmp = iftmp->ifa_next)
			{
				if(strcmp(ifa->ifa_name, iftmp->ifa_name) == 0)
				{					
					for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
					{
						network_info interface = *cur;
						if (interface.device == string(ifa->ifa_name))
						{
							if (iftmp->ifa_addr->sa_family == AF_INET) {
								(*cur).addresses.push_back(string(inet_ntoa(((struct sockaddr_in *)iftmp->ifa_addr)->sin_addr)));
							}
							
							#ifdef AF_INET6
								if (iftmp->ifa_addr->sa_family == AF_INET6) {
									char ip[INET6_ADDRSTRLEN];
									const char *conversion = inet_ntop(AF_INET6, &((struct sockaddr_in6 *)iftmp->ifa_addr)->sin6_addr, ip, sizeof(ip));
									(*cur).addresses.push_back(string(conversion));
								}
							#endif
						}
					}		
				}
			}
		}
	}

	freeifaddrs(ifap);
}

#else

void StatsNetwork::updateAddresses()
{
	if(_items.size() == 0)
		return;

	struct ifconf ifc;
    struct ifreq ifr[10];
    int i, sd, ifc_num, addr;

	for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
	{
		(*cur).addresses.clear();
	}

    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd > 0)
    {
        ifc.ifc_len = sizeof(ifr);
        ifc.ifc_ifcu.ifcu_buf = (caddr_t)ifr;

        if (ioctl(sd, SIOCGIFCONF, &ifc) == 0)
        {
            ifc_num = ifc.ifc_len / sizeof(struct ifreq);

            for (i = 0; i < ifc_num; ++i)
            {
                if (ifr[i].ifr_addr.sa_family != AF_INET)
                {
                    continue;
                }

                string ip = string("");

                if (ioctl(sd, SIOCGIFADDR, &ifr[i]) == 0)
                {
                    addr = ((struct sockaddr_in *)(&ifr[i].ifr_addr))->sin_addr.s_addr;

					char str[32];
					sprintf(str, "%d.%d.%d.%d", INT_TO_ADDR(addr));
					ip = string(str);
                }
                if(_items.size() > 0)
                {
	                for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
					{
						network_info interface = *cur;
						if (interface.device == string(ifr[i].ifr_name))   
						{
							(*cur).addresses.push_back(ip);
						}      
					}
				}
            }                      
        }

        close(sd);
    }
}

#endif

void StatsNetwork::createInterface(string key)
{
	if(key == "lo" || key == "lo0")
		return;

	if(_items.size() > 0)
	{
		for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			network_info interface = *cur;
			if (interface.device == key)
			{
				return;
			}
		}
	}

	network_info item;
	item.last_down = 0;
	item.last_up = 0;
	item.device = key;
	session++;

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
				net_data sample;
				sample.d = query.doubleForColumn("download");
				sample.u = query.doubleForColumn("upload");
				sample.sampleID = (long long)query.doubleForColumn("sample");
				sample.time = query.doubleForColumn("time");
				item.samples[x].push_front(sample);
			}
		}
	}
#endif
	_items.insert(_items.begin(), item);	

	updateAddresses();
}

void StatsNetwork::processInterface(string key, long long sampleID, unsigned long long upload, unsigned long long download)
{
	if(key == "lo" || key == "lo0" || key == "nic" || key.find("virbr") == 0 || key.find("ath0") == 0)
		return;

	createInterface(key);

	for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
	{
		if (key == (*cur).device)
		{
			(*cur).active = true;

			if(ready == 0)
				return;

			net_data data;

			if((*cur).last_up == 0)
				(*cur).last_up = upload;
			if((*cur).last_down == 0)
				(*cur).last_down = download;

			double u = upload - (*cur).last_up;
			double d = download - (*cur).last_down;

			(*cur).last_up = upload;
			(*cur).last_down = download;

			data.u = u;
			data.d = d;
			data.sampleID = sampleIndex[0].sampleID;
			data.time = sampleIndex[0].time;

			(*cur).samples[0].push_front(data);
			if ((*cur).samples[0].size() > HISTORY_SIZE)
				(*cur).samples[0].pop_back();

			break;
		}
	}
}

void StatsNetwork::prepareUpdate()
{
	if(ready == 0)
		return;

	StatsBase::prepareUpdate();

	if(_items.size() > 0)
	{
		for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			(*cur).active = false;
		}
	}
}

void StatsNetwork::_init()
{	
	initShared();
	ready = 0;

	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		databaseType = 1;
		databasePrefix = "network_";

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
				string sql = "create table " + table + " (sample double NOT NULL, time double NOT NULL DEFAULT 0, uuid varchar(255) NOT NULL, download double NOT NULL DEFAULT 0, upload double NOT NULL DEFAULT 0)";		
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
void StatsNetwork::loadPreviousSamples()
{
	loadPreviousSamplesAtIndex(1);
	loadPreviousSamplesAtIndex(2);
	loadPreviousSamplesAtIndex(3);
	loadPreviousSamplesAtIndex(4);
	loadPreviousSamplesAtIndex(5);
	loadPreviousSamplesAtIndex(6);
	loadPreviousSamplesAtIndex(7);
}

void StatsNetwork::loadPreviousSamplesAtIndex(int index)
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

void StatsNetwork::updateHistory()
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
					for (vector<network_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
					{
						net_data sample = historyItemAtIndex(x, (*cur));

						(*cur).samples[x].push_front(sample);	
						if ((*cur).samples[x].size() > HISTORY_SIZE) (*cur).samples[x].pop_back();

						if(sample.empty)
							continue;

						string table = databasePrefix + tableAtIndex(x);
						string sql = "insert into " + table + " (sample, time, upload, download, uuid) values(?, ?, ?, ?, ?)";

						DatabaseItem dbItem = _database.databaseItem(sql);
						sqlite3_bind_double(dbItem._statement, 1, (double)sample.sampleID);
						sqlite3_bind_double(dbItem._statement, 2, sample.time);
						sqlite3_bind_double(dbItem._statement, 3, sample.u);
						sqlite3_bind_double(dbItem._statement, 4, sample.d);
						sqlite3_bind_text(dbItem._statement, 5, (*cur).device.c_str(), -1, SQLITE_STATIC);
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

net_data StatsNetwork::historyItemAtIndex(int index, network_info item)
{
	net_data sample;
	double u = 0;
	double d = 0;

	std::deque<net_data> from = item.samples[sampleIndex[index].historyIndex];
	double minimumTime = sampleIndex[index].time - sampleIndex[index].interval;
	double maximumTime = sampleIndex[index].time;
	if(sampleIndex[index].historyIndex == 0)
		maximumTime += 0.99;

	int count = 0;
	if(from.size() > 0)
	{
		for (deque<net_data>::iterator cursample = from.begin(); cursample != from.end(); ++cursample)
		{
			if ((*cursample).time > maximumTime)
				continue;

			if ((*cursample).time < minimumTime)
				break;

			u += (*cursample).u;
			d += (*cursample).d;
			count++;
		}
		if (count > 0)
		{
			u /= count;
			d /= count;
		}
	}

	sample.u = u;
	sample.d = d;

	sample.sampleID = sampleIndex[index].sampleID;
	sample.time = sampleIndex[index].time;
	sample.empty = false;
	if(count == 0)
		sample.empty = true;

	return sample;
}
#endif

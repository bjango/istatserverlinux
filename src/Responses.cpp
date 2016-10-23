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
#include <sstream>
#include <string.h>
#include <iostream>
#include <string.h>
#include <sys/param.h>
#include "System.h"

#include "Responses.h"
#include "Stats.h"
#include "Utility.h"

using namespace std;

string isr_create_header()
{
	return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
}

string isr_accept_code()
{
	stringstream temp;
	temp << isr_create_header() << "<isr type=\"103\"></isr>";
	return temp.str();
}

string isr_reject_code()
{
	stringstream temp;
	temp << isr_create_header() << "<isr type=\"102\"></isr>";
	return temp.str();
}

string isr_serverinfo(int session, int auth, string uuid, bool historyEnabled)
{
	int history = 0;
	#ifdef USE_SQLITE
	if(historyEnabled == true)
		history = 1;
	#endif

	stringstream temp;
	temp << isr_create_header() << "<isr type=\"101\" build=\""<< SERVER_BUILD << "\" version=\""<< SERVER_VERSION << "\" history=\""<< history << "\" protocol=\""<< PROTOCOL_VERSION << "\" platform=\"" << serverPlatform() << "\" session=\"" << session << "\" uuid=\"" << uuid << "\" auth=\"" << auth << "\"></isr>";
	return temp.str();
}

string isr_accept_connection()
{
	stringstream temp;
	temp << isr_create_header() << "<isr type=\"100\" protocol=\"" << PROTOCOL_VERSION << "\" sec=\"1\"></isr>";
	return temp.str();
}

string isr_cpu_data(xmlNodePtr node, Stats *stats)
{
	#ifdef USE_CPU_NONE
	return "";
	#endif

	stringstream output;

	char *identifiers = (char *)xmlGetProp(node, (const xmlChar *)"samples");
	vector<string> identifierItems = explode(string(identifiers), "|");
	for(uint x = 0;x < identifierItems.size(); x++)
	{
		double sampleID = to_double(identifierItems[x].c_str());

		deque<cpu_data> samples;
		for(size_t i = 0;i < stats->cpuStats.samples[x].size(); i++)
		{
			cpu_data sample = stats->cpuStats.samples[x][i];
			if (sample.sampleID > sampleID)
			{
				samples.push_front(sample);
			}
			else
				break;
		}

		output << "<stat type=\"cpu\" interval=\"" << x << "\" session=\"" << stats->cpuStats.session << "\" id=\"" << stats->cpuStats.sampleIndex[x].sampleID << "\" threads=\"" << stats->processStats.threadCount << "\" tasks=\"" << stats->processStats.processCount << "\" samples=\"" << samples.size() << "\">";
		for(size_t i = 0;i < samples.size(); i++)
		{
			struct cpu_data sample = samples[i];	
			output << "<s id=\"" << sample.sampleID << "\" time=\"" << (long long)sample.time << "\" u=\"" << sample.u << "\" s=\"" << sample.s << "\" n=\"" << sample.n << "\" io=\"" << sample.io << "\"";
		
			#ifdef USE_CPU_PERFSTAT
			if(stats->cpuStats.hasLpar)
				output << " ent=\"" << sample.ent << "\" phys=\"" << sample.phys << "\"";
			#endif

			output << "></s>";
		}
		output << "</stat>";
	}
	free(identifiers);

	return output.str();
}

string isr_memory_data(xmlNodePtr node, Stats *stats)
{
	#ifdef USE_MEM_NONE
	return "";
	#endif

	stringstream output;

	char *identifiers = (char *)xmlGetProp(node, (const xmlChar *)"samples");
	vector<string> identifierItems = explode(string(identifiers), "|");
	for(uint x = 0;x < identifierItems.size(); x++)
	{
		double sampleID = to_double(identifierItems[x].c_str());

		deque<mem_data> samples;
		for(size_t i = 0;i < stats->memoryStats.samples[x].size(); i++)
		{
			mem_data sample = stats->memoryStats.samples[x][i];
			if (sample.sampleID > sampleID)
			{
				samples.push_front(sample);
			}
			else
				break;
		}

		output << "<stat type=\"memory\" interval=\"" << x << "\" session=\"" << stats->memoryStats.session << "\" id=\"" << stats->memoryStats.sampleIndex[x].sampleID << "\" samples=\"" << samples.size() << "\">";
		for(size_t i = 0;i < samples.size(); i++)
		{
			struct mem_data mem = samples[i];	
			output << "<s id=\"" << mem.sampleID << "\" time=\"" << (long long)mem.time << "\"";

			if(mem.values[memory_value_file] >= 0)
				output << " file=\"" << mem.values[memory_value_file] << "\"";
			if(mem.values[memory_value_ex] >= 0)
			  	output << " ex=\"" << mem.values[memory_value_ex] << "\"";
			if(mem.values[memory_value_buffer] >= 0)
				output << " buf=\"" << mem.values[memory_value_buffer] << "\"";
			if(mem.values[memory_value_used] >= 0)
				output << " u=\"" << mem.values[memory_value_used] << "\"";
			if(mem.values[memory_value_wired] >= 0)
				output << " w=\"" << mem.values[memory_value_wired] << "\"";
			if(mem.values[memory_value_cached] >= 0)
				output << " ca=\"" << mem.values[memory_value_cached] << "\"";
			if(mem.values[memory_value_active] >= 0)
				output << " a=\"" << mem.values[memory_value_active] << "\"";
			if(mem.values[memory_value_inactive] >= 0)
				output << " i=\"" << mem.values[memory_value_inactive] << "\"";
			if(mem.values[memory_value_free] >= 0)
				output << " f=\"" << mem.values[memory_value_free] << "\"";
			if(mem.values[memory_value_total] >= 0)
				output << " t=\"" << mem.values[memory_value_total] << "\"";
			if(mem.values[memory_value_swapused] >= 0)
				output << " su=\"" << mem.values[memory_value_swapused]  << "\"";
			if(mem.values[memory_value_swaptotal] >= 0)
				output << " st=\"" << mem.values[memory_value_swaptotal] << "\"";
			if(mem.values[memory_value_swapin] >= 0)
				output << " pi=\"" << mem.values[memory_value_swapin] << "\"";
			if(mem.values[memory_value_swapin] >= 0)
				output << " po=\"" << mem.values[memory_value_swapout] << "\"";
			if(mem.values[memory_value_virtualtotal] >= 0)
				output << " vt=\"" << mem.values[memory_value_virtualtotal] << "\"";
			if(mem.values[memory_value_virtualactive] >= 0)
				output << " va=\"" << mem.values[memory_value_virtualactive] << "\"";

			output << "></s>";
		}
		output << "</stat>";
	}
	free(identifiers);

	return output.str();
}

string keyForIndex(string uuid, int index)
{
	stringstream skey;
	skey << index << ":" << uuid;
	string key = skey.str();
	return key;
}

string isr_network_data(int index, long sampleID, StatsNetwork stats, vector<string> keys, vector<string> *added)
{
	stringstream output;
	output << "<stat type=\"network\" interval=\"" << index << "\" session=\"" << stats.session << "\" id=\"" << stats.sampleIndex[index].sampleID << "\">";

	for(size_t itemindex = 0;itemindex < stats._items.size(); itemindex++)
	{
		network_info item = stats._items[itemindex];
		if(!item.active)
			continue;

		if(!shouldAddKey(index, item.device, keys, added))
			continue;

		deque<net_data> samples;
		for(size_t i = 0;i < item.samples[index].size(); i++)
		{
			net_data sample = item.samples[index][i];
			if (sample.sampleID > sampleID)
			{
				samples.push_front(sample);
			}
			else
				break;
		}

		output << "<item uuid=\"" << item.device << "\" samples=\"" << samples.size() << "\"";
		if(index == 0)
		{
			string addresses = "";
			for(size_t i = 0;i < item.addresses.size(); i++)
			{
				if(addresses.size() > 0)
					addresses += ",";
				addresses += item.addresses[i];
			}
			output << " name=\"" << item.device << "\" ip=\"" << addresses << "\" d=\"" << item.last_down << "\" u=\"" << item.last_up << "\"";
		}

		output << ">";

		for(size_t i = 0;i < samples.size(); i++)
		{
			struct net_data sample = samples[i];	
			output << "<s id=\"" << sample.sampleID << "\" time=\"" << (long long)sample.time << "\" d=\"" << sample.d << "\" u=\"" << sample.u << "\"></s>";
		}
		output << "</item>";
	}
	output << "</stat>";

	return output.str();
}

bool shouldAddKey(int index, string key, vector<string> keys, vector<string> *added)
{
	bool add = true;
	string k = keyForIndex(key, index);
			
	for(size_t i = 0;i < added->size(); i++)
	{
		if(added->at(i) == k)
		{
			add = false;
			break;
		}
	}
	if(add == true && keys.size() > 0)
	{
		add = false;
		for(size_t i = 0;i < keys.size(); i++)
		{
			if(keys[i] == key)
			{
				add = true;
				break;
			}
		}
	}
	if(add == true)
	{
		added->push_back(k);
	}

	return add;
}

string isr_multiple_data(xmlNodePtr node, Stats *stats)
{
	stringstream output;
	vector<string> addedKeys;

	char *type = (char *)xmlGetProp(node, (const xmlChar *)"type");
	xmlNodePtr child = node->children;
	while (child){
		char *identifiers = (char *)xmlGetProp(child, (const xmlChar *)"samples");
		char *keys = (char *)xmlGetProp(child, (const xmlChar *)"keys");

		vector<string> identifierItems = explode(string(identifiers), "|");
		vector<string> keyItems;
		if(keys != NULL)
			 keyItems = explode(string(keys), "|");

		for(uint x = 0;x < identifierItems.size(); x++)
		{
			double sampleID = to_double(identifierItems[x].c_str());

			if(strcmp(type, "network") == 0)
				output << isr_network_data(x, sampleID, stats->networkStats, keyItems, &addedKeys);
			else if(strcmp(type, "diskactivity") == 0)
				output << isr_activity_data(x, sampleID, stats->activityStats, keyItems, &addedKeys);
			else if(strcmp(type, "sensors") == 0)
				output << isr_sensor_data(x, sampleID, stats->sensorStats, keyItems, &addedKeys);
			else if(strcmp(type, "disks") == 0)
				output << isr_disk_data(x, sampleID, stats->diskStats, keyItems, &addedKeys);
			else if(strcmp(type, "processes") == 0)
				output << isr_process_data(x, sampleID, stats->processStats, keyItems, &addedKeys);
			else if(strcmp(type, "battery") == 0)
				output << isr_battery_data(x, sampleID, stats->batteryStats, keyItems, &addedKeys);
		}

		if(keys != NULL)
			free(keys);

		free(identifiers);
		child = child->next;
	}
	free(type);
	return output.str();
}

string isr_activity_data(int index, long sampleID, StatsActivity stats, vector<string> keys, vector<string> *added)
{
	stringstream output;
	output << "<stat type=\"diskactivity\" interval=\"" << index << "\" session=\"" << stats.session << "\" id=\"" << stats.sampleIndex[index].sampleID << "\">";

	for(size_t itemindex = 0;itemindex < stats._items.size(); itemindex++)
	{
		activity_info item = stats._items[itemindex];
		if(!item.active)
			continue;

		if(!shouldAddKey(index, item.device, keys, added))
			continue;

		deque<activity_data> samples;
		for(size_t i = 0;i < item.samples[index].size(); i++)
		{
			activity_data sample = item.samples[index][i];
			if (sample.sampleID > sampleID)
			{
				samples.push_front(sample);
			}
			else
				break;
		}

		output << "<item uuid=\"" << item.device << "\" samples=\"" << samples.size() << "\"";
		if(index == 0)
		{
			output << " name=\"" << item.device << "\" r=\"" << item.last_r << "\" w=\"" << item.last_w << "\" rio=\"" << item.last_rIOPS << "\" wio=\"" << item.last_wIOPS << "\"";
			if(item.mounts.size() > 0)
			{
				output << " mounts=\"";

				unsigned int x;
				for(x=0;x<item.mounts.size();x++)
				{
					if(x > 0)
						output << ",";
					output << item.mounts[x];

				}
				output << "\"";
			}
		}

		output << ">";

		for(size_t i = 0;i < samples.size(); i++)
		{
			struct activity_data sample = samples[i];	
			output << "<s id=\"" << sample.sampleID << "\" time=\"" << (long long)sample.time << "\" r=\"" << sample.r << "\" w=\"" << sample.w << "\"></s>";
		}
		output << "</item>";
	}
	output << "</stat>";

	return output.str();
}

string isr_disk_data(int index, long sampleID, StatsDisks stats, vector<string> keys, vector<string> *added)
{
	stringstream output;
	output << "<stat type=\"disks\" interval=\"" << index << "\" session=\"" << stats.session << "\" id=\"" << stats.sampleIndex[index].sampleID << "\">";

	for(size_t itemindex = 0;itemindex < stats._items.size(); itemindex++)
	{
		disk_info item = stats._items[itemindex];
		if(!item.active)
			continue;

		if(!shouldAddKey(index, item.key, keys, added))
			continue;

		deque<disk_data> samples;
		for(size_t i = 0;i < item.samples[index].size(); i++)
		{
			disk_data sample = item.samples[index][i];
			if (sample.sampleID > sampleID)
			{
				samples.push_front(sample);
			}
			else
				break;
		}

		output << "<item bsd=\"" << item.key << "\" uuid=\"" << item.uuid << "\" name=\"" << item.displayName << "\" samples=\"" << samples.size() << "\">";
		for(size_t i = 0;i < samples.size(); i++)
		{
			struct disk_data sample = samples[i];	
			output << "<s id=\"" << sample.sampleID << "\" time=\"" << (long long)sample.time << "\" f=\"" << sample.f << "\" u=\"" << sample.u << "\" s=\"" << sample.t << "\" p=\"" << sample.p << "\"></s>";
		}
		output << "</item>";
	}
	output << "</stat>";

	return output.str();
}

string isr_uptime_data(long uptime)
{
	#ifdef USE_UPTIME_NONE
	return "";
	#endif

	stringstream temp;
	temp << "<stat type=\"uptime\" u=\"" << uptime << "\"></stat>";
	return temp.str();
}

string isr_loadavg_data(xmlNodePtr node, Stats *stats)
{
	#ifdef USE_LOAD_NONE
	return "";
	#endif

	stringstream output;

	char *identifiers = (char *)xmlGetProp(node, (const xmlChar *)"samples");
	vector<string> identifierItems = explode(string(identifiers), "|");
	for(uint x = 0;x < identifierItems.size(); x++)
	{
		double sampleID = to_double(identifierItems[x].c_str());

		deque<load_data> samples;
		for(size_t i = 0;i < stats->loadStats.samples[x].size(); i++)
		{
			load_data sample = stats->loadStats.samples[x][i];
			if (sample.sampleID > sampleID)
			{
				samples.push_front(sample);
			}
			else
				break;
		}

		output << "<stat type=\"load\" interval=\"" << x << "\" session=\"" << stats->loadStats.session << "\" id=\"" << stats->loadStats.sampleIndex[x].sampleID << "\" samples=\"" << samples.size() << "\">";
		for(size_t i = 0;i < samples.size(); i++)
		{
			load_data sample = samples[i];
			output << "<s id=\"" << sample.sampleID << "\" time=\"" << (long long)sample.time << "\" one=\"" << sample.one << "\" five=\"" << sample.two << "\" fifteen=\"" << sample.three << "\"></s>";
		}
		output << "</stat>";
	}
	free(identifiers);

	return output.str();
}

string isr_sensor_data(int index, long sampleID, StatsSensors stats, vector<string> keys, vector<string> *added)
{
	stringstream output;
	output << "<stat type=\"sensors\" interval=\"" << index << "\" session=\"" << stats.session << "\" id=\"" << stats.sampleIndex[index].sampleID << "\" partial=\"1\">";

	for(size_t itemindex = 0;itemindex < stats._items.size(); itemindex++)
	{
		sensor_info item = stats._items[itemindex];
		if(!shouldAddKey(index, item.key, keys, added))
			continue;

		deque<sensor_data> samples;
		for(size_t i = 0;i < item.samples[index].size(); i++)
		{
			sensor_data sample = item.samples[index][i];
			if (sample.sampleID > sampleID)
			{
				samples.push_front(sample);
			}
			else
				break;
		}

		output << "<item low=\"" << item.lowestValue << "\" high=\"" << item.highestValue << "\" uuid=\"" << item.key << "\" name=\"" << item.label << "\" type=\"" << item.kind << "\" samples=\"" << samples.size() << "\">";
		for(size_t i = 0;i < samples.size(); i++)
		{
			struct sensor_data sample = samples[i];	
			output << "<s id=\"" << sample.sampleID << "\" time=\"" << (long long)sample.time << "\" v=\"" << sample.value << "\"></s>";
		}
		output << "</item>";
	}
	output << "</stat>";

	return output.str();
}

string isr_battery_data(int index, long sampleID, StatsBattery stats, vector<string> keys, vector<string> *added)
{
	stringstream output;
	output << "<stat type=\"battery\" interval=\"" << index << "\" session=\"" << stats.session << "\" id=\"" << stats.sampleIndex[index].sampleID << "\">";

	for(size_t itemindex = 0;itemindex < stats._items.size(); itemindex++)
	{
		battery_info item = stats._items[itemindex];
		if(!shouldAddKey(index, item.key, keys, added))
			continue;

		output << "<item uuid=\"" << item.key << "\" health=\"" << item.health << "\" time=\"" << item.timeRemaining << "\" cycles=\"" << item.cycles << "\" state=\"" << item.state << "\" source=\"" << item.source << "\" percentage=\"" << item.percentage << "\">";
		output << "</item>";
	}
	output << "</stat>";

	return output.str();
}

bool sortProcessesCPU (process_info i,process_info j) { return (j.cpu<i.cpu); }
bool sortProcessesMemory (process_info i,process_info j) { return (j.memory<i.memory); }
bool sortProcessesIORead (process_info i,process_info j) { return (j.io_read<i.io_read); }
bool sortProcessesIOWrite (process_info i,process_info j) { return (j.io_write<i.io_write); }

string isr_process_data(int index, long sampleID, StatsProcesses stats, vector<string> keys, vector<string> *added)
{
	vector<process_info> _history;

	if(stats._items.size() > 0)
	{
		for (vector<process_info>::iterator cur = stats._items.begin(); cur != stats._items.end(); ++cur)
		{
			process_info temp = *cur;
			_history.push_back(temp);
		}
	}

	stringstream output;

	if(_history.size() > 0)
	{
		output << "<stat type=\"processes\" interval=\"0\" id=\"" << _history.back().sampleID << "\">";
	
		if(shouldAddKey(0, "cpu", keys, added))
		{
			std::sort (_history.begin(), _history.end(), sortProcessesCPU);
			int count = 0;

			for (vector<process_info>::iterator cur = _history.begin(); cur != _history.end(); ++cur)
			{
				output << "<item key=\"" << cur->pid << "\" c=\"" << cur->cpu << "\" name=\"" << cur->name << "\"></item>";
				count++;
				if(count == 20)
					break;
			}

		}

		if(shouldAddKey(0, "memory", keys, added))
		{
			int count = 0;
			std::sort (_history.begin(), _history.end(), sortProcessesMemory);
	
			for (vector<process_info>::iterator cur = _history.begin(); cur != _history.end(); ++cur)
			{
				output << "<item key=\"" << cur->pid << "\" m=\"" << cur->memory << "\" name=\"" << cur->name << "\"></item>";
				count++;
				if(count == 20)
					break;
			}
		}

		/*
		if(keys.find("disks") != std::string::npos)
		{
			int count = 0;
			std::sort (_history->begin(), _history->end(), sortProcessesIORead);
	
			for (vector<process_info>::iterator cur = _history->begin(); cur != _history->end(); ++cur)
			{
				temp << "<item key=\"" << cur->pid << "\" r=\"" << cur->io_read << "\" w=\"" << cur->io_write << "\" name=\"" << cur->name << "\"></item>";
				count++;
				if(count == 10)
					break;
			}

			count = 0;
			std::sort (_history->begin(), _history->end(), sortProcessesIOWrite);
	
			for (vector<process_info>::iterator cur = _history->begin(); cur != _history->end(); ++cur)
			{
				temp << "<item key=\"" << cur->pid << "\" r=\"" << cur->io_read << "\" w=\"" << cur->io_write << "\" name=\"" << cur->name << "\"></item>";
				count++;
				if(count == 10)
					break;
			}
		}*/

		output << "</stat>";
	} else {
		output << "<stat type=\"processes\" interval=\"0\" id=\"0\"></stat>";
	}

	
	return output.str();
}

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

#include "StatsBattery.h"

using namespace std;

#if defined(USE_BATTERY_APM)

#ifndef APM_BATT_ABSENT
#define APM_BATT_ABSENT APM_BATTERY_ABSENT
#endif

void StatsBattery::init()
{
	_init();

	int fd = open("/dev/apm", O_RDONLY);
	if(fd == -1)
		return;
	createBattery("apm", "", "");
}

void StatsBattery::update(long long sampleID)
{
	if(_items.size() == 0)
		return;

	int fd = open("/dev/apm", O_RDONLY);
	if(fd == -1)
		return;

	int state = BatteryStateUnknown;
	int source = BatterySourceAC;
	int present = 1;
	int percentage = -1;
	float t = 0;

    struct apm_power_info info;
    if (ioctl(fd, APM_IOC_GETPOWER, &info) == 0)
    {
    	if(info.battery_life == 255)
    		percentage = 100;
    	else
    		percentage = info.battery_life;

    	if(info.battery_state == APM_BATT_CHARGING)
    	{
			if(percentage == 100)
				state = BatteryStateCharged;
			else
				state = BatteryStateCharging;						
    	}
    	else if(info.battery_state == APM_BATT_UNKNOWN || info.battery_state == APM_BATT_ABSENT)
    	{

    	}
    	else
    	{
    		state = BatteryStateDraining;
    	}

    	if(info.ac_state == APM_AC_ON)
    	{
			if(percentage == 100)
				state = BatteryStateCharged;
			else
				state = BatteryStateCharging;						
    		source = BatterySourceAC;
    	}
    	else
    		source = BatterySourceBattery;

    	if(info.minutes_left > 0 && info.minutes_left < 0xFFFF)
	    	t = info.minutes_left;

		int timeRemaining = t;

		for (vector<battery_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			(*cur).state = state;
			(*cur).source = source;
			(*cur).present = present;
			(*cur).percentage = percentage;
			(*cur).timeRemaining = timeRemaining;
		}
    }
	close(fd);
}

#elif defined(USE_BATTERY_ACPI) && defined(HAVE_SYSCTLBYNAME)

#ifndef UNKNOWN_CAP
#define UNKNOWN_CAP 0xffffffff
#endif

#ifndef UNKNOWN_VOLTAGE
#define UNKNOWN_VOLTAGE 0xffffffff
#endif

void StatsBattery::init()
{
	_init();

	int batteries;
	size_t len = sizeof(batteries);

	if (sysctlbyname("hw.acpi.battery.units", &batteries, &len, NULL, 0) >= 0)
	{
		int x;
		for(x=0;x<batteries;x++)
		{
			stringstream key;
			key << x;
			createBattery(key.str(), "", "");
		}
	}
}

void StatsBattery::update(long long sampleID)
{
	if(_items.size() == 0)
		return;

	int acpifd = open("/dev/acpi", O_RDONLY);

	for (vector<battery_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
	{
		union acpi_battery_ioctl_arg battio;
		battio.unit = atoi((*cur).key.c_str());
		if (ioctl(acpifd, ACPIIO_BATT_GET_BIF, &battio) == -1)
		{
			(*cur).active = false;
			continue;
		}
	
		int state = BatteryStateUnknown;
		int source = BatterySourceAC;
		int present = 1;
		int cycles = 0;
		int percentage = -1;
		int capacityDesign = 0;
		int capacityFull = 0;
		int capacityNow = 0;
		int capacityRate = 0;
		int voltage = 0;
		float health = 0;
		float t = 0;

		if (battio.bif.dcap != UNKNOWN_CAP)
			capacityDesign = battio.bif.dcap;

		if (battio.bif.lfcap != UNKNOWN_CAP)
			capacityFull = battio.bif.lfcap;


		battio.unit = atoi((*cur).key.c_str());
		if (ioctl(acpifd, ACPIIO_BATT_GET_BATTINFO, &battio) == -1)
		{
			(*cur).active = false;
			continue;			
		}

		(*cur).active = true;

		int acline = 0;
		ioctl(acpifd, ACPIIO_ACAD_GET_STATUS, &acline);

		if (battio.battinfo.state != ACPI_BATT_STAT_NOT_PRESENT)
		{
			if (battio.battinfo.cap != -1)
				percentage = battio.battinfo.cap;

			if(acline == 1)
			{
				source = BatterySourceAC;

				if (battio.battinfo.state & ACPI_BATT_STAT_CHARGING)
					state = BatteryStateCharging;
				else
				{
					if(percentage == 100)
						state = BatteryStateCharged;
					else
						state = BatteryStateCharging;						
				}
			}
			else
			{
				source = BatterySourceBattery;
				state = BatteryStateDraining;		
			}

			if (battio.battinfo.min != -1)
				t = battio.battinfo.min;
		}

		battio.unit = atoi((*cur).key.c_str());
		if (ioctl(acpifd, ACPIIO_BATT_GET_BST, &battio) != -1)
		{
			if (battio.bst.state != ACPI_BATT_STAT_NOT_PRESENT) {
				if (battio.bst.volt != UNKNOWN_VOLTAGE)
					voltage = battio.bst.volt;
			}
		}

/*
		if(percentage == -1)
		{
			if(capacityFull > 0 && capacityNow > 0)
				percentage = (capacityNow / capacityFull) * 100;
			else
				percentage = 0;
		}*/

		if(capacityDesign > 0 && capacityFull > 0)
			health = ((float)capacityFull / (float)capacityDesign) * 100;

		int timeRemaining = t;

		(*cur).state = state;
		(*cur).source = source;
		(*cur).present = present;
		(*cur).cycles = cycles;
		(*cur).percentage = percentage;
		(*cur).capacityDesign = capacityDesign;
		(*cur).capacityFull = capacityFull;
		(*cur).capacityNow = capacityNow;
		(*cur).capacityRate = capacityRate;
		(*cur).voltage = voltage;
		(*cur).timeRemaining = timeRemaining;
		(*cur).health = health;
	}

	close(acpifd);
}

#elif defined(USE_BATTERY_PROCFS)

bool StatsBattery::fileExists(string path)
{
	FILE * fp = NULL;
	
	if (!(fp = fopen(path.c_str(), "r")))
	{
		return false;
	}
	
	fclose(fp);
	return true;
}

void StatsBattery::init()
{
	_init();

	DIR *dpdf;
	struct dirent *epdf;

	vector<string> adapters;
	vector<string> batteries;
	string directory = string("/sys/class/power_supply/");

	dpdf = opendir(directory.c_str());
	if (dpdf != NULL){
   		while ((epdf = readdir(dpdf))){
   			string file = string(epdf->d_name);
   			if(file.find("A") == 0)
   			{
   				string path = directory;
   				adapters.push_back(path.append(string(epdf->d_name)));
   			}
   			if(file.find("BAT") == 0)
   			{
   				string path = directory;
   				batteries.push_back(path.append(string(epdf->d_name)));
   			}
   		}
	}

	if(batteries.size() > 0)
	{
		for (vector<string>::iterator curbat = batteries.begin(); curbat != batteries.end(); ++curbat)
		{
			string uevent = (*curbat);
			uevent = uevent.append("/uevent");
			if(!fileExists(uevent))
				continue;

		   	string batterysuffix = (*curbat).substr(3, (*curbat).length() - 3);

		   	string adapterpath = string("");
			if(batterysuffix.length() > 0 && adapters.size() > 0)
			{
				for (vector<string>::iterator curadapter = adapters.begin(); curadapter != adapters.end(); ++curadapter)
				{
				   	string adaptersuffix = (*curadapter).substr((*curadapter).length() - batterysuffix.length(), batterysuffix.length());
				   	if(batterysuffix == adaptersuffix)
				   	{
				   		adapterpath = (*curadapter);
				   		break;
				   	}
				}
			}

			createBattery((*curbat), uevent, adapterpath);
		}
	}
	closedir(dpdf);
}

void StatsBattery::update(long long sampleID)
{
	if(_items.size() == 0)
		return;

	for (vector<battery_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
	{
		FILE * fp = NULL;
		char buf[512];

		if (!(fp = fopen((*cur).path.c_str(), "r")))
		{
			(*cur).present = 0;
			continue;
		}

		(*cur).active = true;
	
		int state = BatteryStateUnknown;
		int source = BatterySourceAC;
		int present = 1;
		int cycles = 0;
		int percentage = -1;
		int capacityDesign = 0;
		int capacityFull = 0;
		int capacityNow = 0;
		int capacityRate = 0;
		int voltage = 0;
		float health = 0;

		char charbuf[128];

		while (fgets(buf, sizeof(buf), fp))
		{
			sscanf(buf, "POWER_SUPPLY_PRESENT=%d", &present);
			sscanf(buf, "POWER_SUPPLY_CYCLE_COUNT=%d", &cycles);
			sscanf(buf, "POWER_SUPPLY_CAPACITY=%d", &percentage);
			sscanf(buf, "POWER_SUPPLY_ENERGY_FULL_DESIGN=%d", &capacityDesign);
			sscanf(buf, "POWER_SUPPLY_ENERGY_FULL=%d", &capacityFull);
			sscanf(buf, "POWER_SUPPLY_ENERGY_NOW=%d", &capacityNow);
			sscanf(buf, "POWER_SUPPLY_POWER_NOW=%d", &capacityRate);
			sscanf(buf, "POWER_SUPPLY_VOLTAGE_NOW=%d", &voltage);

			if(sscanf(buf, "POWER_SUPPLY_STATUS=%s", charbuf) == 1)
			{
				string status = string(charbuf);
				if(status.find("Charging") == 0)
				{
					source = BatterySourceAC;
					state = BatteryStateCharging;
				}
				else if(status.find("Discharging") == 0)
				{
					source = BatterySourceBattery;
					state = BatteryStateDraining;
				}
				else if(status.find("Charged") == 0)
				{
					source = BatterySourceAC;
					state = BatteryStateCharged;
				}
				else 
				{
					source = BatterySourceAC;
					state = BatteryStateCharged;
				}
			}
		}


		if(percentage == -1)
		{
			if(capacityFull > 0 && capacityNow > 0)
				percentage = (capacityNow / capacityFull) * 100;
			else
				percentage = 0;
		}

		if(percentage > 100)
			percentage = 100;

		float t = 0;
		if(state == BatteryStateCharging)
		{
			int remainingUntilFull = capacityFull - capacityNow;
			t = 0;
			if(remainingUntilFull > 0 && capacityRate > 0)
				t = ((float)remainingUntilFull / capacityRate);			
		}
		else if(state == BatteryStateDraining)
		{
			t = 0;
			if(capacityNow > 0 && capacityRate > 0)
				t = ((float)capacityNow / capacityRate);						
		}

		int timeRemaining = t * 60;

		if(capacityDesign > 0 && capacityFull > 0)
			health = ((float)capacityFull / (float)capacityDesign) * 100;

		(*cur).state = state;
		(*cur).source = source;
		(*cur).present = present;
		(*cur).cycles = cycles;
		(*cur).percentage = percentage;
		(*cur).capacityDesign = capacityDesign;
		(*cur).capacityFull = capacityFull;
		(*cur).capacityNow = capacityNow;
		(*cur).capacityRate = capacityRate;
		(*cur).voltage = voltage;
		(*cur).timeRemaining = timeRemaining;
		(*cur).health = health;

		fclose(fp);
	}
}

#else

void StatsBattery::init()
{
	_init();
}

void StatsBattery::update(long long sampleID)
{
}

#endif

void StatsBattery::_init()
{	
	initShared();
	ready = 1;
}

void StatsBattery::prepareUpdate()
{
	StatsBase::prepareUpdate();

	if(_items.size() > 0)
	{
		for (vector<battery_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			(*cur).active = false;
		}
	}
}

void StatsBattery::createBattery(string key, string batterypath, string adapterpath)
{
	if(_items.size() > 0)
	{
		for (vector<battery_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			battery_info interface = *cur;
			if (interface.key == key)
			{
				return;
			}
		}
	}

	battery_info battery;
	battery.key = key;
	battery.path = batterypath;
	battery.adapterpath = adapterpath;

	_items.insert(_items.begin(), battery);	
}

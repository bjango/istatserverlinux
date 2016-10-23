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

#ifndef _STATSBATTERY_H
#define _STATSBATTERY_H

#define BatteryStateUnknown -1
#define BatteryStateCharged 0
#define BatteryStateCharging 1
#define BatteryStateDraining 2
#define BatteryStateNotCharging 3
#define BatteryStateNA 4

#define BatterySourceBattery 0
#define BatterySourceAC 1

class battery_info
{
	public:
		bool active;
		long long sampleID;
		std::string key;
		std::string path;	
		std::string adapterpath;	

		int state;
		int source;
		int present;
		int cycles;
		int percentage;
		int capacityDesign;
		int capacityFull;
		int capacityNow;
		int capacityRate;
		int voltage;
		int timeRemaining;
		float health;
//		std::vector<net_data> history;
};

class StatsBattery : public StatsBase
{
	public:
		void update(long long sampleID);
		void init();
		void _init();
		bool fileExists(std::string path);
		std::vector<battery_info> _items;
		void createBattery(std::string key, std::string batterypath, std::string adapterpath);
		void prepareUpdate();
	};
#endif

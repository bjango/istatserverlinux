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

#ifndef _RESPONSES_H
#define _RESPONSES_H

#include "config.h"
#include <vector>
#include <deque>
#include <algorithm>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "Stats.h"
#include "System.h"

std::string isr_create_header();
std::string isr_accept_code();
std::string isr_reject_code();
std::string isr_accept_connection();
std::string isr_serverinfo(int session, int auth, std::string uuid, bool historyEnabled);

bool shouldAddKey(int index, std::string key, std::vector<std::string> keys, std::vector<std::string> *added);
std::string keyForIndex(std::string uuid, int index);

std::string isr_multiple_data(xmlNodePtr node, Stats *stats);
std::string isr_cpu_data(xmlNodePtr node, Stats *stats);
std::string isr_network_data(int index, long sampleID, StatsNetwork stats, std::vector<std::string> keys, std::vector<std::string> *added);
std::string isr_disk_data(int index, long sampleID, StatsDisks stats, std::vector<std::string> keys, std::vector<std::string> *added);
std::string isr_uptime_data(long uptime);
std::string isr_loadavg_data(xmlNodePtr node, Stats *stats);
std::string isr_memory_data(xmlNodePtr node, Stats *stats);
std::string isr_fan_data(std::vector<sensor_info> *_data, long _init);
std::string isr_temp_data(std::vector<sensor_info> *_data, long _init);
std::string isr_sensor_data(int index, long sampleID, StatsSensors stats, std::vector<std::string> keys, std::vector<std::string> *added);
std::string isr_activity_data(int index, long sampleID, StatsActivity stats, std::vector<std::string> keys, std::vector<std::string> *added);
std::string isr_battery_data(int index, long sampleID, StatsBattery stats, std::vector<std::string> keys, std::vector<std::string> *added);
std::string isr_process_data(int index, long sampleID, StatsProcesses stats, std::vector<std::string> keys, std::vector<std::string> *added);

#endif

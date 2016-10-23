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

#ifndef _STATSNETWORK_H
#define _STATSNETWORK_H

class network_info
{
	public:
		bool active;
		unsigned int id;
		unsigned long long last_up;
		unsigned long long last_down;
		
		std::vector<std::string> addresses;
		std::string device;
		double last_update;
		
		std::deque<net_data> samples[8];
};

class StatsNetwork : public StatsBase
{
	public:
		void update(long long sampleID);
		void init();
		std::vector<network_info> _items;
		void createInterface(std::string key);
		void processInterface(std::string key, long long sampleID, unsigned long long upload, unsigned long long download);
		void updateAddresses();
		void prepareUpdate();
		#ifdef HAVE_LIBKSTAT
		kstat_ctl_t *ksh;
		std::vector<std::string> interfaces();
		#endif

		void _init();
		#ifdef USE_SQLITE
		void updateHistory();
		net_data historyItemAtIndex(int index, network_info item);
		void loadPreviousSamples();
		void loadPreviousSamplesAtIndex(int index);
		#endif

	   	std::deque<sample_data> samples[8];

		#ifdef USE_NET_SYSCTL
		int get_ifcount();
		#endif
	};
#endif

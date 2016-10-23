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

#ifndef _AIXVERSION_610
int getprocs64 (struct procentry64 *procsinfo, int sizproc, struct fdsinfo64 *fdsinfo, int sizfd, pid_t *index, int count);
#endif

#ifndef _STATSPROCESSES_H
#define _STATSPROCESSES_H

class process_info
{
	public:
		bool exists;
		bool is_new;
		double cpu;
		unsigned long long io_read;
		unsigned long long io_write;
		unsigned long long io_read_total;
		unsigned long long io_write_total;
		unsigned long memory;
		double cpuTime;
		double lastClockTime;
		long threads;
		int pid;
		long long sampleID;
		char name[128];
};
class StatsProcesses
{
	public:
		void update(long long sampleID, double ticks);
		void prepareUpdate();
		void init();
		std::vector<process_info> _items;
		void createProcess(int pid);
		void processProcess(int pid, long long sampleID);

		long threadCount;
		long processCount;
		double aixEntitlement;

		#ifdef USE_PROCESSES_PROCFS
		std::string nameFromCmd(int pid, std::string name);
		std::string nameFromStatus(int pid);
		std::vector<std::string> componentsFromString(std::string input, char seperator);
		#endif

		void finishUpdate();
	};
#endif

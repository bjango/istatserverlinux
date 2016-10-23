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

#include "StatsProcesses.h"

using namespace std;


#ifdef USE_PROCESSES_AIX

void StatsProcesses::init()
{
	//aixEntitlement = 0.12;
}

void StatsProcesses::update(long long sampleID, double totalTicks)
{
	int treesize;
	pid_t firstproc = 0;
	if ((treesize = getprocs64(NULL, 0, NULL, 0, &firstproc, PID_MAX)) < 0) {
		return;
	}
	struct procentry64 *procs = (struct procentry64 *)malloc(treesize * sizeof (struct procentry64));;

	firstproc = 0;
	if ((treesize = getprocs64(procs, sizeof(struct procentry64), NULL, 0, &firstproc, treesize)) < 0) {
		free(procs);
		return;
	}

	threadCount = 0;

	double currentTime = get_current_time();

	for (int i = 0; i < treesize; i++)
	{
		pid_t pid = procs[i].pi_pid;
		if(pid == 0)
			continue;

		processProcess(pid, sampleID);

		for (vector<process_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			if((*cur).pid == pid)
			{
				struct psinfo psinfo;
				char buffer [BUFSIZ];

				sprintf (buffer, "/proc/%d/psinfo", (int) pid);

				int fd = open(buffer, O_RDONLY);

				if (fd < 0) {
					continue;
				}

				ssize_t len = pread(fd, &psinfo, sizeof (struct psinfo), 0);
				if (len != sizeof (struct psinfo))
				{
					close(fd);
					continue;
				}

				(*cur).exists = true;
				if((*cur).is_new == true)
				{
					sprintf((*cur).name, psinfo.pr_fname);
					(*cur).is_new = false;
				}

				double t = (double)(procs[i].pi_ru.ru_utime.tv_sec + procs[i].pi_ru.ru_stime.tv_sec);
				t += (double)(procs[i].pi_ru.ru_utime.tv_usec + procs[i].pi_ru.ru_stime.tv_usec) / NS_PER_SEC;
				
				if((*cur).cpuTime == 0)
				{
					(*cur).cpuTime = t;
					(*cur).lastClockTime = currentTime;
				}

				double timediff = currentTime - (*cur).lastClockTime;

				(*cur).cpu = ((t - (*cur).cpuTime) / timediff * 10000) / (aixEntitlement * 100);
				(*cur).threads = procs[i].pi_thcount;
				(*cur).memory = (uint64_t)(procs[i].pi_drss + procs[i].pi_trss) * (uint64_t)getpagesize();
				threadCount += procs[i].pi_thcount;

				(*cur).cpuTime = t;
				(*cur).lastClockTime = currentTime;

				close(fd);
			}
		}
	}

	free(procs);
}

#elif defined(USE_PROCESSES_PSINFO)

void StatsProcesses::init()
{
}

void StatsProcesses::update(long long sampleID, double totalTicks)
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir("/proc")))
	{
		return;
	}

	while ((entry = readdir(dir)))
	{
		int pid = 0;

		if (!entry)
		{
			closedir(dir);
			return;
		}

		if (sscanf(entry->d_name, "%d", &pid) > 0)
		{
			processProcess(pid, sampleID);

			for (vector<process_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
			{
				if((*cur).pid == pid)
				{
					struct psinfo psinfo;
					char buffer [BUFSIZ];

					sprintf (buffer, "/proc/%d/psinfo", (int) pid);

   					int fd = open(buffer, O_RDONLY);

					if (fd < 0) {
						continue;
					}

   					ssize_t len = pread(fd, &psinfo, sizeof (struct psinfo), 0);
					if (len != sizeof (struct psinfo))
					{
						close(fd);
						continue;
					}

					(*cur).exists = true;
					if((*cur).is_new == true)
					{
						sprintf((*cur).name, psinfo.pr_fname);
						(*cur).is_new = false;
					}

					(*cur).cpu = (double)(psinfo.pr_pctcpu * 100.0f) / 0x8000;
					(*cur).memory = psinfo.pr_rssize * 1024;

					close(fd);
				}
			}
		}
	}

	closedir(dir);
}

#elif defined(USE_PROCESSES_KVM)

void StatsProcesses::init()
{

}
void StatsProcesses::update(long long sampleID, double totalTicks)
{
	#if defined(PROCESSES_KVM_NETBSD)
	struct kinfo_proc2 *p;
	#else
	struct kinfo_proc *p;
	#endif
	int n_processes;
	int i;
	kvm_t *kd;

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, NULL)) == NULL)
	{
		return;
	}

	#if defined(PROCESSES_KVM_NETBSD)
	p = kvm_getproc2(kd, KERN_PROC_ALL, 0, sizeof(kinfo_proc2), &n_processes);
	#elif defined(PROCESSES_KVM_OPENBSD)
	p = kvm_getprocs(kd, KERN_PROC_ALL, 0, sizeof(kinfo_proc), &n_processes);
	#elif defined(PROCESSES_KVM_DRAGONFLY)
	p = kvm_getprocs(kd, KERN_PROC_ALL, sizeof(kinfo_proc), &n_processes);
	#else
	p = kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);
	#endif

	for (i = 0; i < n_processes; i++) {
		#if defined(PROCESSES_KVM_DRAGONFLY)
		if (!((p[i].kp_flags & P_SYSTEM)) && p[i].kp_comm != NULL) {
		#elif defined(PROCESSES_KVM_OPENBSD) || defined(PROCESSES_KVM_NETBSD)
		if (!((p[i].p_flag & P_SYSTEM)) && p[i].p_comm != NULL) {
		#else
		if (!((p[i].ki_flag & P_SYSTEM)) && p[i].ki_comm != NULL) {
		#endif

		#if defined(PROCESSES_KVM_DRAGONFLY)
			int pid = p[i].kp_pid;
			struct kinfo_lwp lwp = p[i].kp_lwp;
		#elif defined(PROCESSES_KVM_OPENBSD) || defined(PROCESSES_KVM_NETBSD)
			int pid = p[i].p_pid;
		#else
			int pid = p[i].ki_pid;
		#endif
			processProcess(pid, sampleID);

			for (vector<process_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
			{
				if((*cur).pid == pid)
				{
					(*cur).exists = true;
					if((*cur).is_new == true)
					{
						#if defined(PROCESSES_KVM_DRAGONFLY)
						sprintf((*cur).name, "%s", p[i].kp_comm);
						#elif defined(PROCESSES_KVM_OPENBSD) || defined(PROCESSES_KVM_NETBSD)
						sprintf((*cur).name, "%s", p[i].p_comm);
						#else
						sprintf((*cur).name, "%s", p[i].ki_comm);
						#endif
						(*cur).is_new = false;
					}

					#if defined(PROCESSES_KVM_DRAGONFLY)
					(*cur).memory = (p[i].kp_vm_rssize * getpagesize());
					(*cur).cpu = (double)(100.0 * lwp.kl_pctcpu / FSCALE);
					#elif defined(PROCESSES_KVM_OPENBSD) || defined(PROCESSES_KVM_NETBSD)
					(*cur).memory = (p[i].p_vm_rssize * getpagesize());
					(*cur).cpu = (double)(100.0 * p[i].p_pctcpu / FSCALE);
					#else
					(*cur).memory = (p[i].ki_rssize * getpagesize());
					(*cur).cpu = (double)(100.0 * p[i].ki_pctcpu / FSCALE);
					#endif
				}
			}
		}
	}

	kvm_close(kd);
}

#elif defined(USE_PROCESSES_PROCFS)

void StatsProcesses::init()
{

}

vector<string> StatsProcesses::componentsFromString(string input, char seperator)
{
	vector<string> components;
	stringstream ss(input);
	string tok;
  
	while(getline(ss, tok, seperator))
	{
		components.push_back(tok);
	}
	return components;
}

string StatsProcesses::nameFromCmd(int pid, string name)
{
	stringstream tmp;
	tmp << "/proc/" << pid << "/cmdline";

	int fd = open(tmp.str().c_str(), O_RDONLY);
	if (fd > 0) {
		char buffer[1024];
		ssize_t len;
		if ((len = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
			buffer[len] = '\0';
					
			int i;
			for (i = 0; i < len; i++) {
				if (buffer[i] == '\0')
					buffer[i] = ' ';
			}
		}
		close(fd);

		if(len == 0)
			return name;

		string cmd = string(buffer);

		vector<string> components = componentsFromString(cmd, ' ');
		if(components.size() > 0)
		{
			for (vector<string>::iterator cur = components.begin(); cur != components.end(); ++cur)
			{
				if((*cur).find(name) != std::string::npos)
				{
					vector<string> componentsn = componentsFromString((*cur), '/');
					if(componentsn.size() > 0)
						return componentsn.back();
					return (*cur);
				}
			}
		}
	}
	return name;
}

string StatsProcesses::nameFromStatus(int pid)
{

	stringstream tmp;
	tmp << "/proc/" << pid << "/status";

	FILE * fp = NULL;
	
	if ((fp = fopen(tmp.str().c_str(), "r")))
	{
		char buf[32];
		while (fgets(buf, sizeof(buf), fp) != NULL)
		{
			if(sscanf(buf, "Name:	 %s", buf) > 0)
			{
				fclose(fp);
				return string(buf);
			}
		}
		fclose(fp);
	}
	return "";
}

void StatsProcesses::update(long long sampleID, double totalTicks)
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir("/proc")))
	{
		return;
	}

	while ((entry = readdir(dir)))
	{
		int pid = 0;

		if (!entry)
		{
			break;
		}

		if (sscanf(entry->d_name, "%d", &pid) > 0)
		{
			processProcess(pid, sampleID);

			for (vector<process_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
			{
				if((*cur).pid == pid)
				{
					(*cur).exists = true;

					if((*cur).is_new == true)
					{
						string name = nameFromStatus(pid);
						if(name.length() == 15)
						{
							name = nameFromCmd(pid, name);
						}
						sprintf((*cur).name, "%s", name.c_str());
						(*cur).is_new = false;
					}

					{
						stringstream tmp;
						tmp << "/proc/" << pid << "/stat";

						FILE * fp = NULL;
	
						if ((fp = fopen(tmp.str().c_str(), "r")))
						{
							unsigned long userTime = 0;
							unsigned long systemTime = 0;
							unsigned long rss = 0;
							long threads = 0;
							char name[255];

							if(fscanf(fp, "%*d %s %*c %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %lu %*s %*s %*s %*s %ld %*s %*s %*s %lu", name, &userTime, &systemTime, &threads, &rss) > 0)
							{
								if((*cur).cpuTime == 0)
								{
									(*cur).cpuTime = (double)(userTime + systemTime);
									(*cur).lastClockTime = get_current_time();
								}

								double cpuTime = (double)(userTime + (double)systemTime) - (*cur).cpuTime;
								double clockTimeDifference = get_current_time() - (*cur).lastClockTime;

								if(clockTimeDifference > 0)
								{
									double procs = (double)sysconf(_SC_NPROCESSORS_ONLN);
									if(procs < 1)
										procs = 1;
									(*cur).cpu = (((cpuTime / (double)sysconf(_SC_CLK_TCK)) / clockTimeDifference) * 100) / procs;
								}
								else
								{
									(*cur).cpu = 0;
								}

								threadCount += threads;
								(*cur).threads = threads;
								(*cur).memory = rss * getpagesize();
								(*cur).cpuTime = (double)(userTime + systemTime);
								(*cur).lastClockTime = get_current_time();
							}
							fclose(fp);
						}
					}
					
					// /proc/pid/io requires root access which we usually dont run with
					/*
					{
						stringstream tmp;
						tmp << "/proc/" << pid << "/io";

						FILE * fp = NULL;
	
						if ((fp = fopen(tmp.str().c_str(), "r")))
						{
							char buf[1024];
							unsigned long long totalRead = 0;
							unsigned long long totalWrite = 0;
							while (fgets(buf, sizeof(buf), fp))
							{
								sscanf(buf, "read_bytes: %llu", &totalRead);
								sscanf(buf, "write_bytes: %llu", &totalWrite);
							}

							if((*cur).io_read_total == 0)
							{
								(*cur).io_read_total = totalRead;
							}

							if((*cur).io_write_total == 0)
							{
								(*cur).io_write_total = totalWrite;
							}

							(*cur).io_read = totalRead - (*cur).io_read_total;
							(*cur).io_write = totalWrite - (*cur).io_write_total;

							(*cur).io_read_total = totalRead;
							(*cur).io_write_total = totalWrite;

							fclose(fp);
						}
					}*/
				}
			}
		}
	}

	closedir(dir);
}

#else

void StatsProcesses::init()
{

}

void StatsProcesses::update(long long sampleID, double totalTicks)
{

}

#endif

void StatsProcesses::createProcess(int pid)
{
	if(_items.size() > 0)
	{
		for (vector<process_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
				process_info process = *cur;
				if(process.pid == pid){
					return;
				}
		}
	}

	process_info process;
	process.cpu = 0;
	process.memory = 0;
	process.is_new = true;
	process.pid = pid;
	process.cpuTime = 0;
	process.io_read = 0;
	process.io_write = 0;
	process.exists = true;
	_items.insert(_items.begin(), process);	
}

void StatsProcesses::processProcess(int pid, long long sampleID)
{
	processCount++;
	createProcess(pid);
}

void StatsProcesses::prepareUpdate()
{
	threadCount = 0;
	processCount = 0;

	if(_items.size() > 0)
	{
		for (vector<process_info>::iterator cur = _items.begin(); cur != _items.end(); ++cur)
		{
			(*cur).exists = false;
		}
	}
}

void StatsProcesses::finishUpdate()
{
	if(_items.size() > 0)
	{
		for (vector<process_info>::iterator i = _items.begin(); i != _items.end(); ) {
  			if (i->exists == false) {
    			i = _items.erase(i);
  			} else {
    			++i;
			}
		}
	}
}

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

#ifndef _STATBASE_H
#define _STATBASE_H

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include <vector>
#include <limits.h>
#include <iostream>
#include <sstream>
#include <deque>
#include <algorithm>

#include "config.h"
#include "System.h"
#include "Utility.h"

// needed for solaris
#define _STRUCTURED_PROC 1

#ifdef HAVE_SENSORS_SENSORS_H
# include <sensors/sensors.h>
#endif

#ifdef HAVE_SYS_PROCFS_H
#include <sys/procfs.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_PROCFS_H
#include <procfs.h>
#endif

#ifdef HAVE_PROCINFO_H
#include <procinfo.h>
#endif

#ifdef HAVE_KVM_H
# include <kvm.h>
#endif

#ifdef HAVE_SYS_USER_H
#include <sys/user.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif

#ifdef HAVE_NET_IF_MIB_H
# include <net/if_mib.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_GETIFADDRS
	#include <ifaddrs.h>
	#include <arpa/inet.h>

	#ifndef AF_LINK
	#define AF_LINK AF_PACKET
	#endif
#endif

#ifdef HAVE_INET_COMMON_H
#include <inet/common.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#ifdef HAVE_SYS_SWAP_H
# include <sys/swap.h>
#endif

#ifdef HAVE_UVM_UVM_EXTERN_H
# include <uvm/uvm_extern.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#elif defined(HAVE_SYS_TIME_H)
# include <sys/time.h>
#else
# include <time.h>
#endif

#ifdef HAVE_SYS_VMMETER_H
# include <sys/vmmeter.h>
#endif

#ifdef HAVE_SYS_MNTTAB_H
# include <sys/mnttab.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#elif defined(HAVE_SYS_STATFS_H)
# include <sys/statfs.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif

#ifdef HAVE_MNTENT_H
# include <mntent.h>
#endif

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif

#ifdef HAVE_SYS_SCHED_H
# include <sys/sched.h>
#endif

#ifdef HAVE_SYS_PROCESSOR_H
# include <sys/processor.h>
#endif

#ifdef HAVE_SYS_SYSINFO_H
# include <sys/sysinfo.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_MACHINE_APMVAR_H
#include <machine/apmvar.h>
#endif

#ifdef HAVE_DEV_ACPICA_ACPIIO_H
#include <dev/acpica/acpiio.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif

#if defined(HAVE_DEVSTAT) || defined(HAVE_DEVSTAT_ALT)
#include <devstat.h>
#endif

#ifdef HAVE_KSTAT_H
# include <kstat.h>
#endif

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif

#ifdef HAVE_SYS_LOADAVG_H
# include <sys/loadavg.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_PROC_H
# include <sys/proc.h>
#endif

#ifdef HAVE_LIBPERFSTAT_H
# include <libperfstat.h>
#endif

#ifdef HAVE_SYS_PSTAT_H
# include <sys/pstat.h>
#endif

#ifdef HAVE_SYS_DK_H
# include <sys/dk.h>
#endif

#ifdef HAVE_SYS_DLPI_H
# include <sys/dlpi.h>
#endif

#ifdef HAVE_SYS_DLPI_EXT_H
# include <sys/dlpi_ext.h>
#endif

#ifdef HAVE_SYS_MIB_H
# include <sys/mib.h>
#endif

#ifdef HAVE_SYS_STROPTS_H
# include <sys/stropts.h>
#endif

#ifdef HAVE_SYS_DISK_H
# include <sys/disk.h>
#endif

#ifdef HAVE_SYS_DKSTAT_H
# include <sys/dkstat.h>
#endif

#ifdef USE_SQLITE
#include "Database.h"
#endif

class StatsBase
{
	typedef struct sampleindexconfig {
		long long sampleID;
    	double time;
    	double nextTime;
    	double interval;
    	int historyIndex;
	} sampleindexconfig_t;

	public:
		void prepareUpdate();
		void tick();
		void tickSample();
		struct sampleindexconfig sampleIndex[8];
		void initShared();
		int ready;
		long session;
		bool historyEnabled;
		bool debugLogging;

		#ifdef USE_SQLITE
		std::vector<DatabaseItem> databaseQueue;
		Database _database;
		int databaseType;
		std::string databasePrefix;
		void fillGaps();
		void fillGapsAtIndex(int index);
		std::string tableAtIndex(int index);
		int InsertInitialSample(int index, double time, long long sampleID);
		double sampleIdForTable(std::string table);
		void loadPreviousSamples();
		void removeOldSamples();
		#endif
};
#endif

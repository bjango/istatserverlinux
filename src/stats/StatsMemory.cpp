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

#include "StatsMemory.h"

using namespace std;

#if defined(USE_MEM_HPUX)

void StatsMemory::init()
{
	databaseKeys.push_back("total");
	databaseKeys.push_back("free");
	databaseKeys.push_back("used");
	databaseKeys.push_back("swaptotal");
	databaseKeys.push_back("swapused");

	databaseMap.push_back(memory_value_total);
	databaseMap.push_back(memory_value_free);
	databaseMap.push_back(memory_value_used);
	databaseMap.push_back(memory_value_swaptotal);
	databaseMap.push_back(memory_value_swapused);

	_init();
}

void StatsMemory::update(long long sampleID)
{
	mem_data _mem;
	prepareSample(&_mem);

	struct pst_static pstat_static;
	struct pst_dynamic pstat_dynamic;
	struct pst_vminfo vminfo;

	if( pstat_getstatic(&pstat_static, sizeof(pstat_static), 1, 0) == -1 ) {
		return;
	}

	if (pstat_getdynamic(&pstat_dynamic, sizeof(pstat_dynamic), 1, 0) == -1) {
		return;
	}

	if(pstat_getvminfo( &vminfo, sizeof(vminfo), 1, 0 ) == -1 ) {
		return;
	}

	_mem.values[memory_value_total] = (double)(pstat_static.physical_memory * pstat_static.page_size);
	_mem.values[memory_value_free] = (double)(pstat_dynamic.psd_free * pstat_static.page_size);
	_mem.values[memory_value_used] = (double)(_mem.values[memory_value_total] - _mem.values[memory_value_free]);


    struct pst_swapinfo swap;
    int i = 0;

    while (pstat_getswap(&pss, sizeof(pss), (size_t) 1, i) != -1) {
		i++;

		if ((swap.pss_flags & SW_ENABLED) != SW_ENABLED)
			continue;

		if ((swap.pss_flags & SW_BLOCK) == SW_BLOCK) {
			_mem.values[memory_value_swaptotal] += (double)(swap.pss_nblksavail * 1024);
			_mem.values[memory_value_swapused] += (double)(swap.pss_nfpgs * 1024);
		}

		if ((swap.pss_flags & SW_FS) == SW_FS) {
			_mem.values[memory_value_swaptotal] += (double)(swap.pss_limit * 1024);
			_mem.values[memory_value_swapused] += (double)(swap.pss_allocated * 1024);
		}
    }

	//vminfo.psv_spgin;
	//vminfo.psv_spgout;


	addSample(_mem, sampleID);
}

#elif defined(HAVE_SYSCTLBYNAME) && (defined(USE_MEM_SYSCTLBYNAME) || (defined(__FreeBSD__) && __FreeBSD__ >= 11))

void StatsMemory::init()
{
	databaseKeys.push_back("total");
	databaseKeys.push_back("free");
	databaseKeys.push_back("active");
	databaseKeys.push_back("inactive");
	databaseKeys.push_back("wired");
	databaseKeys.push_back("cached");
	databaseKeys.push_back("used");
	databaseKeys.push_back("buffer");
	databaseKeys.push_back("swaptotal");
	databaseKeys.push_back("swapused");

	databaseKeys.push_back("swapin");
	databaseKeys.push_back("swapout");

	databaseMap.push_back(memory_value_total);
	databaseMap.push_back(memory_value_free);
	databaseMap.push_back(memory_value_active);
	databaseMap.push_back(memory_value_inactive);
	databaseMap.push_back(memory_value_wired);
	databaseMap.push_back(memory_value_cached);
	databaseMap.push_back(memory_value_used);
	databaseMap.push_back(memory_value_buffer);
	databaseMap.push_back(memory_value_swaptotal);
	databaseMap.push_back(memory_value_swapused);

	databaseMap.push_back(memory_value_swapin);
	databaseMap.push_back(memory_value_swapout);

	_init();
}

void StatsMemory::update(long long sampleID)
{
	mem_data _mem;
	prepareSample(&_mem);

	u_int _total, _inactive, _free, _active, _cached, _wired, _swaptotal, _swapused, _swapin, _swapout;
	long _buf;

	int pagesize = getpagesize();

	size_t len = sizeof(_total);

    if (sysctlbyname("vm.stats.vm.v_page_count", &_total, &len, NULL, 0) < 0)
    	_total = 0;

    if (sysctlbyname("vm.stats.vm.v_free_count", &_free, &len, NULL, 0) < 0)
    	_free = 0;

    if (sysctlbyname("vm.stats.vm.v_inactive_count", &_inactive, &len, NULL, 0) < 0)
    	_inactive = 0;

    if (sysctlbyname("vm.stats.vm.v_active_count", &_active, &len, NULL, 0) < 0)
    	_active = 0;

    if (sysctlbyname("vm.stats.vm.v_cache_count", &_cached, &len, NULL, 0) < 0)
    	_cached = 0;

    if (sysctlbyname("vm.stats.vm.v_wire_count", &_wired, &len, NULL, 0) < 0)
    	_wired = 0;

    if (sysctlbyname("vm.swap_size", &_swaptotal, &len, NULL, 0) < 0)
    	_swaptotal = 0;

    if (sysctlbyname("vm.swap_anon_use", &_swapused, &len, NULL, 0) < 0)
    	_swapused = 0;

    if (sysctlbyname("vm.stats.vm.v_swappgsin", &_swapin, &len, NULL, 0) < 0)
    	_swapin = 0;

    if (sysctlbyname("vm.stats.vm.v_swappgsout", &_swapout, &len, NULL, 0) < 0)
    	_swapout = 0;



	len = sizeof(_buf);
    if (sysctlbyname("vfs.bufspace", &_buf, &len, NULL, 0) < 0)
    	_buf = 0;

    if(_buf == 0)
    {
		size_t bufsizelen;
        long bufsize;
        bufsizelen = sizeof(bufsize);

        if (sysctlbyname("kern.nbuf", &bufsize, &bufsizelen, NULL, 0) >= 0)
        {
        	_buf = ((bufsize * 16) * 1024);
        }
    }

	_mem.values[memory_value_total] = (double)(_total * pagesize);
	_mem.values[memory_value_free] = (double)(_free * pagesize);
	_mem.values[memory_value_active] = (double)(_active * pagesize);
	_mem.values[memory_value_inactive] = (double)(_inactive * pagesize);
	_mem.values[memory_value_wired] = (double)(_wired * pagesize);
	_mem.values[memory_value_cached] = (double)(_cached * pagesize);
	_mem.values[memory_value_used] = (double)(_mem.values[memory_value_total] - (_mem.values[memory_value_free] + _mem.values[memory_value_inactive]));
	_mem.values[memory_value_buffer] = (double)(_buf);
	_mem.values[memory_value_swapused] = (double)(_swapused * pagesize);
	_mem.values[memory_value_swaptotal] = (double)(_swaptotal * pagesize);
	_mem.values[memory_value_swapin] = (double)(_swapin * pagesize);
	_mem.values[memory_value_swapout] = (double)(_swapout * pagesize);

  #if defined(HAVE_LIBKVM) && defined(__FreeBSD__)
	struct kvm_swap swap[1];
	{
		if (kvm_getswapinfo(kd, swap, 1, 0) != -1)
		{
			_mem.values[memory_value_swaptotal] = (double)((double)swap[0].ksw_total * pagesize);
			_mem.values[memory_value_swapused] = (double)((double)swap[0].ksw_used * pagesize);
		}
	}
  #endif


	addSample(_mem, sampleID);
}

#elif defined(USE_MEM_SYSCTL)

void StatsMemory::init()
{

	databaseKeys.push_back("total");
	databaseKeys.push_back("free");
	databaseKeys.push_back("used");
	databaseKeys.push_back("active");
	databaseKeys.push_back("inactive");
	databaseKeys.push_back("wired");
	databaseKeys.push_back("swaptotal");
	databaseKeys.push_back("swapused");

#ifdef VFS_BCACHESTAT
	databaseKeys.push_back("buffer");
#endif

#ifdef VM_UVMEXP2
	databaseKeys.push_back("file");
	databaseKeys.push_back("exec");
#endif

	databaseMap.push_back(memory_value_total);
	databaseMap.push_back(memory_value_free);
	databaseMap.push_back(memory_value_used);
	databaseMap.push_back(memory_value_active);
	databaseMap.push_back(memory_value_inactive);
	databaseMap.push_back(memory_value_wired);
	databaseMap.push_back(memory_value_swaptotal);
	databaseMap.push_back(memory_value_swapused);

#ifdef VFS_BCACHESTAT
	databaseMap.push_back(memory_value_buffer);
#endif

#ifdef VM_UVMEXP2
	databaseMap.push_back(memory_value_file);
	databaseMap.push_back(memory_value_ex);
#endif

	_init();
}

void StatsMemory::updateSwap(mem_data* _mem)
{
	struct swapent *swdev;
	int nswap, rnswap, i;

	nswap = swapctl(SWAP_NSWAP, 0, 0);
	if (nswap == 0)
		return;

	swdev = (struct swapent *)calloc(nswap, sizeof(*swdev));
	if (swdev == NULL)
		return;

	rnswap = swapctl(SWAP_STATS, swdev, nswap);
	if (rnswap == -1) {
		free(swdev);
		return;
	}

	/* if rnswap != nswap, then what? */
	/* Total things up */
	double total = 0;
	double used = 0;
	for (i = 0; i < nswap; i++) {
		if (swdev[i].se_flags & SWF_ENABLE) {
			used += (double)(swdev[i].se_inuse / (1024 / DEV_BSIZE));
			total += (double)(swdev[i].se_nblks / (1024 / DEV_BSIZE));
		}
	}

	_mem->values[memory_value_swapused] = used;
	_mem->values[memory_value_swaptotal] = total;

	free(swdev);
}

void StatsMemory::update(long long sampleID)
{
	mem_data _mem;
	prepareSample(&_mem);

#ifdef VM_UVMEXP2
	static int mib_vm[2] = { CTL_VM, VM_UVMEXP2 };
	struct uvmexp_sysctl uvmexp;
#else
	static int mib_vm[2] = { CTL_VM, VM_UVMEXP };
	struct uvmexp uvmexp;
#endif

	size_t size;
	int pagesize = getpagesize();

	size = sizeof(uvmexp);
	if (sysctl(mib_vm, 2, &uvmexp, &size, NULL, 0) < 0) {
		return;
	}

#ifdef VFS_BCACHESTAT
	static int mib_buf[3] = { CTL_VFS, VFS_GENERIC, VFS_BCACHESTAT };
	struct bcachestats bcstats;

	size = sizeof(bcstats);
	if (sysctl(mib_buf, 3, &bcstats, &size, NULL, 0) >= 0) {
		_mem.values[memory_value_buffer] = (double)(bcstats.numbufpages * pagesize);
	}
#endif

	_mem.values[memory_value_total] = (double)(uvmexp.npages * pagesize);
	_mem.values[memory_value_free] = (double)(uvmexp.free * pagesize);
	_mem.values[memory_value_used] = (double)(_mem.values[memory_value_total] - _mem.values[memory_value_free]);
	_mem.values[memory_value_active] = (double)(uvmexp.active * pagesize);
	_mem.values[memory_value_inactive] = (double)(uvmexp.inactive * pagesize);
	_mem.values[memory_value_wired] = (double)(uvmexp.wired * pagesize);

#ifdef VM_UVMEXP2
	_mem.values[memory_value_file] = (double)(uvmexp.filepages * pagesize);
	_mem.values[memory_value_ex] = (double)(uvmexp.execpages * pagesize);
#endif

	updateSwap(&_mem);

	addSample(_mem, sampleID);
}

#elif defined(HAVE_LIBKVM) && defined(USE_MEM_KVM)

void StatsMemory::init()
{
	databaseKeys.push_back("total");
	databaseKeys.push_back("free");
	databaseKeys.push_back("active");
	databaseKeys.push_back("inactive");
	databaseKeys.push_back("wired");
	databaseKeys.push_back("cached");
	databaseKeys.push_back("used");
	databaseKeys.push_back("buffer");
	databaseKeys.push_back("swaptotal");
	databaseKeys.push_back("swapused");
	databaseKeys.push_back("swapin");
	databaseKeys.push_back("swapout");

	databaseMap.push_back(memory_value_total);
	databaseMap.push_back(memory_value_free);
	databaseMap.push_back(memory_value_active);
	databaseMap.push_back(memory_value_inactive);
	databaseMap.push_back(memory_value_wired);
	databaseMap.push_back(memory_value_cached);
	databaseMap.push_back(memory_value_used);
	databaseMap.push_back(memory_value_buffer);
	databaseMap.push_back(memory_value_swaptotal);
	databaseMap.push_back(memory_value_swapused);
	databaseMap.push_back(memory_value_swapin);
	databaseMap.push_back(memory_value_swapout);

	_init();
}

void StatsMemory::update(long long sampleID)
{
	mem_data _mem;
	prepareSample(&_mem);

	size_t len;
	double kbpp;
	struct vmmeter sum;
	struct kvm_swap swap[1];

	struct nlist nl[] = {
		{ "_cnt" },
		{ NULL }
	};

	/* get virtual memory data */
	if (kvm_nlist(kd, nl) == -1)
	{
		cout << "kvm_nlist(): " << strerror(errno) << endl;
		return;
	}

	len = sizeof(sum);

	if (kvm_read(kd, nl[0].n_value, &sum, len) == -1)
	{
		cout << "kvm_read(): " << strerror(errno) << endl;
		return;
	}

	/* kilo bytes per page */
	kbpp = sum.v_page_size;// / 1024;

	_mem.values[memory_value_total] = (double)(sum.v_page_count * kbpp);
	_mem.values[memory_value_free] = (double)(sum.v_free_count * kbpp);
	_mem.values[memory_value_active] = (double)(sum.v_active_count * kbpp);
	_mem.values[memory_value_inactive] = (double)(sum.v_inactive_count * kbpp);
	_mem.values[memory_value_cached] = (double)(sum.v_cache_count * kbpp);
	_mem.values[memory_value_wired] = (double)(sum.v_wire_count * kbpp);
	_mem.values[memory_value_used] = (double)(_mem.values[memory_value_active] + _mem.values[memory_value_wired]);

	_mem.values[memory_value_swapin] = (double)(sum.v_swappgsin * kbpp);
	_mem.values[memory_value_swapout] = (double)(sum.v_swappgsout * kbpp);

#ifdef HAVE_SYSCTLBYNAME
		size_t bufsizelen;
        long bufsize;
        bufsizelen = sizeof(bufsize);

        if (sysctlbyname("kern.nbuf", &bufsize, &bufsizelen, NULL, 0) >= 0)
        {
        	_mem.values[memory_value_buffer] = (double)((bufsize * 16) * 1024);
        }
#endif

	if (kvm_getswapinfo(kd, swap, 1, 0) == -1)
	{
		cout << "kvm_getswapinfo(): " << strerror(errno) << endl;
		return;
	}

	_mem.values[memory_value_swaptotal] = (double)((double)swap[0].ksw_total * kbpp);
	_mem.values[memory_value_swapused] = (double)((double)swap[0].ksw_used * kbpp);

	addSample(_mem, sampleID);

} /*USE_MEM_KVM*/

# elif defined(HAVE_LIBPERFSTAT) && defined(USE_MEM_PERFSTAT)

void StatsMemory::init()
{
	databaseKeys.push_back("total");
	databaseKeys.push_back("free");
	databaseKeys.push_back("used");
	databaseKeys.push_back("swaptotal");
	databaseKeys.push_back("swapused");
	databaseKeys.push_back("swapin");
	databaseKeys.push_back("swapout");
	databaseKeys.push_back("virtualtotal");
	databaseKeys.push_back("virtualactive");

	databaseMap.push_back(memory_value_total);
	databaseMap.push_back(memory_value_free);
	databaseMap.push_back(memory_value_used);
	databaseMap.push_back(memory_value_swaptotal);
	databaseMap.push_back(memory_value_swapused);
	databaseMap.push_back(memory_value_swapin);
	databaseMap.push_back(memory_value_swapout);
	databaseMap.push_back(memory_value_virtualtotal);
	databaseMap.push_back(memory_value_virtualactive);

	_init();
}

void StatsMemory::update(long long sampleID)
{
	perfstat_memory_total_t meminfo;
	int ps;
	mem_data _mem;
	prepareSample(&_mem);

	ps=getpagesize();
	perfstat_memory_total(NULL, &meminfo, sizeof meminfo, 1);
	_mem.values[memory_value_total]   = (double)(ps * meminfo.real_total);
	_mem.values[memory_value_free]   = (double)(ps * meminfo.real_free);
	_mem.values[memory_value_swapin] = (double)(ps * meminfo.pgspins);
	_mem.values[memory_value_swapout] = (double)(ps * meminfo.pgspouts);
	_mem.values[memory_value_swaptotal] = (double)(ps * meminfo.pgsp_total);
	_mem.values[memory_value_swapused] = (double)(ps * (meminfo.pgsp_total- meminfo.pgsp_free));
	_mem.values[memory_value_used] = (double)(_mem.values[memory_value_total] - _mem.values[memory_value_free]);
	_mem.values[memory_value_virtualtotal]   = (double)(ps * meminfo.virt_total);
	_mem.values[memory_value_virtualactive]   = (double)(ps * meminfo.virt_active);

	addSample(_mem, sampleID);

} /*USE_MEM_PERFSTAT*/
#elif defined(USE_MEM_PROCFS)

void StatsMemory::init()
{
	databaseKeys.push_back("total");
	databaseKeys.push_back("free");
	databaseKeys.push_back("active");
	databaseKeys.push_back("inactive");
	databaseKeys.push_back("cached");
	databaseKeys.push_back("used");
	databaseKeys.push_back("buffer");
	databaseKeys.push_back("swaptotal");
	databaseKeys.push_back("swapused");
	databaseKeys.push_back("swapin");
	databaseKeys.push_back("swapout");

	databaseMap.push_back(memory_value_total);
	databaseMap.push_back(memory_value_free);
	databaseMap.push_back(memory_value_active);
	databaseMap.push_back(memory_value_inactive);
	databaseMap.push_back(memory_value_cached);
	databaseMap.push_back(memory_value_used);
	databaseMap.push_back(memory_value_buffer);
	databaseMap.push_back(memory_value_swaptotal);
	databaseMap.push_back(memory_value_swapused);
	databaseMap.push_back(memory_value_swapin);
	databaseMap.push_back(memory_value_swapout);

	_init();
}

void StatsMemory::update(long long sampleID)
{
	char buf[320];
	FILE * fp = NULL;
	mem_data _mem;
	prepareSample(&_mem);

	unsigned long long t = 0;
	unsigned long long f = 0;
	unsigned long long a = 0;
	unsigned long long i = 0;
	unsigned long long swt = 0;
	unsigned long long swf = 0;
	unsigned long long swi = 0;
	unsigned long long swo = 0;
	unsigned long long c = 0;
	unsigned long long b = 0;

	if (!(fp = fopen("/proc/meminfo", "r"))) return;

	while (fgets(buf, sizeof(buf), fp))
	{
		sscanf(buf, "MemTotal: %llu kB", &t);
		sscanf(buf, "MemFree: %llu kB", &f);
		sscanf(buf, "Active: %llu kB", &a);
		sscanf(buf, "Inactive: %llu kB", &i);
		sscanf(buf, "SwapTotal: %llu kB", &swt);
		sscanf(buf, "SwapFree: %llu kB", &swf);
		sscanf(buf, "Cached: %llu kB", &c);
		sscanf(buf, "Buffers: %llu kB", &b);
	}

	swf = swf * 1024;

	_mem.values[memory_value_total] = (double)(t * 1024);
	_mem.values[memory_value_free] = (double)(f * 1024);
	_mem.values[memory_value_active] = (double)(a * 1024);
	_mem.values[memory_value_inactive] = (double)(i * 1024);
	_mem.values[memory_value_swaptotal] = (double)(swt * 1024);
	_mem.values[memory_value_cached] = (double)(c * 1024);
	_mem.values[memory_value_buffer] = (double)(b * 1024);
	_mem.values[memory_value_swapused] = (double)(_mem.values[memory_value_swaptotal] - swf);

	if (0 == _mem.values[memory_value_active] && 0 == _mem.values[memory_value_inactive] && 0 == _mem.values[memory_value_cached])
	{
		_mem.values[memory_value_active] = (double)(_mem.values[memory_value_total] - _mem.values[memory_value_free]);
	}

	fclose(fp);
	
	if (!(fp = fopen("/proc/vmstat", "r"))) return;
	
	while (fgets(buf, sizeof(buf), fp))
	{
		sscanf(buf, "pswpin %llu", &swi);
		sscanf(buf, "pswpout %llu", &swo);
	}
	
	_mem.values[memory_value_swapin] = (double)swi;
	_mem.values[memory_value_swapout] = (double)swo;

	fclose(fp);
	
	_mem.values[memory_value_used] = (double)(_mem.values[memory_value_total] - (_mem.values[memory_value_free] + _mem.values[memory_value_buffer] + _mem.values[memory_value_cached]));

	addSample(_mem, sampleID);

} /* USE_MEM_PROCFS */
#elif defined(USE_MEM_KSTAT)

void StatsMemory::init()
{
	databaseKeys.push_back("total");
	databaseKeys.push_back("free");
	databaseKeys.push_back("active");
	databaseKeys.push_back("inactive");
	databaseKeys.push_back("cached");
	databaseKeys.push_back("used");
	databaseKeys.push_back("swaptotal");
	databaseKeys.push_back("swapused");
	databaseKeys.push_back("swapin");
	databaseKeys.push_back("swapout");

	databaseMap.push_back(memory_value_total);
	databaseMap.push_back(memory_value_free);
	databaseMap.push_back(memory_value_active);
	databaseMap.push_back(memory_value_inactive);
	databaseMap.push_back(memory_value_cached);
	databaseMap.push_back(memory_value_used);
	databaseMap.push_back(memory_value_swaptotal);
	databaseMap.push_back(memory_value_swapused);
	databaseMap.push_back(memory_value_swapin);
	databaseMap.push_back(memory_value_swapout);

	_init();
}

void StatsMemory::update(long long sampleID)
{
	kstat_t *ksp;
	kstat_named_t *kn;
	unsigned long long lv, ps;
	int cc, ncpu;
	cpu_stat_t cs;
	mem_data _mem;
	prepareSample(&_mem);

	if(NULL == (ksp = kstat_lookup(ksh, (char*)"unix", -1, (char*)"system_pages"))) return;
	if(-1 == kstat_read(ksh, ksp, NULL)) return;
	
	ps = getpagesize();
	
	if(NULL != (kn = (kstat_named_t *) kstat_data_lookup(ksp, (char*)"pagestotal")))
	{
		lv = ksgetull(kn);
		_mem.values[memory_value_total] = (double)(lv * ps);// / 1024;
	}
	
	if(NULL != (kn = (kstat_named_t *) kstat_data_lookup(ksp, (char*)"pageslocked")))
	{
		lv = ksgetull(kn);
		_mem.values[memory_value_cached] = (double)(lv * ps);// / 1024;
	}

	if(NULL != (kn = (kstat_named_t *) kstat_data_lookup(ksp, (char*)"pagesfree")))
	{
		lv = ksgetull(kn);
		_mem.values[memory_value_free] = (double)(lv * ps);// / 1024;
	}

	if(NULL != (kn = (kstat_named_t *) kstat_data_lookup(ksp, (char*)"pp_kernel")))
	{
		lv = ksgetull(kn);
		_mem.values[memory_value_wired] = (double)(lv * ps);// / 1024;
	}

	_mem.values[memory_value_active] = (double)(_mem.values[memory_value_total] - _mem.values[memory_value_free]);
	_mem.values[memory_value_used] = (double)(_mem.values[memory_value_active]);

	ncpu = sysconf(_SC_NPROCESSORS_CONF);
	
	for(cc = 0; cc < ncpu; cc++)
	{
		if(p_online(cc, P_STATUS) != P_ONLINE)
		{
			continue;
		}
		if(NULL != (ksp = kstat_lookup(ksh, (char*)"cpu_stat", cc, NULL)))
		{
			if(-1 != kstat_read(ksh, ksp, &cs))
			{
				_mem.values[memory_value_swapin] += (double)(cs.cpu_vminfo.pgin);
				_mem.values[memory_value_swapout] += (double)(cs.cpu_vminfo.pgout);
			}
		}
	}
	get_swp_data(&_mem);

	addSample(_mem, sampleID);
}

void StatsMemory::get_swp_data(struct mem_data * _mem)
{
    swaptbl_t *s;
    char strtab[255];
    int swap_num;
    int status;
    int i;
	int bpp = getpagesize() >> DEV_BSHIFT;

    unsigned long long avail = 0;
    unsigned long long total = 0;

    swap_num = swapctl (SC_GETNSWP, NULL);
    if (swap_num < 0)
    {
		return;
    }
    else if (swap_num == 0)
		return;

    s = (swaptbl_t *) malloc (swap_num * sizeof (swapent_t) + sizeof (struct swaptable));
    if (s == NULL)
    {
		return;
    }

    for (i = 0; i < (swap_num + 1); i++) {
            s->swt_ent[i].ste_path = strtab;
    }
    s->swt_n = swap_num + 1;
    status = swapctl (SC_LIST, s);
    if (status != swap_num)
    {
		free (s);
		return;
    }

    for (i = 0; i < swap_num; i++)
    {
		if ((s->swt_ent[i].ste_flags & ST_INDEL) != 0)
			continue;

		avail += ((unsigned long long) s->swt_ent[i].ste_free) * bpp * DEV_BSIZE;
		total += ((unsigned long long) s->swt_ent[i].ste_pages) * bpp * DEV_BSIZE;
    }

    if (total < avail)
		return;

	_mem->values[memory_value_swaptotal] = (double)(total);
	_mem->values[memory_value_swapused] = (double)(total - avail);
}

#else

void StatsMemory::init()
{
	_init();
}

void StatsMemory::update(long long sampleID) {}

#endif

void StatsMemory::prepareSample(mem_data* data)
{
	int x;
	for(x=0;x<memory_values_count;x++)
		data->values[x] = -1;	
}

void StatsMemory::addSample(mem_data data, long long sampleID)
{
	data.sampleID = sampleIndex[0].sampleID;
	data.time = sampleIndex[0].time;

	samples[0].push_front(data);	
	if (samples[0].size() > HISTORY_SIZE) samples[0].pop_back();
}

void StatsMemory::_init()
{	
	initShared();

	#ifdef USE_SQLITE
	if(historyEnabled == true)
	{
		databaseType = 0;
		databasePrefix = "memory_";

		int x;
		for(x=1;x<8;x++)
		{
			string table = databasePrefix + tableAtIndex(x);
			if(!_database.tableExists(table))
			{
				string sql = "create table " + table + " (sample double PRIMARY KEY NOT NULL, time double NOT NULL DEFAULT 0, empty integer NOT NULL DEFAULT 0)";		
				DatabaseItem dbItem = _database.databaseItem(sql);
				dbItem.executeUpdate();
			}

			size_t y;
			for(y=0;y<databaseKeys.size();y++)
			{
				if(!_database.columnExists(databaseKeys[y], table))
				{
					stringstream sql;
					sql << "ALTER TABLE " << table << " ADD COLUMN " << databaseKeys[y] << " DOUBLE NOT NULL DEFAULT 0";

					DatabaseItem query = _database.databaseItem(sql.str());
					query.executeUpdate();
				}			
			}
		}
		loadPreviousSamples();
		fillGaps();
	}
	#endif
}

#ifdef USE_SQLITE
void StatsMemory::loadPreviousSamples()
{
	loadPreviousSamplesAtIndex(1);
	loadPreviousSamplesAtIndex(2);
	loadPreviousSamplesAtIndex(3);
	loadPreviousSamplesAtIndex(4);
	loadPreviousSamplesAtIndex(5);
	loadPreviousSamplesAtIndex(6);
	loadPreviousSamplesAtIndex(7);
}

void StatsMemory::loadPreviousSamplesAtIndex(int index)
{
	string table = databasePrefix + tableAtIndex(index);
	double sampleID = sampleIdForTable(table);

	string sql = "select * from " + table + " where sample >= @sample order by sample asc limit 602";
	DatabaseItem query = _database.databaseItem(sql);
	sqlite3_bind_double(query._statement, 1, sampleID - 602);

	while(query.next())
	{
		mem_data sample;
		prepareSample(&sample);

		size_t y;
		for(y=0;y<databaseKeys.size();y++)
		{
			sample.values[databaseMap[y]] = query.doubleForColumn(databaseKeys[y]);
		}

		sample.sampleID = (long long)query.doubleForColumn("sample");
		sample.time = query.doubleForColumn("time");
		samples[index].push_front(sample);
	}
	if(samples[index].size() > 0)
	{
		sampleIndex[index].sampleID = samples[index][0].sampleID;
		sampleIndex[index].time = samples[index][0].time;
		sampleIndex[index].nextTime = sampleIndex[index].time + sampleIndex[index].interval;
	}
}

void StatsMemory::updateHistory()
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

				mem_data sample = historyItemAtIndex(x);

				samples[x].push_front(sample);	
				if (samples[x].size() > HISTORY_SIZE) samples[x].pop_back();

				if(sample.empty)
					continue;

				string table = databasePrefix + tableAtIndex(x);
				stringstream sql;
				sql << "insert into " + table + " (empty, sample, time";

				size_t y;
				for(y=0;y<databaseKeys.size();y++)
				{
					sql << ", " << databaseKeys[y];
				}

				sql << ") values(?, ?, ?";

				for(y=0;y<databaseKeys.size();y++)
				{
					sql << ", ?";
				}

				sql << ")";


				DatabaseItem dbItem = _database.databaseItem(sql.str());
				sqlite3_bind_int(dbItem._statement, 1, 0);
				sqlite3_bind_double(dbItem._statement, 2, (double)sample.sampleID);
				sqlite3_bind_double(dbItem._statement, 3, sample.time);

				for(y=0;y<databaseMap.size();y++)
				{
					sqlite3_bind_double(dbItem._statement, (y + 4), sample.values[databaseMap[y]]);
				}

				databaseQueue.push_back(dbItem);
			}
		}
	}
}

mem_data StatsMemory::historyItemAtIndex(int index)
{
	std::deque<mem_data> from = samples[sampleIndex[index].historyIndex];
	double minimumTime = sampleIndex[index].time - sampleIndex[index].interval;
	double maximumTime = sampleIndex[index].time;
	if(sampleIndex[index].historyIndex == 0)
		maximumTime += 0.99;

	mem_data sample;

	double values[memory_values_count];

	int x;
	for(x=0;x<memory_values_count;x++)
		values[x] = 0;

	int count = 0;
	if(from.size() > 0)
	{
		for (deque<mem_data>::iterator cur = from.begin(); cur != from.end(); ++cur)
		{
			if ((*cur).time > maximumTime)
				continue;

			if ((*cur).time < minimumTime)
				break;

			for(x=0;x<memory_values_count;x++)
				values[x] += (*cur).values[x];
			count++;
		}
		if (count > 0)
		{
			for(x=0;x<memory_values_count;x++)
				values[x] /= count;
		}
	}

	for(x=0;x<memory_values_count;x++){
		if(samples[0].size() > 0)
		{
			if(samples[0][0].values[x] == -1)
				values[x] = -1;
		}
	}


	for(x=0;x<memory_values_count;x++)
		sample.values[x] = values[x];

	sample.sampleID = sampleIndex[index].sampleID;
	sample.time = sampleIndex[index].time;
	sample.empty = false;
	if(count == 0)
		sample.empty = true;

	return sample;
}
#endif

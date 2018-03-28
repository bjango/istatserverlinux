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

#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <time.h>
#include <stdio.h>

#define SERVER_VERSION 3.03
#define SERVER_BUILD 105
#define PROTOCOL_VERSION 3
#define HISTORY_SIZE 600

struct load_data
{
	float one, two, three;
	long long sampleID;
	double time;
	bool empty;
};

struct cpu_data
{
	double u, n, s, i, io, ent, phys;
	long long sampleID;
	double time;
	bool empty;
};

#define memory_value_total 0
#define memory_value_free 1
#define memory_value_used 2
#define memory_value_active 3
#define memory_value_inactive 4
#define memory_value_cached 5
#define memory_value_swapused 6
#define memory_value_swaptotal 7
#define memory_value_swapout 8
#define memory_value_swapin 9
#define memory_value_buffer 10
#define memory_value_wired 11
#define memory_value_ex 12
#define memory_value_file 13
#define memory_value_virtualtotal 14
#define memory_value_virtualactive 15

#define memory_values_count 16

struct sample_data
{
	long long sampleID;
	double time;
};

struct mem_data
{
	double values[memory_values_count];
	long long sampleID;
	double time;
	bool empty;
};

struct activity_data
{
	long long sampleID;
	double r, w;
	double rIOPS, wIOPS;
	double time;
	bool empty;
};

struct net_data
{
	long long sampleID;
	double u, d;
	double time;
	bool empty;
};

struct disk_data
{
	long long sampleID;
	float p;
	double t, u, f;
	double time;
	bool empty;
};

struct sensor_data
{
	long long sampleID;
	double value;
	double time;
	bool empty;
};

#endif

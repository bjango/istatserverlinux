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

#ifndef _STATSSENSORS_H
#define _STATSSENSORS_H

class sensor_info
{
	public:
		int id;
		std::string key;
		double last_update;
		double lowestValue;
		double highestValue;
		bool recordedValueChanged;

		int kind;
		std::string label;
		unsigned int chip;
		unsigned int sensor;
		int method;

		std::deque<sensor_data> samples[8];
};

class StatsSensors : public StatsBase
{
	public:
		void init();
		void _init();
		void init_dev_cpu();
		void init_qnap();
		void init_acpi_thermal();
		void init_acpi_freq();
		#ifdef HAVE_LIBSENSORS
		void init_libsensors();
		#endif

		void update(long long sampleID);
		void update_qnap(long long sampleID);
		void update_dev_cpu(long long sampleID);
		void update_acpi_thermal(long long sampleID);
		void update_acpi_freq(long long sampleID);
		#ifdef HAVE_LIBSENSORS
		void update_libsensors(long long sampleID);
		#endif

		std::vector<sensor_info> _items;
		int createSensor(std::string key);
		void processSensor(std::string key, long long sampleID, double value);


		#ifdef HAVE_LIBSENSORS
		bool libsensors_ready;
		#if SENSORS_API_VERSION >= 0x0400 /* libsensor 4 */
		int sensorType(int type);
		#endif
		#endif

		#ifdef USE_SQLITE
		void updateHistory();
		sensor_data historyItemAtIndex(int index, sensor_info item);
		void loadPreviousSamples();
		void loadPreviousSamplesAtIndex(int index);
		#endif

	   	std::deque<sample_data> samples[8];
	};
#endif

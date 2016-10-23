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

#include <vector>
#include <sstream>
#include <errno.h>
#include <fstream>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

#include "Utility.h"

using namespace std;

#ifdef HAVE_KSTAT_H

unsigned long long ksgetull(kstat_named_t *kn)
{
	switch(kn->data_type)
	{
# ifdef KSTAT_DATA_INT32
		case KSTAT_DATA_INT32:		return kn->value.i32;
		case KSTAT_DATA_UINT32:		return kn->value.ui32;
		case KSTAT_DATA_INT64:		return kn->value.i64;
		case KSTAT_DATA_UINT64:		return kn->value.ui64;
# else
		case KSTAT_DATA_LONG:		return kn->value.l;
		case KSTAT_DATA_ULONG:		return kn->value.ul;
		case KSTAT_DATA_LONGLONG:	return kn->value.ll;
		case KSTAT_DATA_ULONGLONG:	return kn->value.ull;
# endif
		default:
			return (unsigned long long) -1;
	}
}
#endif

int serverPlatform()
{
	int platform = 2;

#if defined(__sun) && defined(__SVR4)
	platform = 3;
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD)
	platform = 4;
#endif

#if defined(_AIX) || defined(__AIX) || defined(__AIX__) || defined(__aix) || defined(__aix__)
	platform = 6;
#endif

#if defined(__NetBSD__) || defined(__NetBSD)
	platform = 8;
#endif

#if defined(__OpenBSD__) || defined(__OPENBSD)
	platform = 9;
#endif

#if defined(__DragonFly__)
	platform = 10;
#endif

	return platform;
}

double get_current_time()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (double)tp.tv_sec + ((double)tp.tv_usec / 1000000.f);
}

string get_current_time_string()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);

	double t = (double)tp.tv_sec + ((double)tp.tv_usec / 1000000.f);
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%.4f", t);

	return string(buffer);
}

uid_t get_current_uid()
{
	return geteuid();
}

gid_t get_current_gid()
{
	return getegid();
}

uid_t get_uid_from_str(const string &_user)
{
	struct passwd * ent;
	
	if(!(ent = getpwnam(_user.c_str())))
	{
		return -1;
	}
	
	return(ent->pw_uid);
}

uid_t get_gid_from_str(const string &_group)
{
	struct group * ent;
	
	if(!(ent = getgrnam(_group.c_str())))
	{
		return -1;
	}
	
	return(ent->gr_gid);
}

int get_file_owner(const string &_file)
{
	struct stat stats;
	
	stat(_file.c_str(), &stats);
	
	return stats.st_uid;
}

int pid_dead(int _pid)
{
	// Return 1 if process is dead
	if (waitpid(_pid, NULL, WNOHANG) != 0)
		return 1;
	
	return 0;
}

int check_dir_exist(const string &_dir)
{
	struct stat stats;
	
	// Return 1 if dir exists
	if (stat(_dir.c_str(), &stats) == 0 && S_ISDIR(stats.st_mode) == 1)
		return 1;
	
	return 0;
}

int check_file_exist(const string &_file)
{
	struct stat stats;
	
	// Return 1 if file exists
	if (stat(_file.c_str(), &stats) == 0 && S_ISREG(stats.st_mode) == 1)
		return 1;
	
	return 0;
}

string trim(const string & source, const char *_delim)
{
	string result(source);
	string::size_type index = result.find_last_not_of(_delim);
	
	if (index != string::npos)
		result.erase(++index);
	
	index = result.find_first_not_of(_delim);
	
	if (index != string::npos)
		result.erase(0, index);
	else
		result.erase();
	
	return result;
}

vector<string> split(const string &_str, const string _delim)
{
	vector<string> v;
	string str_elem("");
	std::string::size_type ui_cur_pos, ui_last_pos = 0;
	
	// Check for empty string
	if (_str.empty()) return v;
	
	ui_cur_pos = _str.find_first_of(_delim.c_str(), ui_last_pos);
	
	while(ui_cur_pos != _str.npos)
	{
		str_elem = _str.substr(ui_last_pos, ui_cur_pos-ui_last_pos);
		v.push_back(str_elem);
		
		ui_last_pos = ui_cur_pos + 1;
		ui_cur_pos = _str.find_first_of(_delim.c_str(), ui_last_pos);
	}
	
	// Handle last substring - if any
	if(_str.length() != ui_last_pos)
	{
		str_elem = _str.substr(ui_last_pos, _str.length()-ui_last_pos);
		v.push_back(str_elem);
	}
	
	return v;
}

vector<string> explode(string _source, const string &_delim)
{
	vector<string> ret;
	string splitted_part;
	unsigned int i, no_match = 0;
	string::size_type pos, split_pos;
	
	// Loop array string until we can't find more delimiters
	while (no_match < _delim.length())
	{
		no_match = 0;
		split_pos = string::npos;
		
		// Find first occuring splitter
		for (i = 0; i < _delim.length(); i++)
		{
			pos = _source.find(_delim[i], 0);
			
			if (pos == string::npos) no_match++;
			if (pos < split_pos) split_pos = pos;
		}
		
		// Be nice to things wrapped with quotes
		if (_source[0] == '"' && _source.substr(1).find_first_of("\"") != string::npos)
		{
			split_pos = _source.substr(1).find_first_of("\"") + 2;
		}
		
		// One value from the array
		splitted_part = _source.substr(0, split_pos);
		
		// Save the value if it's not empty
		if (splitted_part != "")
		{
			ret.push_back(splitted_part);
		}
		
		// Remove value from string
		_source.erase(0, split_pos + 1);
	}
	
	return ret;
}

int create_directory(const string &_dir, mode_t _mask)
{
	if (mkdir(_dir.c_str(), _mask) < 0)
		return -1;
	
	return 0;
}

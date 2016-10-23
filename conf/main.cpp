#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

#ifdef HAVE_ERRNO_H
# include "errno.h"
#endif

#include <syslog.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>

using namespace std;

int
main(int argc, char **argv)
{
	static FILE * fp = NULL;

	char buf[1024];

	vector<string> lines;
	vector<string> lines_old;

	bool generatePasscode = false;
	bool added_disk_labels = false;

	string argument;
	int c;
	for (c = 1; c < argc; c++)
	{
		argument = string(argv[c]);
		
		if (argument == "-p")
		{
			generatePasscode = true;
		}
	}

	string path_old = string(CONFIG_PATH) + "istatserver.conf.old";
	fp = fopen(path_old.c_str(), "r");
	if(fp)
	{
		while (fgets(buf, sizeof(buf), fp)){
			string line = string(buf);
			lines_old.push_back(line);
		}
		fclose(fp);
		fp = NULL;
	}

	string path = string(CONFIG_PATH) + "istatserver.conf";
	if (!(fp = fopen(path.c_str(), "r"))) return 0;

	while (fgets(buf, sizeof(buf), fp))
	{
		string line = string(buf);

		bool found_old_line = false;
		if(lines_old.size() > 0)
		{
			if(line.find("disk_rename_label") != std::string::npos)
			{
				string key = line.substr(0, line.find(" "));

				if(!added_disk_labels)
				{
					for (vector<string>::iterator cur = lines_old.begin(); cur < lines_old.end(); ++cur)
					{
						if((*cur).find("disk_rename_label") != std::string::npos)
						{
							lines.push_back((*cur));
						}
					}
				}
				found_old_line = true;
				added_disk_labels = true;
			}
			else if(line.length() > 0 && line.substr(0, 1) != "#")
			{
				string::size_type pos = line.find_first_of(" \t");
				if(pos != std::string::npos)
				{
					string key = line.substr(0, pos);

					if(key == "server_code" && generatePasscode == true)
					{
						srand(time(NULL));
						int c = (rand()%88888) + 10000;

						stringstream code;
						code << c;

						cout << "generating passcode" << endl;

						line = "server_code              " + code.str() + "\n";
					}
					else
					{
						for (vector<string>::iterator cur = lines_old.begin(); cur < lines_old.end(); ++cur)
						{
							if((*cur).find(key) == 0)
							{
								lines.push_back((*cur));
								found_old_line = true;
							}
						}
					}
				}
			}
		}

		if(!found_old_line)
		{
			if(line.length() > 0 && line.substr(0, 1) != "#")
			{
				string::size_type pos = line.find_first_of(" \t");
				if(pos != std::string::npos)
				{
					string key = line.substr(0, pos);

					if(key == "server_code" && generatePasscode == true)
					{
						srand(time(NULL));
						int c = (rand()%88888) + 10000;

						stringstream code;
						code << c;

						cout << "generating passcode" << endl;

						line = "server_code              " + code.str() + "\n";
					}
				}
			}
			
			if(!found_old_line)
				lines.push_back(line);
		}
	}

	if(lines.size() > 0)
	{
		string new_path = string(CONFIG_PATH) + "istatserver.conf";
		ofstream out(new_path.c_str());

		for (vector<string>::iterator cur = lines.begin(); cur < lines.end(); ++cur)
		{
			//cout << (*cur);
			out << (*cur);
		}

		out.close();
	}

	return 0;
}

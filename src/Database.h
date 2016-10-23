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

#include "config.h"
#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

#ifdef USE_SQLITE

#include <sqlite3.h>

#ifndef _DATABASE_H
#define _DATABASE_H
class DatabaseItem
{
	private:
	    typedef std::map<std::string, int> DatabaseColumnNames;

	public:
		int executeUpdate();
		void prepare(std::string sql, sqlite3 *db);
		int next();
		int query();
		void finalize();
		double doubleForColumn(std::string column);
		std::string stringForColumn(std::string column);
		sqlite3_stmt* _statement;
		DatabaseColumnNames columnNames;
		int result;
};
class Database
{
	public:
		sqlite3 *_db;
		int enabled;
		void init();
		bool verify();
		void close();
		int beginTransaction();
		int commit();
		DatabaseItem databaseItem(std::string sql);
		int tableExists(std::string name);
		bool columnExists(std::string column, std::string table);
};

#endif
#endif

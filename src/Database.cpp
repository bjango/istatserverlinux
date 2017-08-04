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

#include "Database.h"
#include <math.h>

using namespace std;

#ifdef USE_SQLITE

bool Database::verify()
{
	sqlite3_stmt *statement = 0;
	bool valid = true;
	int rc = sqlite3_prepare_v2(_db, "PRAGMA integrity_check", -1, &statement, 0);	
	if(SQLITE_OK == rc)
	{
        int success = sqlite3_step(statement);
        string value;
        switch (success) {
            case SQLITE_ERROR:
            case SQLITE_MISUSE:
            case SQLITE_DONE:
            case SQLITE_BUSY:
            	cout << "Error verifying database" << endl;
                break;
            case SQLITE_CORRUPT:
            	cout << "Error verifying database - Corrupt" << endl;
            	valid = false;
                break;
            case SQLITE_ROW:
			    value = string((char *)sqlite3_column_text(statement, 0));
	            
	            if(value == "ok")
	            {
	            	cout << "Database valid" << endl;
                }
                else
                {
	            	cout << "Database invalid - " << value << endl;
                    valid = false;
                }
                break;
            default:
                break;
        }
        
        sqlite3_finalize(statement);
	}
	else
	{
		cout << "Unable to verify database" << endl;
	}
	return valid;
}

void DatabaseItem::prepare(string sql, sqlite3 *db)
{
	_statement = 0;
	int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &_statement, 0);	
	if(SQLITE_OK != rc)
	{
		_statement = NULL;
		return;
	}
}

void DatabaseItem::finalize()
{
	if(_statement == NULL)
		return;

	sqlite3_finalize(_statement);
	_statement = NULL;
}

int DatabaseItem::query()
{
	int rc = sqlite3_step(_statement);
	finalize();
	return rc;
}

int DatabaseItem::next()
{
	int rc = sqlite3_step(_statement);
    if (rc != SQLITE_ROW)
    {
    	finalize();
		return 0;
	}

	if(columnNames.empty())
	{
		int columnCount = sqlite3_column_count(_statement);
		for (int i = 0; i < columnCount; ++i)
		{
			const char* pName = sqlite3_column_name(_statement, i);
			columnNames[pName] = i;
		}
	}

	return 1;
}

double DatabaseItem::doubleForColumn(string column)
{
    const DatabaseColumnNames::const_iterator iIndex = columnNames.find(column);
    if (iIndex == columnNames.end())
    {
    	return 0;
    }
    double value =  sqlite3_column_double(_statement, (*iIndex).second);
    if(isnan(value))
    	value = 0;

    return value;
}

string DatabaseItem::stringForColumn(string column)
{
    const DatabaseColumnNames::const_iterator iIndex = columnNames.find(column);
    if (iIndex == columnNames.end())
    {
    	return 0;
    }

    string value = string((char *)sqlite3_column_text(_statement, (*iIndex).second));
    return value;
}

int DatabaseItem::executeUpdate()
{
	if(_statement == NULL)
		return 0;

	int rc = sqlite3_step(_statement);
    finalize();

    return (rc == SQLITE_DONE || rc == SQLITE_OK);
}

void Database::init()
{
	string path = string(CONFIG_PATH) + "istatserver.db";
	int rc = sqlite3_open(path.c_str(), &_db);
	if(rc != SQLITE_OK)
	{
		cout << "Unable to open database" << endl;
		enabled = 0;
	}
	else
	{
		cout << "Opened database" << endl;
		enabled = 1;		
	}
}

void Database::close()
{
	sqlite3_close(_db);
	cout << "Closed database" << endl;
}

DatabaseItem Database::databaseItem(string sql)
{
	DatabaseItem item;
	item.prepare(sql, _db);
	return item;
}

int Database::beginTransaction()
{
	DatabaseItem query;
	query.prepare("begin exclusive transaction", _db);
	query.query();
	return 0;
}

int Database::commit()
{
	DatabaseItem query;
	query.prepare("commit transaction", _db);
	query.query();
	return 0;
}

int Database::tableExists(string name)
{
	stringstream sql;
	sql << "select name from sqlite_master where type='table' and lower(name) = ?";

	DatabaseItem query;
	query.prepare(sql.str(), _db);

	sqlite3_bind_text(query._statement, 1, name.c_str(), -1, SQLITE_STATIC);

	if(query.next())
	{
		query.finalize();
		return 1;
	}
	return 0;
}

bool Database::columnExists(string column, string table)
{
	stringstream sql;
	sql << "pragma table_info('" << table << "')";

	DatabaseItem query;
	query.prepare(sql.str(), _db);

	while(query.next())
	{
		if(query.stringForColumn("name") == column)
			return true;
	}
	return false;
}

#endif

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
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string.h>
#include <iomanip>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#ifdef HAVE_LIBZLIB
#include <zlib.h>
#endif

#include "Utility.h"
#include "Socket.h"

using namespace std;


#ifdef HAVE_LIBZLIB
std::string compress_string(const std::string& str, int compressionlevel = Z_BEST_COMPRESSION)
{
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, compressionlevel) != Z_OK)
    {
    	return "";
    }

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();           // set the z_stream's input

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // append the block to the output string
            outstring.append(outbuffer,
                             zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
    	return "";
    }

    return outstring;
}
#endif

int Socket::listen()
{
	int yes = 1;
	hostent * host = NULL;
	
	if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "Could not create socket: " << strerror(errno) << endl;
		return 0;
	}
	
	setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &yes, sizeof(yes));
	
	struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

	if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        cout << "setsockopt failed" << endl;
	}

    if (setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        cout << "setsockopt failed" << endl;
    }

	sockaddr_in myAddress;
	
	memset(&myAddress, 0, sizeof(sockaddr_in));
	
	if (!(host = gethostbyname(address.c_str())))
		myAddress.sin_addr.s_addr = inet_addr(address.c_str());
	else
		myAddress.sin_addr.s_addr = ((in_addr *) host->h_addr)->s_addr;
	
	myAddress.sin_family = AF_INET;
	myAddress.sin_port = htons(port);
	
	if (::bind(socket, (sockaddr *) &myAddress, sizeof(myAddress)) == -1)
	{
		cout << "Could not bind socket: " << strerror(errno) << endl;
		return 0;
	}
	
	// cout << "(" << address << ":" << port << ") Socket binded." << endl;
	
	if (::listen(socket, 5) == -1)
	{
		cout << "Could not listen on socket: " << strerror(errno) << endl;
		return 0;
	}

	// cout << "(" << address << ":" << port << ") Socket initialized." << endl;
	return 1;
}


Socket Socket::accept()
{
	int theirSocket;
	sockaddr_in theirAddress;
	socklen_t size = sizeof(sockaddr_in);		
	
	if ((theirSocket = ::accept(socket, (sockaddr *) &theirAddress, &size)) == -1)
	{
		cout << "Could not accept connection: " << strerror(errno) << endl;
	}
		
	Socket socket(theirSocket, inet_ntoa(theirAddress.sin_addr), ntohs(theirAddress.sin_port));
	socket.lastRequest = get_current_time();
	socket.debugLogging = false;
	socket.readbuf = "";
	socket._session = _session;
	socket._serverUUID = _serverUUID;
	socket._sslEnabled = _sslEnabled;
	socket.sslContext = sslContext;

	cout << socket.get_description() << " New connection accepted. " << endl;

    fcntl(socket.get_id(), F_SETFL, fcntl(socket.get_id(), F_GETFL, 0) | O_NONBLOCK);

/*    struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt (theirSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        cout << "setsockopt failed" << endl;

    if (setsockopt (theirSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        cout << "setsockopt failed" << endl;
 */

	return socket;
}

string Socket::get_description()
{
	stringstream desc;
	desc << get_current_time_string() << " - (" << get_address() << ":" << get_port();
	if(_name.length() > 0)
		desc << " - " << _name;
	desc << ")";
	return desc.str();
}

int Socket::send(string data)
{
//	cout << "Writing- " << data << endl;
	int r = 0;
	
	if(secure == true)
	{
		while(data.size() > 0)
		{
			ERR_clear_error();

			unsigned int limit = 8192;

			if(data.size() > limit)
			{
				if(debugLogging)
					cout << get_description() << " Writing " << limit << " bytes" << endl;
				r = SSL_write(ssl, data.substr(0, limit).c_str(), limit);
			}
			else
			{
				if(debugLogging)
					cout << get_description() << " Writing " << data.size() << " bytes" << endl;
				r = SSL_write(ssl, data.c_str(), data.size());
			}

			if(debugLogging)
				cout << get_description() << " Write result " << r << ", ssl error " << SSL_get_error (ssl,r) << ", errorno " << errno << ", err_get_error " << ERR_get_error() << endl;

			switch ( SSL_get_error (ssl,r) ){
				case SSL_ERROR_NONE:
					data = data.substr(r, readbuf.size() - r);
					break;
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_WRITE:
					break;
				case SSL_ERROR_ZERO_RETURN:
					cout << get_description() << " SSL connection closed by peer" << endl;
					return 0;
				break;
				default:
	//					cout << get_description() << " SSL error - " << ERR_error_string( SSL_get_error(ssl, r), NULL ) << endl;
					cout << get_description() << " SSL write error - " << ERR_error_string( ERR_get_error(), NULL ) << endl;
					return 0;
				break;
			}
		}
	}
	else
	{
		if ((r = ::send(socket, data.c_str(), data.size(), 0)) == -1)
		{
			cout << get_description() << " Could not send data: " << strerror(errno) << endl;
			return 0;
		}		
	}
	
	// cout << "(" << address << ":" << port << ") Data sent: " << _data << endl;
	
	return r;
}


int Socket::receive(ClientSet * _clients, Config * _config, Stats * _stats)
{
	char buf[1024];
    int len = 0;

	if(secure == true)
	{
		do {
			memset(buf, 0, 1024);
			ERR_clear_error();
			int r = SSL_read (ssl, buf, 1024);

			if(debugLogging)
				cout << get_description() << " Read result " << r << ", ssl error " << SSL_get_error (ssl,r) << ", errorno " << errno << ", err_get_error " << ERR_get_error() << endl;

			switch ( SSL_get_error (ssl,r) ){
				case SSL_ERROR_NONE:
					len += r;
					readbuf += buf;
//					if(debugLogging)
//						cout << "Read bytes " << buf << endl;

				break;
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_WRITE:
					break;
				case SSL_ERROR_ZERO_RETURN:
					cout << get_description() << " SSL connection closed by peer" << endl;
					return 0;
				break;
				default:
//					cout << get_description() << " SSL read error - " << ERR_error_string( SSL_get_error(ssl, r), NULL ) << endl;
					cout << get_description() << " SSL read error - " << ERR_error_string( ERR_get_error(), NULL ) << endl;
					return 0;
				break;
			}
		}
		while(SSL_pending (ssl) > 0);
	}
	else
	{
		while(1)
		{
			memset(buf, 0, 1024);
			#ifdef MSG_DONTWAIT
			int ret = recv(socket, buf, 1024, MSG_DONTWAIT);
			#else
			int ret = recv(socket, buf, 1024, MSG_NONBLOCK);
			#endif
			if(ret < 0 && (errno == EWOULDBLOCK || errno == EAGAIN))
			{
				break;
			}
			else if(ret <= 0)
			{
				return 0;
			}
			else
			{
				len += ret;
				readbuf += buf;
			}
		}
	}

	size_t position = readbuf.find("</isr>");
	if(position != std::string::npos)
	{
		lastRequest = get_current_time();
		string xml = readbuf.substr(0, position);
//		if(debugLogging)
//			cout << get_description() << " Read xml data " << xml << endl;
		parse(readbuf, _clients, _config, _stats);
		position += 6;
		readbuf = readbuf.substr(position, readbuf.size() - position);
	}

	return len;
}

void Socket::close()
{
}

int Socket::startSSL()
{
	ssl = SSL_new(sslContext);
	SSL_set_fd(ssl, socket);
	SSL_set_accept_state(ssl);
	int code = SSL_accept(ssl);

	if ( code != 1){
		int error = SSL_get_error(ssl, code);
	    while (code != 1 && (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE))
	    {
	    	code = SSL_accept(ssl);
			error = SSL_get_error(ssl, code);
		}
		if(code != 1)
		{
			cout << get_description() << " Error starting SSL: " << ERR_error_string( SSL_get_error(ssl, code), NULL ) <<endl;
			cout << get_description() << " Error starting SSL - " << ERR_error_string( ERR_get_error(), NULL ) << endl;
			ERR_print_errors_fp(stdout);
			fflush(stdout);
			return -1;
		}
    }

	secure = true;
	return 0;
}

void Socket::parse(string _data, ClientSet * _clients, Config * _config, Stats * _stats)
{
	stringstream temp;
	string content, element_name;
		
	// Load properties from config file
	string cf_server_code = _config->get("server_code", "00000");
	string cf_server_reject_delay = _config->get("server_reject_delay", "3");

	
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
	
	doc = xmlCtxtReadMemory(ctxt, _data.c_str(), _data.length(), NULL, NULL, 0);
	
	if ((cur = xmlDocGetRootElement(doc)) != NULL)
	{
		if (xmlStrEqual(cur->name, BAD_CAST "isr"))
		{
	   		char *type = (char *)xmlGetProp(cur, (const xmlChar *)"type");
			int code = atoi(type);
			free(type);
			
			if(code == 100){
	   			char *protocol = (char *)xmlGetProp(cur, (const xmlChar *)"protocol");
				_protocol = atoi(protocol);

				send(isr_accept_connection());
				free(protocol);
			}
			if(code == 101){
	   			char *uuid = (char *)xmlGetProp(cur, (const xmlChar *)"uuid");
	   			char *name = (char *)xmlGetProp(cur, (const xmlChar *)"name");
	   			char *version = (char *)xmlGetProp(cur, (const xmlChar *)"version");

				_uuid = string(uuid);
				_name = string(name);

				int auth = 1;
				if (_clients->is_authenticated(_uuid))
					auth = 0;
				else {
					int code = atoi(cf_server_code.c_str());
					std::ostringstream stm;
					stm << code;
					string stringcode = stm.str();

					// check if code is a 5 digit number
					if(cf_server_code != stringcode || cf_server_code.length() != 5){
						auth = 2; // text based passcode
					}
				}

				send(isr_serverinfo(_session, auth, _serverUUID, _stats->historyEnabled));

				free(uuid);
				free(name);
				free(version);
			}

			if(code == 102){
	   			char *code = (char *)xmlGetProp(cur, (const xmlChar *)"code");
				if (code == cf_server_code)
				{
					_clients->authenticate(_uuid);
					send(isr_accept_code());
				}
				else {
					send(isr_reject_code());
				}
				free(code);
			}

			if(code == 103){
				pthread_mutex_lock(&_stats->lock);	
				temp << isr_create_header() << "<isr type=\"104\">";
				if (_clients->is_authenticated(_uuid))
				{
					xmlNodePtr child = cur->children;
					while (child){
						char *type = (char *)xmlGetProp(child, (const xmlChar *)"type");
						if(type == NULL)
						{
							child = child->next;
							continue;
						}
						if(strcmp(type, "cpu") == 0)
						{
							temp << isr_cpu_data(child, _stats);
						}
						if(strcmp(type, "memory") == 0)
						{
							temp << isr_memory_data(child, _stats);
						}
						if(strcmp(type, "load") == 0)
						{
							temp << isr_loadavg_data(child, _stats);
						}
						if(strcmp(type, "network") == 0)
						{
							#ifndef USE_NET_NONE
							temp << isr_multiple_data(child, _stats);
							#endif
						}
						if(strcmp(type, "diskactivity") == 0)
						{
							#ifndef USE_ACTIVITY_NONE
							temp << isr_multiple_data(child, _stats);
							#endif
						}
						if(strcmp(type, "processes") == 0)
						{
							#ifndef USE_PROCESSES_NONE
							temp << isr_multiple_data(child, _stats);
							#endif
						}
						if(strcmp(type, "disks") == 0)
						{
							#ifndef USE_DISK_NONE
							temp << isr_multiple_data(child, _stats);
							#endif
						}
						if(strcmp(type, "sensors") == 0)
						{
							temp << isr_multiple_data(child, _stats);
						}
						if(strcmp(type, "uptime") == 0)
						{
							temp << isr_uptime_data(_stats->uptime());
						}
						if(strcmp(type, "battery") == 0)
						{
							#ifndef USE_BATTERY_NONE
							temp << isr_multiple_data(child, _stats);
							#endif
						}						

						free(type);
						child = child->next;
					}
				}
				pthread_mutex_unlock(&_stats->lock);

				temp << "</isr>";

				string data = temp.str();
				string compressedTag = "";
			
				#ifdef HAVE_LIBZLIB
				string compressedData = compress_string(data, Z_BEST_COMPRESSION);
				if(compressedData.size() > 0)
				{
					compressedTag = " c=\"1\"";
					data = compressedData;
				}
				#endif

				stringstream temp2;
				temp2 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?><isr type=\"105\" length=\"" << data.length() << "\"" << compressedTag << "></isr>";
				send(temp2.str());
				send(data);
			}
		} // Unknown element recived after header
	} // Failed to read header

	xmlFreeDoc(doc);
	xmlFreeParserCtxt(ctxt);
}

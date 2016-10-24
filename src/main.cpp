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
#include <signal.h>
#include <iostream>

#include "main.h"
#include "Conf.h"
#include "Stats.h"
#include "System.h"
#include "Daemon.h"
#include "Socket.h"
#include "Utility.h"
#include "Argument.h"
#include "Clientset.h"
#include "Avahi.h"
#include "Socketset.h"
#include <unistd.h> 

#include <iostream>
#include <fstream>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef USE_SQLITE
#include "Database.h"
#endif

using namespace std;

SignalResponder * pn_signalresponder = NULL;

int main(int argc, char ** argv)
{
	Stats stats;
	SocketSet sockets;
	ClientSet clients;
	ArgumentSet arguments(argc, argv);

	if (arguments.is_set("version") || arguments.is_set("v"))
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%.2f", SERVER_VERSION);
		cout << PACKAGE_NAME << " version " << buffer << ", build " << SERVER_BUILD << endl;
		return 0;
	}
	
	if (arguments.is_set("verify"))
	{
		#ifdef USE_SQLITE
		Database database;
		database.init();
		database.verify();
		database.close();
		#endif
		return 0;
	}

	if (arguments.is_set("help") || arguments.is_set("h"))
	{
		cout << endl;
		cout << "usage: istatserver [-a HOST] [-p PORT]" << endl;
		cout << endl;
		cout << "    -d                 run in background" << endl;
		cout << "    -h                 print this help text" << endl;
		cout << "    -v                 print version number" << endl;
		cout << "    -verify            verify sqlite database" << endl;
		cout << endl;
		//cout << "    -c DIRECTORY       custom config directory location" << endl;
		cout << "    -a HOST            listen on this address" << endl;
		cout << "    -p PORT            listen on this port" << endl;
		cout << "    -u USER            change running user" << endl;
		cout << "    -g GROUP           change running group" << endl;
		cout << endl;
		cout << "    --pid=FILE         custom pid file location" << endl;
		cout << "    --socket=FILE      custom socket file location" << endl;
		cout << "    --code=CODE        custom passcode" << endl;
		cout << endl;
		return 0;
	}
	
//	string config_directory = arguments.get("c", string(CONFIG_PATH));
	string config_directory = string(CONFIG_PATH);

	// Load and parse configuration
	Config config(config_directory + "istatserver.conf");
	config.parse();
	config.validate();
	
	// Load configuration properties from command line and config file
	bool arg_d = arguments.is_set("d");
	string cf_network_addr = arguments.get("a", config.get("network_addr", "0.0.0.0"));
	string cf_network_port = arguments.get("p", config.get("network_port", "5109"));
	string cf_server_user = arguments.get("u", config.get("server_user", "istat"));
	string cf_server_group = arguments.get("g", config.get("server_group", "istat"));
	string cf_server_pid = arguments.get("pid", config.get("server_pid", "/var/run/istatserver.pid"));

	string cf_server_socket = arguments.get("socket", config.get("server_socket", "/tmp/istatserver.sock"));

	// Load server generated config file
	string generated_path = config_directory + "istatserver_generated.conf";
//	Config configGenerated(arguments.get("c", generated_path.c_str()));
	Config configGenerated(generated_path.c_str());
	configGenerated.parse();
	configGenerated.validate();

	string serverUUID = configGenerated.get("uuid", "");

	if(serverUUID.length() == 0)
	{
		char *uuid = (char *)malloc(128);
		GenerateGuid(uuid);

		std::ofstream outfile;
		outfile.open(generated_path.c_str(), std::ios_base::app);
		outfile << "uuid	" << uuid << "\n";
		outfile.close();
	
		serverUUID = std::string(uuid);

		free(uuid);
	}

	Socket listener(cf_network_addr, to_int(cf_network_port));
	Daemon unixdaemon(cf_server_pid, cf_server_socket);
	SignalResponder signalresponder(&sockets, &listener, &unixdaemon, &stats);
	
	::pn_signalresponder = &signalresponder;

	// Create socket, pid file and put in background if desired
	unixdaemon.create(arg_d, cf_server_user, cf_server_group);

#ifndef HOST_NAME_MAX
	#define HOST_NAME_MAX 1024
#endif

	string hostname = "Unknown";
	char namebuf[HOST_NAME_MAX];  
	int z = gethostname(namebuf, HOST_NAME_MAX);
	if(z == 0)
		hostname = std::string(namebuf);

	if(hostname.length() == 0)
		hostname = "Unknown";

	unixdaemon.publish(to_int(cf_network_port), PROTOCOL_VERSION, hostname, serverUUID, serverPlatform());
	
	// Get old sessions from disk cache
	clients.read_cache(string(CONFIG_PATH));
	
	// Clear cache of saved sessions
	if (arguments.is_set("clear-sessions"))
	{
		clients.clear_cache();
		return 0;
	}

	signal(SIGHUP,  handler);
	signal(SIGUSR1, handler);
	signal(SIGINT,  handler);
	signal(SIGTERM, handler);
	signal(SIGPIPE, handler);
	
	stats.debugLogging = false;
	stats.sampleID = 0;

	bool debugSocket = false;
	bool debugStats = false;

	if (arguments.is_set("debug"))
		debugStats = true;
		
	if (arguments.is_set("debugsocket"))
		debugSocket = true;

	stats.debugLogging = debugStats;

	listener._session = (long)get_current_time();
	listener._serverUUID = serverUUID;
	listener._sslEnabled = 1;
	listener.isServer = true;
	listener.debugLogging = debugSocket;


	string privateKeyPath = string(CONFIG_PATH) + "key.pem";
	string certPath = string(CONFIG_PATH) + "cert.pem";

	listener.sslContext = InitServerCTX();
	if(listener.sslContext != NULL)
	{
		if(access(certPath.c_str(), F_OK) == -1)
		{
			createSSLCertificate();
		}

		LoadCertificates(listener.sslContext, (char *)certPath.c_str(), (char *)privateKeyPath.c_str());
		listener._sslEnabled = 1;
	}

	if (!listener.listen()) return 1;
	
	sockets += listener;

	stats.historyEnabled = true;
	if(to_int(config.get("disable_history_storage", "0")) == 1)
		stats.historyEnabled = false;

	stats.diskStats.useMountPaths = to_int(config.get("disk_mount_path_label", "0"));
	stats.diskStats.customNames = config.get_array("disk_rename_label");
	stats.diskStats.disableFiltering = to_int(config.get("disk_disable_filtering", "0"));
	stats.start();

	while (1)
	{
		if (sockets.get_status(1) > 0)
		{
			if (sockets == listener)
			{
				Socket new_socket = listener.accept();
				new_socket.debugLogging = debugSocket;
				new_socket.sslContext = InitServerCTX();
				LoadCertificates(new_socket.sslContext, (char *)certPath.c_str(), (char *)privateKeyPath.c_str());
				new_socket.startSSL();
				sockets += new_socket;
			}
			else
			{
				Socket &active_socket = sockets.get_ready();

				if (active_socket.receive(&clients, &config, &stats))
				{
				}
				else
				{
					sockets -= active_socket;
				}
			}
		}

		bool finished = false;
		while(finished == false)
		{
			finished = true;

			vector<Socket>::iterator it = sockets.connections.begin();
			while(it != sockets.connections.end())
			{
				if((*it).isServer == true)
				{
					++it;
					continue;
				}
		    	if((*it).lastRequest < (get_current_time() - 120)){
		    		cout << (*it).get_description() << " Removing connection due to timeout: " << (long long)(get_current_time() - (*it).lastRequest) << endl;
		    	    sockets -= (*it);
		    	    finished = false;
		    	    break;
	   			}
			    ++it;
			}		
		}
	}

	::pn_signalresponder = NULL;

	return 0;
}

SSL_CTX* InitServerCTX(void)
{   
	SSL_library_init();

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(TLSv1_server_method());
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stdout);
        fflush(stdout);
        return NULL;
    }

    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

    EC_KEY *ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (ecdh == NULL) {
        cout << "Failed to get EC curve" << endl;
        SSL_CTX_free(ctx);
        return NULL;
    }
    SSL_CTX_set_tmp_ecdh(ctx, ecdh);
    EC_KEY_free(ecdh);

    DH *dh = get_dh2236();
    if (dh == NULL) {
        cout << "Failed to get DH params" << endl;
        SSL_CTX_free(ctx);
        return NULL;
    }
    SSL_CTX_set_tmp_dh(ctx, dh);
    DH_free(dh);

    return ctx;
}

DH *get_dh2236()
	{
	static unsigned char dh2236_p[]={
		0x0A,0x08,0x7D,0xA9,0x64,0x7E,0xC4,0xD5,0xFB,0x56,0xD2,0xFA,
		0x17,0x36,0x21,0xFE,0xE5,0x37,0x9B,0x1C,0x56,0x2B,0x52,0x5A,
		0x81,0xA5,0x76,0x54,0xF0,0x35,0xEB,0xCA,0xC4,0x65,0xA4,0x62,
		0x3E,0x54,0xBC,0x05,0xEC,0x8F,0xBC,0xCB,0x95,0xA4,0x4B,0xE4,
		0x45,0xD2,0xFB,0x4D,0x89,0x39,0x97,0x99,0x3F,0x13,0xCA,0xF4,
		0x42,0xC6,0x19,0xB3,0xCD,0xE6,0xEA,0x50,0xFB,0x9E,0x9E,0xEB,
		0xB6,0x31,0x31,0x58,0xB5,0xA4,0x48,0x9C,0x1E,0x3F,0xFF,0x6F,
		0xE6,0x59,0x00,0xFB,0x54,0x7E,0x79,0x86,0x99,0x55,0xE8,0x8A,
		0x58,0x7E,0xAA,0x35,0x8F,0x44,0x28,0x32,0xB4,0x95,0x73,0xB1,
		0xE4,0x63,0x68,0xD0,0xA8,0x95,0x55,0xE7,0x7B,0x35,0xF8,0x92,
		0x9B,0x15,0x31,0x0C,0x1D,0xDA,0xA3,0x11,0x83,0x63,0x5E,0x47,
		0x01,0x63,0x88,0xDE,0xB2,0x72,0x1B,0x6E,0x07,0xD1,0x68,0xC2,
		0x01,0x70,0x29,0xCC,0x07,0x97,0xF6,0x75,0x7C,0x88,0x2F,0xAD,
		0xBE,0x5B,0x99,0x4D,0x93,0xE6,0xF9,0xA8,0xDF,0x1B,0x95,0xA1,
		0x84,0xCF,0x3C,0xC6,0x9A,0x42,0x41,0xEB,0x21,0x27,0x57,0x95,
		0xBE,0x2C,0xD5,0x0B,0x8E,0xCD,0xCF,0x56,0xB6,0x65,0x71,0x06,
		0x6E,0xCA,0xF8,0x4E,0xB7,0xD0,0x94,0x10,0xC0,0x45,0xE1,0x86,
		0xC6,0x40,0xFF,0xE4,0x71,0xFE,0x3C,0x96,0x4F,0xE4,0xB3,0x97,
		0xEA,0x64,0xEB,0xAF,0x13,0x0A,0xCC,0xA3,0xCD,0xDE,0x41,0x83,
		0x4A,0x67,0x9F,0x0D,0x60,0x81,0x18,0xFD,0x91,0x90,0x47,0x99,
		0x5D,0x49,0x4A,0xE3,0xBD,0x8B,0xEE,0x54,0xCC,0x64,0xB9,0x5D,
		0xE0,0x19,0x5A,0x58,0x2D,0xB4,0xB9,0x55,0xC3,0x4E,0x83,0xEB,
		0xCB,0x15,0x22,0x83,0xD0,0x3C,0x6F,0x90,0x95,0x44,0xE2,0x0D,
		0xDA,0xD2,0x6C,0x53,
		};
	static unsigned char dh2236_g[]={
		0x02,
		};
	DH *dh;

	if ((dh=DH_new()) == NULL) return(NULL);
	dh->p=BN_bin2bn(dh2236_p,sizeof(dh2236_p),NULL);
	dh->g=BN_bin2bn(dh2236_g,sizeof(dh2236_g),NULL);
	if ((dh->p == NULL) || (dh->g == NULL))
		{ DH_free(dh); return(NULL); }
	return(dh);
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stdout);
        fflush(stdout);
        return;
    }
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stdout);
        fflush(stdout);
        return;
    }
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        cout << "Private key does not match the public certificate" << endl;
        return;
    }
}

void handler(int _signal)
{
	if (pn_signalresponder)
	{
		switch (_signal)
		{
			case SIGINT:
				pn_signalresponder->on_sigint();
				return;
			
			case SIGTERM:
				pn_signalresponder->on_sigterm();
				return;
			
			case SIGHUP:
				pn_signalresponder->on_sighup();
				return;
		}
	}
}

void GenerateGuid(char *guidStr)
{
	char *pGuidStr = guidStr;
	int i;
			
	srand(static_cast<unsigned int> (time(NULL)));  /*Randomize based on time.*/
	
	/*Data1 - 8 characters.*/
	for(i = 0; i < 8; i++, pGuidStr++)
		((*pGuidStr = (rand() % 16)) < 10) ? *pGuidStr += 48 : *pGuidStr += 55;		 
	
	/*Data2 - 4 characters.*/
	*pGuidStr++ = '-';  
	for(i = 0; i < 4; i++, pGuidStr++)
		((*pGuidStr = (rand() % 16)) < 10) ? *pGuidStr += 48 : *pGuidStr += 55;
		
	/*Data3 - 4 characters.*/
	*pGuidStr++ = '-';  
	for(i = 0; i < 4; i++, pGuidStr++)
		((*pGuidStr = (rand() % 16)) < 10) ? *pGuidStr += 48 : *pGuidStr += 55;

	/*Data4 - 4 characters.*/
	*pGuidStr++ = '-';  
	for(i = 0; i < 4; i++, pGuidStr++)
		((*pGuidStr = (rand() % 16)) < 10) ? *pGuidStr += 48 : *pGuidStr += 55;
	
	*pGuidStr = '\0';		   
}

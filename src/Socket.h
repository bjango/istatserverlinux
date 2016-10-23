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

#ifndef _SOCKET_H
#define _SOCKET_H

#include "config.h"

#include <stdlib.h>
#include <iostream>
#include <string>
#include <sys/param.h>

#include "Clientset.h"
#include "Conf.h"
#include "Stats.h"
#include "Responses.h"

#include "Certificate.h"

class Socket
{
    public:
        Socket(const std::string & _address, unsigned int _port) : listener(true), port(_port), address(_address) {}
        Socket(int _socket, std::string _address, unsigned int _port) : secure(false), socket(_socket), listener(false), port(_port), address(_address) {}
        
        int get_id() { return socket; }
        bool get_listener() { return listener; }
        unsigned int get_port() { return port; }
        std::string get_address() { return address; }
        std::string get_description();
        int send(std::string data);
        int receive(ClientSet * _clients, Config * _config, Stats * _stats);
        Socket accept();
        int listen();

        void initClient(int s);
        void startReading();
        void close();        

        int startSSL();
        SSL *ssl;
        SSL_CTX *sslContext;

        std::string _serverUUID;
        std::string _uuid;
        std::string _name;     
        long _session;
        int _protocol;
        int _sslEnabled;
        bool isServer;
        double lastRequest;
        std::string readbuf;
        bool secure;
        bool debugLogging;

    private:
        int socket;
        bool listener;
        unsigned int port;
        std::string address;
        void parse(std::string _data, ClientSet * _clients, Config * _config, Stats * _stats);
};

#endif

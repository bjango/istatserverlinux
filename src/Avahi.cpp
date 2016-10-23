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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_LIBAVAHI_CLIENT

#include <iostream>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/thread-watch.h>

#include "Avahi.h"
#include <stdio.h>

using namespace std;

static AvahiEntryGroup *avahiGroup = NULL;
static AvahiClient* avahiClient = NULL;
static AvahiThreadedPoll* avahiThread = NULL;

void AvahiGroupCallback(AvahiEntryGroup *_avahiGroup, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata)
{
	assert(_avahiGroup == avahiGroup || avahiGroup == NULL);
	avahiGroup = _avahiGroup;
	switch (state)
	{
		case AVAHI_ENTRY_GROUP_ESTABLISHED:
			{
				cout << "avahi successfully established" << endl;
			}
			break;
		case AVAHI_ENTRY_GROUP_COLLISION:
		case AVAHI_ENTRY_GROUP_FAILURE:
			{
				cout << "avahi entry group failure: " << avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(avahiGroup))) << endl;
			}
			break;
		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
			{
			}
			break;
	}
	return;
}

void AvahiCreateService(AvahiClient *avahiClient, AvahiPublisher *publisher)
{
	assert(avahiClient);
	if (!avahiGroup)
	{
		avahiGroup = avahi_entry_group_new(avahiClient, AvahiGroupCallback, NULL);
		if (!avahiGroup)
		{
			cout << "avahi_entry_group_new() failed: " << avahi_strerror(avahi_client_errno(avahiClient)) << endl;
			return;
		}
	}
	if (avahi_entry_group_is_empty(avahiGroup))
	{
		AvahiStringList *txt = NULL;
		txt = avahi_string_list_add_pair(txt, "name", publisher->name.c_str());
		txt = avahi_string_list_add_pair(txt, "model", "");

		char buffer [8];
		int n = sprintf(buffer, "%d", publisher->protocol);
		if(n > 0){
			txt = avahi_string_list_add_pair(txt, "protocol", buffer);
		}

		char platformBuffer [8];
		n = sprintf(platformBuffer, "%d", publisher->platform);
		if(n > 0){
			txt = avahi_string_list_add_pair(txt, "platform", platformBuffer);
		}

		int ret = avahi_entry_group_add_service_strlst(
				avahiGroup,
				AVAHI_IF_UNSPEC,
				AVAHI_PROTO_UNSPEC,
				(AvahiPublishFlags)0,
				publisher->uuid.c_str(),
				"_istatserver._tcp",
				"local.",
				NULL,
				publisher->port,
				txt
				);
		if (ret < 0)
		{
			cout << "avahi_entry_group_add_service() failed: " << avahi_strerror(ret) << endl;
			return;
		}
		ret = avahi_entry_group_commit(avahiGroup);
		if (ret < 0)
		{
			cout << "avahi_entry_group_commit() failed: " <<  avahi_strerror(ret) << endl;
			return;
		}
	}
	return;
}

void AvahiCallback(AvahiClient *avahiClient, AvahiClientState state, void *data)
{
	assert(avahiClient);

	AvahiPublisher *publisher = static_cast<AvahiPublisher*>(data);

	switch (state)
	{
		case AVAHI_CLIENT_S_RUNNING:
			{
				AvahiCreateService(avahiClient, publisher);
			}
			break;
		case AVAHI_CLIENT_FAILURE:
			{
				cout << "avahi client failure: " <<  avahi_strerror(avahi_client_errno(avahiClient)) << endl;
			}
			break;
		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			{
				if (avahiGroup)
					avahi_entry_group_reset(avahiGroup);
			}
			break;
		case AVAHI_CLIENT_CONNECTING:
			{
			}
			break;
	}
}

void AvahiPublisher::stop()
{
	if(avahiClient == NULL)
		return;
	
	cout << "Closing avahi" << endl;
	avahi_client_free(avahiClient);
}

int AvahiPublisher::publish_service()
{
	avahiThread = avahi_threaded_poll_new(); /* lost to never be found.. */
	if (!avahiThread)
	{
		cout << "avahi_threaded_poll_new failed" << endl;
		return -1;
	}
	int error;
	avahiClient = avahi_client_new(
			avahi_threaded_poll_get(avahiThread),
			(AvahiClientFlags)0,
			AvahiCallback,
			this,
			&error
			);
	if (!avahiClient)
	{
		cout << "avahi_client_new failed: " <<  avahi_strerror(error) << endl;
		return -1;
	}
	if (avahi_threaded_poll_start(avahiThread) < 0)
	{
		cout << "avahi_threaded_poll_start failed" << endl;
		return -1;
	}
	return 0;
}

#endif


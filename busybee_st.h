// Copyright (c) 2012, Cornell University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of BusyBee nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef busybee_st_h_
#define busybee_st_h_

// STL
#include <tr1/memory>

// po6
#include <po6/net/ipaddr.h>
#include <po6/net/location.h>
#include <po6/net/socket.h>

// e
#include <e/buffer.h>
#include <e/lockfree_fifo.h>
#include <e/lockfree_hash_map.h>
#include <e/striped_lock.h>

// BusyBee
#include <busybee_returncode.h>

class busybee_st
{
    public:
        busybee_st(const po6::net::ipaddr& ip);
        ~busybee_st() throw ();

    public:
        busybee_returncode send(const po6::net::location& to,
                                std::auto_ptr<e::buffer> msg);
        busybee_returncode recv(po6::net::location* from,
                                std::auto_ptr<e::buffer>* msg);

    private:
        class channel;
        class message;
        class pending;

    private:
        busybee_returncode get_channel(const po6::net::location& to,
                                       channel** chan,
                                       uint32_t* chantag);
        busybee_returncode get_channel(po6::net::socket* to,
                                       channel** chan,
                                       uint32_t* chantag);
        int add_descriptor(int fd);
        void postpone_event(channel* chan);
        int receive_event(int*fd, uint32_t* events);
        // Remove the channel and set the resources to be freed.
        void work_close(channel* chan);
        // Read from the socket, and set it up for further reading if necessary.
        //
        // It will return true if "from", "msg", "res" should be returned to the
        // user, and false otherwise.
        //
        // This may call work_close.
        bool work_read(channel* chan,
                       po6::net::location* from,
                       std::auto_ptr<e::buffer>* msg,
                       busybee_returncode* res);
        // Write to the socket, and set it up for further writing if necessary.
        // If no other messages are buffered, write msg, otherwise buffer msg,
        // and write the buffered messages.
        //
        // It will return true if progress was made, and false if there is an
        // error to report (using *res).
        //
        // This may call work_close.
        bool work_write(channel* chan,
                        busybee_returncode* res);

    private:
        po6::io::fd m_epoll;
        e::striped_lock<po6::threads::mutex> m_connectlocks;
        e::lockfree_hash_map<po6::net::location, std::pair<int, uint32_t>, po6::net::location::hash> m_locations;
        e::lockfree_fifo<message> m_incoming;
        std::vector<std::tr1::shared_ptr<channel> > m_channels;
        e::lockfree_fifo<pending> m_postponed;
};

#endif // busybee_st_h_
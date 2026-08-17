/*
 * Copyright (c) 2024 The Regents of the gem5-vulcan study.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Shared helper for resolving the context id to tag a tag-lookup key with.
 * Factored out of base_set_assoc.hh's accessBlock() and BaseCache::allocate()
 * (base.cc), which previously duplicated the same ternary.
 */

#ifndef __MEM_CACHE_TAGS_CONTEXT_KEY_HH__
#define __MEM_CACHE_TAGS_CONTEXT_KEY_HH__

#include "base/types.hh"
#include "mem/packet.hh"
#include "mem/request.hh"

namespace gem5
{

/**
 * Resolve the context id to tag a tag-lookup/allocation key with. Context-
 * aware indexing policies (e.g. a domain-keyed skewed cache) use this to
 * distinguish which security domain issued the request; policies that don't
 * key off per-request context simply ignore the field.
 */
inline ContextID
contextIdFor(const PacketPtr pkt)
{
    // Only trust the requestor's context id on a fully-formed request
    // packet; a default/sentinel command means this packet was
    // synthesized internally rather than carrying real requestor
    // context, so fall back to InvalidContextID in that case.
    return (pkt->cmd == MemCmd::InvalidCmd) ? pkt->req->contextId()
                                            : InvalidContextID;
}

} // namespace gem5

#endif // __MEM_CACHE_TAGS_CONTEXT_KEY_HH__

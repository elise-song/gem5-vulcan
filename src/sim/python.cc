/*
 * Copyright 2019 Google, Inc.
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

#include "pybind11/pybind11.h"
#include "mem/cache/tags/indexing_policies/scatter_associative.hh"
#include "sim/init.hh"
#include "sim/port.hh"

namespace gem5
{

namespace
{

// Experiment hook for the ScatterCache defense: force the active security
// domain for every ScatterAssociative-indexed cache. Called from a config
// between attack phases (e.g. prime/probe run as the attacker domain, the
// victim access as a different domain) so that a traffic-generator workload,
// whose requests carry no contextId, can still drive DISTINCT per-domain
// address->set mappings. Mirrors the reference PL defense's cacheLock hook.
void
scatterSetDomain(int64_t domain)
{
    ScatterAssociative::setActiveDomainAll((ContextID)domain);
}

// Drop the override and fall back to per-request contextId as the domain.
void
scatterClearDomain()
{
    ScatterAssociative::clearActiveDomainAll();
}

void
sim_pybind(pybind11::module_ &m_internal)
{
    pybind11::module_ m = m_internal.def_submodule("sim");
    pybind11::class_<
        Port, std::unique_ptr<Port, pybind11::nodelete>>(m, "Port")
        .def("bind", &Port::bind)
        .def("name", &Port::name)
        ;
    m.def("scatterSetDomain", &scatterSetDomain,
          "Force the active ScatterCache security domain (experiment hook)");
    m.def("scatterClearDomain", &scatterClearDomain,
          "Clear the ScatterCache active-domain override");
}
EmbeddedPyBind embed_("sim", &sim_pybind);

} // anonymous namespace
} // namespace gem5

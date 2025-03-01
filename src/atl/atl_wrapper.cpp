/*
 Copyright 2016-2020 Intel Corporation
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
     http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple.h"
#include "atl/util/pm/pmi_rt/pmi_simple.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable.h"
#include "atl/ofi/atl_ofi.hpp"
#include "atl/mpi/atl_mpi.hpp"
#include "atl_wrapper.h"
#include "common/global/global.hpp"
#include "exec/exec.hpp"
#include "util/pm/pmi_resizable_rt/pmi_resizable_simple_internal.h"

static std::list<std::shared_ptr<iatl>> transports{};
static ccl_executor* executor;

atl_attr_t atl_wrapper::attr = {
    1, /* ep_count */
    1, /* enable_shm */
    64, /* tag_bits */
    0xFFFFFFFFFFFFFFFF, /* max_tag */
    0, /* enable_rma */
    0, /* max_order_waw_size */
    0, /* sync_coll */
    0 /* extra_ep */
};

void atl_wrapper::set_internal_env(const atl_attr_t& attr) {
    auto transport_type = ccl::global_data::env().atl_transport;

    if (transport_type == ccl_atl_mpi)
        atl_mpi::atl_set_env(attr);
    else if (transport_type == ccl_atl_ofi)
        atl_ofi::atl_set_env(attr);
}

void atl_wrapper::set_exec(ccl_executor* exec) {
    executor = exec;
}

atl_wrapper::atl_wrapper() {
    auto transport_type = ccl::global_data::env().atl_transport;

    char* pm_type_str;
    switch (transport_type) {
        case ccl_atl_ofi:
            pm_type_str = getenv(PM_TYPE);
            if (pm_type_str) {
                if (strstr(pm_type_str, PM_RT_VAL_SIMPLE)) {
                    pmi = std::unique_ptr<ipmi>(new pmi_simple());
                }
                else if (strstr(pm_type_str, PM_RT_VAL_RESIZABLE)) {
                    std::shared_ptr<ikvs_wrapper> k(new internal_kvs());
                    pmi = std::unique_ptr<ipmi>(new pmi_resizable(k));
                }
                else {
                    LOG_ERROR("Unknown %s: %s\n", PM_TYPE, pm_type_str);
                }
            }
            else {
                pmi = std::unique_ptr<ipmi>(new pmi_simple());
            }
            transport = std::shared_ptr<iatl>(new atl_ofi());
            break;
        case ccl_atl_mpi: transport = std::shared_ptr<iatl>(new atl_mpi()); break;
        default: LOG_ERROR("Unsupported yet"); break;
    }

    init_transport();
}

atl_wrapper::atl_wrapper(std::shared_ptr<ikvs_wrapper> k) {
    auto transport_type = ccl::global_data::env().atl_transport;

    char* pm_type_str;
    switch (transport_type) {
        case ccl_atl_ofi:
            pm_type_str = getenv(PM_TYPE);
            if (pm_type_str) {
                if (strstr(pm_type_str, PM_RT_VAL_SIMPLE)) {
                    pmi = std::unique_ptr<ipmi>(new pmi_simple());
                }
                else if (strstr(pm_type_str, PM_RT_VAL_RESIZABLE)) {
                    pmi = std::unique_ptr<ipmi>(new pmi_resizable(k));
                }
                else {
                    LOG_ERROR("Unknown %s: %s\n", PM_TYPE, pm_type_str);
                }
            }
            else {
                pmi = std::unique_ptr<ipmi>(new pmi_simple());
            }
            transport = std::shared_ptr<iatl>(new atl_ofi());
            break;
        case ccl_atl_mpi: transport = std::shared_ptr<iatl>(new atl_mpi()); break;
        default: LOG_ERROR("Unsupported yet"); break;
    }

    init_transport();
}

atl_wrapper::atl_wrapper(int total_rank_count,
                         const std::vector<int>& ranks,
                         std::shared_ptr<ikvs_wrapper> k) {
    auto transport_type = ccl::global_data::env().atl_transport;

    switch (transport_type) {
        case ccl_atl_ofi: {
            size_t transorts_count = transports.size();
            std::shared_ptr<internal_kvs> kvs;
            if ((kvs = std::dynamic_pointer_cast<internal_kvs>(k)) != nullptr) {
                pmi = std::unique_ptr<ipmi>(
                    new pmi_resizable_simple_internal(total_rank_count, ranks, kvs));
            }
            else {
                pmi = std::unique_ptr<ipmi>(new pmi_resizable_simple(total_rank_count, ranks, k));
            }

            if (pmi->get_local_thread_idx() == 0) {
                transports.push_back(std::shared_ptr<iatl>(new atl_ofi()));
            }
            //TODO: Rework it on barrier
            while (transorts_count == transports.size()) {
                ccl_yield(ccl::global_data::env().yield_type);
            }
            static std::mutex memory_mutex;
            {
                std::lock_guard<std::mutex> lock(memory_mutex);
                transport = transports.back();
            }
        } break;
        case ccl_atl_mpi: transport = std::shared_ptr<iatl>(new atl_mpi()); break;
        default: LOG_ERROR("Unsupported yet"); break;
    }

    init_transport();
}
void atl_wrapper::init_transport() {
    LOG_DEBUG("init ATL, requested ep_count ", attr.ep_count);
    static std::mutex memory_mutex;
    {
        std::lock_guard<std::mutex> lock(memory_mutex);
        if (!transport->is_inited()) {
            CCL_THROW_IF_NOT(
                transport->atl_init(nullptr, nullptr, &attr, nullptr, pmi) == ATL_STATUS_SUCCESS,
                "failed to initialize ATL");
        }
    }
    eps = transport->atl_get_eps();
    tag = std::unique_ptr<ccl_atl_tag>(new ccl_atl_tag(attr.tag_bits, attr.max_tag));

    if (pmi) {
        threads_per_process = pmi->get_threads_per_process();
        ranks_per_process = pmi->get_ranks_per_process();
        rank = pmi->get_rank();
        size = pmi->get_size();
    }
    else {
        threads_per_process = 1;
        ranks_per_process = 1;
        rank = static_cast<atl_mpi*>(transport.get())->get_rank();
        size = static_cast<atl_mpi*>(transport.get())->get_size();
    }

    if (rank == 0) {
        tag->print();
        LOG_INFO("atl-parameters:");
        LOG_INFO("  ep_count: ", attr.ep_count);
        LOG_INFO("  enable_shm: ", attr.enable_shm);
        LOG_INFO("  enable_rma: ", attr.enable_rma);
        LOG_INFO("  max_order_waw_size: ", attr.max_order_waw_size);
        LOG_INFO("  sync_coll: ", attr.sync_coll);
        LOG_INFO("  extra_ep: ", attr.extra_ep);
    }

    if ((!pmi) || (pmi && pmi->get_local_thread_idx() == 0)) {
        if (!executor->are_workers_started()) {
            atl_proc_coord_t* coord = atl_get_proc_coord();
            if (rank < coord->local_count)
                LOG_INFO("start workers for local process [",
                         coord->local_idx,
                         ":",
                         coord->local_count,
                         "]");
            executor->start_workers(coord->local_idx, coord->local_count);
        }
    }
}

atl_wrapper::~atl_wrapper() {
    static std::mutex memory_mutex;
    std::lock_guard<std::mutex> lock(memory_mutex);
    transports.remove(transport);
    tag.reset();
}

/***************************************************************************
 *  Copyright 2020 Codeplay Software Limited
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  For your convenience, a copy of the License has been included in this
 *  repository.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 **************************************************************************/

#include "nvidia/cudnn_eltwise.hpp"
#include "nvidia/sycl_cuda_scoped_context.hpp"
#include "nvidia/sycl_cuda_stream.hpp"
#include "sycl/sycl_buffer_memory_storage.hpp"

namespace dnnl {
namespace impl {
namespace cuda {

status_t cudnn_eltwise_fwd_t::execute(const exec_ctx_t &ctx) const {
    memory_desc_wrapper wrap(pd()->src_md());
    if (wrap.size() == 0) { return status::success; }

    cuda::sycl_cuda_stream_t *cuda_stream
            = utils::downcast<cuda::sycl_cuda_stream_t *>(ctx.stream());

    return cuda_stream->interop_task([&](cl::sycl::handler &cgh) {
        auto src_acc = CTX_IN_ACCESSOR(DNNL_ARG_SRC);
        auto dst_acc = CTX_OUT_ACCESSOR(DNNL_ARG_DST);

        cgh.interop_task([=](cl::sycl::interop_handler ih) {
            std::vector<void *> args;
            auto &sycl_engine = *utils::downcast<sycl_cuda_engine_t *>(
                    cuda_stream->engine());
            auto sc = cuda_sycl_scoped_context_handler_t(sycl_engine);
            auto handle = cuda_stream->get_cudnn_handle();

            args.push_back(sc.memory<void *>(ih, src_acc));
            args.push_back(sc.memory<void *>(ih, dst_acc));

            pd()->eltwise_fwd_impl->execute(handle, args.data(), args.size());
        });
    });
}

status_t cudnn_eltwise_bwd_t::execute(const exec_ctx_t &ctx) const {
    memory_desc_wrapper wrap(pd()->src_md());
    if (wrap.size() == 0) { return status::success; }

    cuda::sycl_cuda_stream_t *cuda_stream
            = utils::downcast<cuda::sycl_cuda_stream_t *>(ctx.stream());

    return cuda_stream->interop_task([&](cl::sycl::handler &cgh) {
        auto src_acc = CTX_IN_ACCESSOR(DNNL_ARG_SRC);
        auto diff_dst_acc = CTX_IN_ACCESSOR(DNNL_ARG_DIFF_DST);
        auto diff_src_acc = CTX_OUT_ACCESSOR(DNNL_ARG_DIFF_SRC);

        cgh.interop_task([=](cl::sycl::interop_handler ih) {
            std::vector<void *> args;
            auto &sycl_engine = *utils::downcast<sycl_cuda_engine_t *>(
                    cuda_stream->engine());
            auto sc = cuda_sycl_scoped_context_handler_t(sycl_engine);
            auto handle = cuda_stream->get_cudnn_handle();

            args.push_back(sc.memory<void *>(ih, src_acc));
            args.push_back(sc.memory<void *>(ih, diff_dst_acc));
            args.push_back(sc.memory<void *>(ih, diff_src_acc));

            pd()->eltwise_bwd_impl->execute(handle, args.data(), args.size());
        });
    });
}

} // namespace cuda
} // namespace impl
} // namespace dnnl

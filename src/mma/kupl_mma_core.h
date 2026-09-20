/*
 * Copyright (c) 2026 Huawei Technologies Co., Ltd. All Rights Reserved.
 *
 * KUPL is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *        http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#pragma once

#include <arm_sme.h>

// compiler helpers
#ifndef kupl_always_inline
#define kupl_always_inline inline __attribute__((always_inline))
#define kupl_likely(_x) __builtin_expect(!!(_x), 1)
#define kupl_unlikely(_x) __builtin_expect(!!(_x), 0)
#endif

// SME function attributes
//
// 使用规则（按函数操作选择）：
//   操作 ZA tile（读+写，如 mma A*B+C->ZA）             -> KUPL_MMA_INOUT
//   操作 ZA tile（只写，如 trans mem->ZA->mem）        -> KUPL_MMA_OUT
//   操作 ZA tile（只读，如 store ZA->mem）             -> KUPL_MMA_IN
//   streaming-compatible（不操作 ZA，如 Sv*/Tensor 算术）-> KUPL_MMA_COMP
#define KUPL_MMA_IN __arm_streaming __arm_in("za")
#define KUPL_MMA_OUT __arm_streaming __arm_out("za")
#define KUPL_MMA_INOUT __arm_streaming __arm_inout("za")
#define KUPL_MMA_COMP __arm_streaming_compatible

namespace kupl {

namespace tensor {

// tile shape constants
static constexpr int M_1 = 1;
static constexpr int M_2 = 2;
static constexpr int M_4 = 4;
static constexpr int M_16 = 16;
static constexpr int M_32 = 32;
static constexpr int N_1 = 1;
static constexpr int N_2 = 2;
static constexpr int N_4 = 4;
static constexpr int N_16 = 16;
static constexpr int N_32 = 32;
static constexpr int N_64 = 64;
static constexpr int K_512 = 512;
static constexpr int K_2 = 2;
static constexpr int K_4 = 4;
static constexpr int ZA_TILE_0 = 0;
static constexpr int ZA_TILE_1 = 1;
static constexpr int ZA_TILE_2 = 2;
static constexpr int ZA_TILE_3 = 3;
static constexpr int F64_TILE_8 = 8;
static constexpr int F64_TILE_8X2 = 16;
static constexpr int F64_TILE_8X3 = 24;
static constexpr int BF16_TILE_32 = 32;
static constexpr int BF16_TILE_32X2 = 64;
static constexpr int BF16_TILE_32X3 = 96;
static constexpr int F32_TILE_16 = 16;
static constexpr int F32_TILE_16X2 = 32;
static constexpr int F32_TILE_16X3 = 48;
static constexpr int INT8_TILE_64 = 64;
static constexpr int INT8_TILE_64X2 = 128;
static constexpr int INT8_TILE_64X3 = 192;
static constexpr int INT32_TILE_16 = 16;
static constexpr int INT32_TILE_16X2 = 32;
static constexpr int INT32_TILE_16X3 = 48;

// TiledCallFunc: call_mma/call_copy container (friend of TiledMma/TiledCopy)
class TiledCallFunc {
public:
    template <typename MmaAtom, typename Shape>
    friend class TiledMma;
    template <typename CopyAtom, typename Shape>
    friend class TiledCopy;

private:
    template <int size_m, int size_n, typename StrideA, typename StrideB, typename StrideC, typename dtypeA,
              typename dtypeB, typename dtypeC>
    static void call_mma(dtypeA *A, dtypeB *B, dtypeC *C, int size_k) KUPL_MMA_INOUT;

    template <int size_m, int size_n, typename StrideA, typename StrideB, typename dtypeA, typename dtypeB>
    static void call_mma(dtypeA *A, dtypeB *B, int size_k) KUPL_MMA_INOUT;

    template <typename CopyAtom, typename StrideD, typename dtypeD>
    static void call_copy(dtypeD *data) KUPL_MMA_IN;

    template <typename CopyAtom, typename dtypeD, typename dtypeS>
    static void call_copy(dtypeD *dst, dtypeS *src, int size_m, int size_n) KUPL_MMA_OUT;
};

} // namespace tensor

} // namespace kupl

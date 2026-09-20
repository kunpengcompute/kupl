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

#include <arm_bf16.h>
#include <arm_sve.h>
#include <stdexcept>
#include <type_traits>

#include "kupl_mma_core.h"

namespace kupl {

namespace tensor {

// SVE vector type mapping: scalar type -> fixed-length SVE vector type (512-bit)
template <typename T>
struct vector_type;

template <>
struct vector_type<float> {
    using type = svfloat32_t __attribute__((arm_sve_vector_bits(512)));
};
template <>
struct vector_type<bfloat16_t> {
    using type = svbfloat16_t __attribute__((arm_sve_vector_bits(512)));
};
template <typename T>
using vector_type_t = typename vector_type<T>::type;

// Predicate generator: svptrue_for<dtype>()
template <typename dtype>
kupl_always_inline svbool_t svptrue_for() KUPL_MMA_COMP
{
    if constexpr (std::is_same_v<dtype, float>)
        return svptrue_b32();
    else if constexpr (std::is_same_v<dtype, bfloat16_t>)
        return svptrue_b16();
    else
        static_assert(dependent_false_v<dtype>, "Unsupported dtype for svptrue_for");
}

template <typename dtype>
class PtrEngine {
    dtype *ptr_;

public:
    using element_type = dtype;
    PtrEngine(dtype *ptr) : ptr_(ptr)
    {
        if (kupl_unlikely(ptr == nullptr)) {
            throw std::invalid_argument("The original ptr for KUPL tensor is nullptr");
        }
    }
    dtype *data() const
    {
        return ptr_;
    }
};

template <typename dtype>
class VectorEngine {
    vector_type_t<dtype> vec_;

public:
    using element_type = dtype;
    VectorEngine() : vec_{} {}
    VectorEngine(vector_type_t<dtype> v) : vec_(v) {}
    vector_type_t<dtype> data() const
    {
        return vec_;
    }
};

template <typename dtype>
class MatrixEngine {
public:
    using element_type = dtype;
};

// Physical ZA tile; Tile is compile-time (ACLE svread_hor_za32_m needs it as an immediate).
template <typename dtype, int Tile>
class MatrixTileEngine {
    uint32_t slice_;

public:
    using element_type = dtype;

    MatrixTileEngine() : slice_(0) {}
    MatrixTileEngine(uint32_t slice) : slice_(slice) {}
    uint32_t data() const
    {
        return slice_;
    }
};

template <typename Engine>
struct is_vector_engine : std::false_type {};
template <typename dtype>
struct is_vector_engine<VectorEngine<dtype>> : std::true_type {};
template <typename Engine>
constexpr bool is_vector_engine_v = is_vector_engine<Engine>::value;

template <typename Engine>
struct is_matrix_engine : std::false_type {};
template <typename dtype>
struct is_matrix_engine<MatrixEngine<dtype>> : std::true_type {};
template <typename Engine>
constexpr bool is_matrix_engine_v = is_matrix_engine<Engine>::value;

template <typename Engine>
struct is_matrix_tile_engine : std::false_type {};
template <typename dtype, int Tile>
struct is_matrix_tile_engine<MatrixTileEngine<dtype, Tile>> : std::true_type {};
template <typename Engine>
constexpr bool is_matrix_tile_engine_v = is_matrix_tile_engine<Engine>::value;

template <typename Engine>
struct is_ptr_engine : std::false_type {};
template <typename dtype>
struct is_ptr_engine<PtrEngine<dtype>> : std::true_type {};
template <typename Engine>
constexpr bool is_ptr_engine_v = is_ptr_engine<Engine>::value;

struct SvLoad {
    template <typename dtype, typename VecType = vector_type_t<dtype>>
    kupl_always_inline static VecType apply(const dtype *ptr) KUPL_MMA_COMP
    {
        auto pg = svptrue_for<dtype>();
        if constexpr (std::is_same_v<VecType, vector_type_t<float>>)
            return svld1_f32(pg, ptr);
        else if constexpr (std::is_same_v<VecType, vector_type_t<bfloat16_t>>)
            return svld1_bf16(pg, ptr);
        else
            static_assert(dependent_false_v<VecType>, "Unsupported VecType for SvLoad");
    }
};

struct SvStore {
    template <typename dtype, typename VecType>
    kupl_always_inline static void apply(dtype *ptr, VecType vec) KUPL_MMA_COMP
    {
        auto pg = svptrue_for<dtype>();
        if constexpr (std::is_same_v<VecType, vector_type_t<float>>)
            svst1_f32(pg, ptr, vec);
        else if constexpr (std::is_same_v<VecType, vector_type_t<bfloat16_t>>)
            svst1_bf16(pg, ptr, vec);
        else
            static_assert(dependent_false_v<VecType>, "Unsupported VecType for SvStore");
    }
};

} // namespace tensor
} // namespace kupl

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

#include <cmath>
#include <type_traits>

#include "kupl_mma_engine.h"
#include "kupl_mma_layout.h"

namespace kupl {

namespace tensor {

template <typename Layout, typename dtype>
struct is_valid_vector_layout : std::false_type {};

template <>
struct is_valid_vector_layout<Layout<Shape<Int<16>>, Stride<Int<1>>>, float> : std::true_type {};

template <>
struct is_valid_vector_layout<Layout<Shape<Int<32>>, Stride<Int<1>>>, bfloat16_t> : std::true_type {};

template <typename Layout, typename dtype>
constexpr bool is_valid_vector_layout_v = is_valid_vector_layout<Layout, dtype>::value;

template <typename Layout, typename dtype>
struct is_valid_matrix_layout : std::false_type {};

template <>
struct is_valid_matrix_layout<Layout<Shape<Int<16>, Int<64>>, Stride<Int<64>, Int<1>>>, float> : std::true_type {};

template <>
struct is_valid_matrix_layout<Layout<Shape<Int<32>, Int<32>>, Stride<Int<32>, Int<1>>>, float> : std::true_type {};

template <>
struct is_valid_matrix_layout<Layout<Shape<Int<32>, Int<16>>, Stride<Int<16>, Int<1>>>, double> : std::true_type {};

template <>
struct is_valid_matrix_layout<Layout<Shape<Int<16>, Int<64>>, Stride<Int<64>, Int<1>>>, int32_t> : std::true_type {};

template <>
struct is_valid_matrix_layout<Layout<Shape<Int<32>, Int<32>>, Stride<Int<32>, Int<1>>>, int32_t> : std::true_type {};

template <typename Layout, typename dtype>
constexpr bool is_valid_matrix_layout_v = is_valid_matrix_layout<Layout, dtype>::value;

// Physical ZA tile layout per dtype (SVL=512). f32/za32 -> 16x16 row-major.
template <typename dtype>
struct matrix_tile_layout;

template <>
struct matrix_tile_layout<float> {
    using type = Layout<Shape<Int<16>, Int<16>>, Stride<Int<16>, Int<1>>>;
    static type make()
    {
        return {};
    }
};

template <>
struct matrix_tile_layout<double> {
    using type = Layout<Shape<Int<8>, Int<8>>, Stride<Int<8>, Int<1>>>;
    static type make()
    {
        return {};
    }
};

template <>
struct matrix_tile_layout<int32_t> {
    using type = Layout<Shape<Int<16>, Int<16>>, Stride<Int<16>, Int<1>>>;
    static type make()
    {
        return {};
    }
};
template <typename dtype>
using matrix_tile_layout_t = typename matrix_tile_layout<dtype>::type;

// One ZA tile row = one SVE vector. f32 -> 16 elem, stride 1.
template <typename dtype>
struct matrix_row_layout;

template <>
struct matrix_row_layout<float> {
    using type = Layout<Shape<Int<16>>, Stride<Int<1>>>;
    static type make()
    {
        return {};
    }
};

template <>
struct matrix_row_layout<double> {
    using type = Layout<Shape<Int<8>>, Stride<Int<1>>>;
    static type make()
    {
        return {};
    }
};

template <>
struct matrix_row_layout<int32_t> {
    using type = Layout<Shape<Int<16>>, Stride<Int<1>>>;
    static type make()
    {
        return {};
    }
};
template <typename dtype>
using matrix_row_layout_t = typename matrix_row_layout<dtype>::type;

template <typename Engine, typename Layout>
class Tensor;

template <typename Engine, typename Layout>
class TensorAdd;

template <typename Engine, typename Layout>
class TensorScalMul;

template <typename Engine, typename Layout>
class Tensor {
    Engine engine_;
    Layout layout_;

public:
    using element_type = typename Engine::element_type;

    Tensor(Engine engine, Layout layout) : engine_(engine), layout_(layout) {}

    auto data() const
    {
        return engine_.data();
    }
    const Layout &layout() const
    {
        return layout_;
    }

    template <class Coord>
    decltype(auto) operator()(Coord const &coord) const
    {
        if constexpr (is_ptr_engine_v<Engine>) {
            if constexpr (has_underscore<Coord>) {
                auto [sliced_layout, offset] = slice_and_offset(coord, layout_);
                return Tensor<PtrEngine<element_type>, decltype(sliced_layout)>{
                    PtrEngine<element_type>(engine_.data() + offset), sliced_layout};
            } else {
                return engine_.data()[layout_(coord)];
            }
        } else if constexpr (is_matrix_tile_engine_v<Engine>) {
            static_assert(has_underscore<Coord>, "MatrixTileEngine only supports slice with _; no element access");
            auto slice = (uint32_t)coord.template get<0>();
            return Tensor<Engine, matrix_row_layout_t<element_type>>(Engine(slice),
                                                                     matrix_row_layout<element_type>::make());
        } else if constexpr (is_matrix_engine_v<Engine>) {
            static_assert(dependent_false_v<Engine>,
                          "MatrixEngine does not support operator(); use .tile() to obtain a MatrixTileTensor");
        } else {
            static_assert(dependent_false_v<Engine>, "operator() is not supported for this engine");
        }
    }

    // Descend to a physical tile; Tile must be compile-time (Int<N>) for ACLE.
    template <typename TileInt, typename E = Engine>
    auto tile(TileInt)
    {
        if constexpr (is_matrix_engine_v<E>) {
            constexpr int Tile = TileInt::val;
            return Tensor<MatrixTileEngine<element_type, Tile>, matrix_tile_layout_t<element_type>>(
                MatrixTileEngine<element_type, Tile>(), matrix_tile_layout<element_type>::make());
        } else {
            static_assert(is_matrix_engine_v<E>, "tile() is only available for MatrixEngine tensors");
        }
    }

    template <typename E = Engine, std::enable_if_t<is_vector_engine_v<E>, int> = 0>
    Tensor &operator=(const Tensor &other)
    {
        engine_ = other.engine_;
        return *this;
    }

    template <typename E = Engine, std::enable_if_t<is_ptr_engine_v<E>, int> = 0>
    Tensor &operator=(const TensorAdd<Engine, Layout> &add)
    {
        static_assert(decltype(layout_.shape())::size_v() == 2, "TensorAdd assignment requires a 2D tensor");
        auto lhs = add.lhs_;
        auto rhs = add.rhs_;
        const int m = std::remove_reference_t<decltype(layout_.shape().template get<0>())>::val;
        const int n = std::remove_reference_t<decltype(layout_.shape().template get<1>())>::val;
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                int idx = layout_(Coord<int, int>(i, j));
                engine_.data()[idx] = lhs.data()[idx] + rhs.data()[idx];
            }
        }
        return *this;
    }

    template <typename E = Engine, std::enable_if_t<is_ptr_engine_v<E>, int> = 0>
    Tensor &operator=(const TensorScalMul<Engine, Layout> &scalmul)
    {
        static_assert(decltype(layout_.shape())::size_v() == 2, "TensorScalMul assignment requires a 2D tensor");
        auto lhs = scalmul.lhs_;
        auto rhs = scalmul.rhs_;
        const int m = std::remove_reference_t<decltype(layout_.shape().template get<0>())>::val;
        const int n = std::remove_reference_t<decltype(layout_.shape().template get<1>())>::val;
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                int idx = layout_(Coord<int, int>(i, j));
                engine_.data()[idx] = lhs.data()[idx] * rhs;
            }
        }
        return *this;
    }
};

template <typename Engine, typename Layout>
class TensorAdd {
public:
    Tensor<Engine, Layout> lhs_, rhs_;
    TensorAdd(Tensor<Engine, Layout> lhs, Tensor<Engine, Layout> rhs) : lhs_(lhs), rhs_(rhs) {}
};

template <typename Engine, typename Layout>
class TensorScalMul {
public:
    Tensor<Engine, Layout> lhs_;
    typename Engine::element_type rhs_;
    TensorScalMul(Tensor<Engine, Layout> lhs, typename Engine::element_type rhs) : lhs_(lhs), rhs_(rhs) {}
};

template <typename Engine, typename Layout, std::enable_if_t<is_ptr_engine_v<Engine>, int> = 0>
TensorAdd<Engine, Layout> operator+(const Tensor<Engine, Layout> &lhs, const Tensor<Engine, Layout> &rhs)
{
    return TensorAdd<Engine, Layout>{lhs, rhs};
}

template <typename Engine, typename Layout, std::enable_if_t<is_ptr_engine_v<Engine>, int> = 0>
TensorScalMul<Engine, Layout> operator*(const Tensor<Engine, Layout> &lhs, const typename Engine::element_type &rhs)
{
    return TensorScalMul<Engine, Layout>{lhs, rhs};
}

template <typename Engine, typename Layout, std::enable_if_t<is_ptr_engine_v<Engine>, int> = 0>
TensorScalMul<Engine, Layout> operator*(const typename Engine::element_type &lhs, const Tensor<Engine, Layout> &rhs)
{
    return TensorScalMul<Engine, Layout>{rhs, lhs};
}

template <typename Engine, typename Layout>
inline auto exp2f(Tensor<Engine, Layout> x)
{
    static_assert(std::is_same_v<typename Engine::element_type, float>, "exp2f only supports float");

    auto pg = svptrue_for<float>();
    auto native = x.data();

    vector_type_t<float> z1 = svreinterpret_f32_u32(svdup_u32(1212161984));
    vector_type_t<float> z2 = svreinterpret_f32_u32(svdup_u32(1060205250));
    vector_type_t<float> z3 = svreinterpret_f32_u32(svdup_u32(1047920148));
    vector_type_t<float> z5 = svreinterpret_f32_u32(svdup_u32(1123811328));

    auto z4 = native + z1;
    z1 = z4 - z1;
    z1 = native - z1;
    z2 = svmla_x(pg, z2, z1, z3);
    z3 = svreinterpret_f32_u32(svdup_u32(1065353216));
    z1 = svmad_x(pg, z1, z2, z3);
    z2 = svexpa(svreinterpret_u32_f32(z4));
    z1 = svmul_x(pg, z2, z1);

    auto poverflow = svacge(pg, native, z5);
    if (__builtin_expect(svptest_any(pg, poverflow), 0)) {
        svbool_t pgt = svcmpgt(pg, native, z5);
        z1 = svsel(pgt, svdup_f32(INFINITY), z1);
        svbool_t plt = svcmplt(pg, native, svneg_x(pg, z5));
        z1 = svsel(plt, svdup_f32(0), z1);
    }

    return Tensor<Engine, Layout>(Engine(z1), x.layout());
}

template <typename dtype, typename Layout>
Tensor<PtrEngine<dtype>, Layout> make_tensor(dtype *ptr, Layout layout)
{
    return Tensor<PtrEngine<dtype>, Layout>{PtrEngine<dtype>(ptr), layout};
}

template <typename dtype, typename Layout, std::enable_if_t<is_valid_vector_layout_v<Layout, dtype>, int> = 0>
Tensor<VectorEngine<dtype>, Layout> make_tensor(Layout layout)
{
    return Tensor<VectorEngine<dtype>, Layout>{VectorEngine<dtype>(), layout};
}

template <typename dtype, typename Layout, std::enable_if_t<is_valid_matrix_layout_v<Layout, dtype>, int> = 0>
Tensor<MatrixEngine<dtype>, Layout> make_tensor(Layout layout)
{
    return Tensor<MatrixEngine<dtype>, Layout>{MatrixEngine<dtype>(), layout};
}

// Zero the ZA accumulator backing a MatrixEngine tensor.
template <typename dtype, typename Layout>
kupl_always_inline void clear([[maybe_unused]] Tensor<MatrixEngine<dtype>, Layout> tensor) KUPL_MMA_OUT
{
    svzero_za();
}

template <auto Policy, typename dtypeS, typename ShapeS, typename StrideS>
kupl_always_inline void prefetch_impl(const Tensor<PtrEngine<dtypeS>, Layout<ShapeS, StrideS>> &src) KUPL_MMA_COMP
{
    if constexpr (sizeof(dtypeS) == 1)
        svprfb(svptrue_b8(), src.data(), Policy);
    else if constexpr (sizeof(dtypeS) == 2)
        svprfh(svptrue_b16(), src.data(), Policy);
    else if constexpr (sizeof(dtypeS) == 4)
        svprfw(svptrue_b32(), src.data(), Policy);
    else if constexpr (sizeof(dtypeS) == 8)
        svprfd(svptrue_b64(), src.data(), Policy);
    else
        static_assert(dependent_false_v<dtypeS>, "Unsupported dtype for prefetch");
}

} // namespace tensor
} // namespace kupl

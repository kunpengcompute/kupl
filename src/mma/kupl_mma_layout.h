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

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "kupl_mma_core.h"

namespace kupl {

namespace tensor {

template <typename T>
constexpr bool dependent_false_v = false;

template <typename T, T V>
class Val {
public:
    static constexpr T val = V;
};

template <int V>
class Int : public Val<int, V> {};

template <int V>
class Ops : public Val<int, V> {};

class Underscore : public Int<-1> {};

template <typename T>
class is_int_constexpr : public std::false_type {};

template <int V>
class is_int_constexpr<Int<V>> : public std::true_type {};

template <>
class is_int_constexpr<Underscore> : public std::true_type {};

template <typename... Args>
class kupl_tuple {
public:
    using tuple_type = std::tuple<Args...>;

    kupl_tuple(Args &...args) : data_(args...) {}

    template <typename... AA>
    kupl_tuple(AA &&...args) : data_(std::forward<AA>(args)...)
    {
    }

    static constexpr size_t size_v()
    {
        return size_;
    }

    template <size_t Index>
    constexpr auto &get()
    {
        static_assert(Index < size_, "Index out of bounds");
        return std::template get<Index>(data_);
    }

    template <size_t Index>
    constexpr const auto &get() const
    {
        static_assert(Index < size_, "Index out of bounds");
        return std::template get<Index>(data_);
    }

private:
    std::tuple<Args...> data_;
    static constexpr size_t size_ = sizeof...(Args);
};

template <typename... Args>
class Coord : public kupl_tuple<Args...> {
public:
    Coord(Args... args) : kupl_tuple<Args...>(args...) {}
};

template <typename... Args>
class Shape : public kupl_tuple<Args...> {
public:
    Shape() : kupl_tuple<Args...>(Args{}...) {}
    Shape(Args... args) : kupl_tuple<Args...>(args...) {}
};

template <typename... Args>
class Stride : public kupl_tuple<Args...> {
public:
    Stride() : kupl_tuple<Args...>(Args{}...) {}
    Stride(Args... args) : kupl_tuple<Args...>(args...) {}
};

template <typename T, typename Tuple>
struct tuple_contains : std::false_type {};

template <typename T, typename... Args>
struct tuple_contains<T, kupl_tuple<Args...>> : std::disjunction<std::is_same<T, Args>...> {};

template <typename T, typename... Args>
struct tuple_contains<T, Coord<Args...>> : std::disjunction<std::is_same<T, Args>...> {};

template <typename T, typename Tuple>
constexpr bool has_elem = tuple_contains<T, Tuple>::value;

template <typename Tuple>
constexpr bool has_underscore = has_elem<Underscore, Tuple>;

template <typename... Args>
Coord<Args...> make_coord(Args &&...args)
{
    return Coord<Args...>{std::forward<Args>(args)...};
}

template <typename... Args>
Shape<Args...> make_shape(Args &&...args)
{
    return Shape<Args...>{std::forward<Args>(args)...};
}

template <typename... Args>
Stride<Args...> make_stride(Args &&...args)
{
    return Stride<Args...>{std::forward<Args>(args)...};
}

template <typename Coord, typename Shape, typename Stride>
constexpr auto crd2idx(Coord coord, Shape shape, Stride stride);

template <typename Coord, typename Layout>
constexpr auto slice_and_offset(Coord coord, Layout layout);

template <typename Shape, typename Stride>
class Layout {
public:
    // Default args enable default-construction (static Shapes/Strides value-init correctly,
    // e.g. Layout<Shape<Int<16>>, Stride<Int<1>>>{} yields 16-elem / stride-1). Matches cute's
    // Layout(Shape const& = {}, Stride const& = {}) convention.
    Layout(Shape shape = {}, Stride stride = {}) : shape_(shape), stride_(stride) {}

    Shape shape() const
    {
        return shape_;
    }

    Stride stride() const
    {
        return stride_;
    }

    template <class Coord>
    auto operator()(Coord const &coord) const
    {
        static_assert(!has_underscore<Coord>, "layout() does not support slice");
        return crd2idx(coord, shape_, stride_);
    }

private:
    Shape shape_;
    Stride stride_;
};

template <typename Shape, typename Stride>
Layout<Shape, Stride> make_layout(Shape shape, Stride stride)
{
    return Layout<Shape, Stride>{shape, stride};
}

template <size_t I = 0, typename Coord, typename Shape, typename... Collected>
constexpr auto slice_shape_impl(const Coord &coord, const Shape &shape, Collected &&...collected)
{
    constexpr size_t size = Coord::size_v();
    if constexpr (I >= size) {
        return make_shape(std::forward<Collected>(collected)...);
    } else {
        if constexpr (std::is_same_v<std::decay_t<decltype(coord.template get<I>())>, Underscore>) {
            return slice_shape_impl<I + 1>(coord, shape, std::forward<Collected>(collected)...,
                                           shape.template get<I>());
        } else {
            return slice_shape_impl<I + 1>(coord, shape, std::forward<Collected>(collected)...);
        }
    }
}

template <typename Coord, typename Shape>
constexpr auto slice_shape(Coord coord, Shape shape)
{
    static_assert(Coord::size_v() == Shape::size_v(), "coord and stride must have the same size");
    return slice_shape_impl(coord, shape);
}

template <size_t I = 0, typename Coord, typename Stride, typename... Collected>
constexpr auto slice_stride_impl(const Coord &coord, const Stride &stride, Collected &&...collected)
{
    constexpr size_t size = Coord::size_v();
    static_assert(size == 2, "slice_stride requires a 2D stride");
    if constexpr (I >= size) {
        return make_stride(std::forward<Collected>(collected)...);
    } else {
        if constexpr (std::is_same_v<std::decay_t<decltype(coord.template get<I>())>, Underscore>) {
            return slice_stride_impl<I + 1>(coord, stride, std::forward<Collected>(collected)...,
                                            stride.template get<I>());
        } else {
            return slice_stride_impl<I + 1>(coord, stride, std::forward<Collected>(collected)...);
        }
    }
}

template <typename Coord, typename Stride>
constexpr auto slice_stride(Coord coord, Stride stride)
{
    static_assert(Coord::size_v() == Stride::size_v(), "coord and stride must have the same size");
    return slice_stride_impl(coord, stride);
}

template <typename Coord, typename Shape, typename Stride, size_t... I>
constexpr auto crd2idx_impl(const Coord &coord, const Shape &shape, const Stride &stride, std::index_sequence<I...>)
{
    ptrdiff_t result = 0;

    auto process_dim = [&](auto index_constant) {
        constexpr size_t Index = decltype(index_constant)::value;
        if constexpr (is_int_constexpr<std::decay_t<decltype(coord.template get<Index>())>>::value) {
            static_assert(((std::remove_reference_t<decltype(coord.template get<Index>())>::val <
                            std::remove_reference_t<decltype(shape.template get<Index>())>::val)),
                          "Coord must be less than Shape at the same index");
            result += (std::is_same_v<std::decay_t<decltype(coord.template get<Index>())>, Underscore> ?
                           0 :
                           std::remove_reference_t<decltype(coord.template get<Index>())>::val) *
                      std::remove_reference_t<decltype(stride.template get<Index>())>::val;
        } else {
            if (kupl_likely(coord.template get<Index>() >= 0 &&
                            coord.template get<Index>() <
                                std::remove_reference_t<decltype(shape.template get<Index>())>::val)) {
                result +=
                    coord.template get<Index>() * std::remove_reference_t<decltype(stride.template get<Index>())>::val;
            }
        }
    };

    (process_dim(std::integral_constant<size_t, I>{}), ...);

    return result;
}

template <typename Coord, typename Shape, typename Stride>
constexpr auto crd2idx(Coord coord, Shape shape, Stride stride)
{
    static_assert(coord.size_v() == shape.size_v(), "Coord and Shape must have the same size");
    static_assert(coord.size_v() == stride.size_v(), "Coord and Stride must have the same size");
    return crd2idx_impl(coord, shape, stride, std::make_index_sequence<coord.size_v()>{});
}

template <typename Coord, typename Layout>
constexpr auto slice_and_offset(Coord coord, Layout layout)
{
    auto sliced_shape = slice_shape(coord, layout.shape());
    auto sliced_stride = slice_stride(coord, layout.stride());
    auto sliced_idx = crd2idx(coord, layout.shape(), layout.stride());
    return std::make_pair(make_layout(sliced_shape, sliced_stride), sliced_idx);
}

} // namespace tensor
} // namespace kupl

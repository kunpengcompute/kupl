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

#include <stdexcept>

#include "kupl_mma_core.h"
#include "kupl_mma_tensor.h"

namespace kupl {

namespace tensor {

typedef enum copy_atom {
    KP36_32x1_F64_TRANS_RM2CM = 0,
    KP36_1x16_F64_TRANS_CM2RM,
    KP36_32x2_BF16_TRANS_RM2ZZ,
    KP36_2x32_BF16_TRANS_CM2NN,
    KP36_16x2_BF16_TRANS_RM2ZZ,
    KP36_2x64_BF16_TRANS_CM2NN,
    KP36_16x1_BF16_TRANS_RM2CM,
    KP36_1x64_BF16_TRANS_CM2RM,
    KP36_16x4_INT8_TRANS_RM2ZZ,
    KP36_4x64_INT8_TRANS_CM2NN,
    KP36_32x4_INT8_TRANS_RM2ZZ,
    KP36_4x32_INT8_TRANS_CM2NN,
    KP36_PREFETCH_L1STRM,
    KP36_PREFETCH_L1KEEP,
    KP36_PREFETCH_L2STRM,
    KP36_PREFETCH_L2KEEP,
    KP36_VEC_LOAD,
    KP36_VEC_STORE,
    KP36_VEC_STORE_NT,
    KP36_VEC_COPY_HOR,
    KP36_32x16_F64_STORE,
    KP36_16x64_F32_STORE,
    KP36_16x64_INT32_STORE,
    KP36_32x32_INT32_STORE,
    KP36_32x32_F32_STORE
} copy_atom_t;

template <typename CopyAtom, typename Shape>
class TiledCopy;

template <typename CopyAtom, typename Shape>
TiledCopy<CopyAtom, Shape> make_tiled_copy([[maybe_unused]] CopyAtom copy_atom, [[maybe_unused]] Shape atom_shape)
{
    return TiledCopy<CopyAtom, Shape>{};
}

template <typename TiledCopy, typename EngineD, typename LayoutD, typename EngineS, typename LayoutS>
void copy(TiledCopy tiled_copy, Tensor<EngineD, LayoutD> &dst, Tensor<EngineS, LayoutS> src) KUPL_MMA_INOUT
{
    tiled_copy.call(dst, src);
}

template <typename TiledCopy, typename EngineS, typename LayoutS>
void copy(TiledCopy tiled_copy, Tensor<EngineS, LayoutS> src)
{
    tiled_copy.call(src);
}

template <typename CopyAtom>
struct StoreTraits;

/*
 * TransTraits<CopyAtom>: trans CopyAtom 属性表（mem -> ZA -> mem 转置）
 *
 * M/N:          CopyAtom 的 tile 形状（编译期常量，用于算 size_m/size_n）
 * AlignmentM/N: 该维度的对齐粒度
 *               0 = 不检查（标量转置，或该维度步长已自动满足）
 *               非 0 = 要求 (Traits::X * AtomShapeX) % AlignmentX == 0
 *
 * 为什么需要 alignment:
 *   SVE 向量化转置（svld1_hor_za / svst1_ver_za）循环步长 = 向量寄存器宽度对应的元素数。
 *   非 0 对齐保证 size 是步长整数倍，否则越界 load / 转置偏移错位 / 末尾 tile 数据丢失。
 *
 * AlignmentX 取值 = 512bit / sizeof(dtype):  F64=8, BF16=32, INT8=64
 * 检查维度由 trans 方向决定:
 *   RM2ZZ (Row-Major -> ZA) 按行 load -> 检查 N（列数对齐）
 *   CM2NN (Col-Major -> ZA) 按列 load -> 检查 M（行数对齐）
 *   RM2CM/CM2RM 标量转置        -> AlignmentM = AlignmentN = 0（不检查）
 */
template <typename CopyAtom>
struct TransTraits;

template <>
struct StoreTraits<Ops<KP36_32x16_F64_STORE>> {
    static constexpr int M = M_32, N = N_16;
};
template <>
struct StoreTraits<Ops<KP36_16x64_F32_STORE>> {
    static constexpr int M = M_16, N = N_64;
};
template <>
struct StoreTraits<Ops<KP36_16x64_INT32_STORE>> {
    static constexpr int M = M_16, N = N_64;
};
template <>
struct StoreTraits<Ops<KP36_32x32_INT32_STORE>> {
    static constexpr int M = M_32, N = N_32;
};
template <>
struct StoreTraits<Ops<KP36_32x32_F32_STORE>> {
    static constexpr int M = M_32, N = N_32;
};
template <>
struct TransTraits<Ops<KP36_32x1_F64_TRANS_RM2CM>> {
    static constexpr int M = M_32, N = N_1, AlignmentM = 0, AlignmentN = F64_TILE_8;
    static constexpr const char *AlignMsg = "KP36_32x1_F64_TRANS_RM2CM: n must be multiple of 8";
};
template <>
struct TransTraits<Ops<KP36_1x16_F64_TRANS_CM2RM>> {
    static constexpr int M = M_1, N = N_16, AlignmentM = F64_TILE_8, AlignmentN = 0;
    static constexpr const char *AlignMsg = "KP36_1x16_F64_TRANS_CM2RM: m must be multiple of 8";
};
template <>
struct TransTraits<Ops<KP36_16x2_BF16_TRANS_RM2ZZ>> {
    static constexpr int M = M_16, N = N_2, AlignmentM = 0, AlignmentN = BF16_TILE_32;
    static constexpr const char *AlignMsg = "KP36_16x2_BF16_TRANS_RM2ZZ: n must be multiple of 32";
};
template <>
struct TransTraits<Ops<KP36_2x64_BF16_TRANS_CM2NN>> {
    static constexpr int M = M_2, N = N_64, AlignmentM = BF16_TILE_32, AlignmentN = 0;
    static constexpr const char *AlignMsg = "KP36_2x64_BF16_TRANS_CM2NN: m must be multiple of 32";
};
template <>
struct TransTraits<Ops<KP36_16x1_BF16_TRANS_RM2CM>> {
    static constexpr int M = M_16, N = N_1, AlignmentM = 0, AlignmentN = 0;
    static constexpr const char *AlignMsg = "scalar trans, no alignment requirement";
};
template <>
struct TransTraits<Ops<KP36_1x64_BF16_TRANS_CM2RM>> {
    static constexpr int M = M_1, N = N_64, AlignmentM = 0, AlignmentN = 0;
    static constexpr const char *AlignMsg = "scalar trans, no alignment requirement";
};
template <>
struct TransTraits<Ops<KP36_16x4_INT8_TRANS_RM2ZZ>> {
    static constexpr int M = M_16, N = N_4, AlignmentM = 0, AlignmentN = INT8_TILE_64;
    static constexpr const char *AlignMsg = "KP36_16x4_INT8_TRANS_RM2ZZ: n must be multiple of 64";
};
template <>
struct TransTraits<Ops<KP36_4x64_INT8_TRANS_CM2NN>> {
    static constexpr int M = M_4, N = N_64, AlignmentM = INT8_TILE_64, AlignmentN = 0;
    static constexpr const char *AlignMsg = "KP36_4x64_INT8_TRANS_CM2NN: m must be multiple of 64";
};
template <>
struct TransTraits<Ops<KP36_32x4_INT8_TRANS_RM2ZZ>> {
    static constexpr int M = M_32, N = N_4, AlignmentM = 0, AlignmentN = INT8_TILE_64;
    static constexpr const char *AlignMsg = "KP36_32x4_INT8_TRANS_RM2ZZ: n must be multiple of 64";
};
template <>
struct TransTraits<Ops<KP36_4x32_INT8_TRANS_CM2NN>> {
    static constexpr int M = M_4, N = N_32, AlignmentM = INT8_TILE_64, AlignmentN = 0;
    static constexpr const char *AlignMsg = "KP36_4x32_INT8_TRANS_CM2NN: m must be multiple of 64";
};
template <>
struct TransTraits<Ops<KP36_32x2_BF16_TRANS_RM2ZZ>> {
    static constexpr int M = M_32, N = N_2, AlignmentM = 0, AlignmentN = BF16_TILE_32;
    static constexpr const char *AlignMsg = "KP36_32x2_BF16_TRANS_RM2ZZ: n must be multiple of 32";
};
template <>
struct TransTraits<Ops<KP36_2x32_BF16_TRANS_CM2NN>> {
    static constexpr int M = M_2, N = N_32, AlignmentM = BF16_TILE_32, AlignmentN = 0;
    static constexpr const char *AlignMsg = "KP36_2x32_BF16_TRANS_CM2NN: m must be multiple of 32";
};

template <typename CopyAtom>
struct PrefetchTraits;

template <>
struct PrefetchTraits<Ops<KP36_PREFETCH_L1STRM>> {
    static constexpr auto policy = SV_PLDL1STRM;
};
template <>
struct PrefetchTraits<Ops<KP36_PREFETCH_L1KEEP>> {
    static constexpr auto policy = SV_PLDL1KEEP;
};
template <>
struct PrefetchTraits<Ops<KP36_PREFETCH_L2STRM>> {
    static constexpr auto policy = SV_PLDL2STRM;
};
template <>
struct PrefetchTraits<Ops<KP36_PREFETCH_L2KEEP>> {
    static constexpr auto policy = SV_PLDL2KEEP;
};

template <typename CopyAtom, int AtomShapeM, int AtomShapeN>
class TiledCopy<CopyAtom, Shape<Int<AtomShapeM>, Int<AtomShapeN>>> {
public:
    template <typename TiledCopy, typename EngineD, typename LayoutD, typename EngineS, typename LayoutS>
    friend void copy(TiledCopy tiled_copy, Tensor<EngineD, LayoutD> &dst, Tensor<EngineS, LayoutS> src) KUPL_MMA_INOUT;

private:
    // store: ZA -> mem
    template <typename dtypeD, typename ShapeD, typename StrideD, typename dtypeS, typename ShapeS, typename StrideS>
    kupl_always_inline void call(Tensor<PtrEngine<dtypeD>, Layout<ShapeD, StrideD>> &dst,
                                 [[maybe_unused]] const Tensor<MatrixEngine<dtypeS>, Layout<ShapeS, StrideS>> &src)
        KUPL_MMA_IN
    {
        TiledCallFunc::call_copy<CopyAtom, StrideD, dtypeD>(dst.data());
    }
    // trans: mem -> ZA -> mem
    template <typename dtypeD, typename ShapeD, typename StrideD, typename dtypeS, typename ShapeS, typename StrideS>
    kupl_always_inline void call(Tensor<PtrEngine<dtypeD>, Layout<ShapeD, StrideD>> &dst,
                                 const Tensor<PtrEngine<dtypeS>, Layout<ShapeS, StrideS>> &src) KUPL_MMA_OUT
    {
        if constexpr (TransTraits<CopyAtom>::AlignmentM != 0 &&
                      (TransTraits<CopyAtom>::M * AtomShapeM) % TransTraits<CopyAtom>::AlignmentM != 0) {
            throw std::invalid_argument(TransTraits<CopyAtom>::AlignMsg);
        }
        if constexpr (TransTraits<CopyAtom>::AlignmentN != 0 &&
                      (TransTraits<CopyAtom>::N * AtomShapeN) % TransTraits<CopyAtom>::AlignmentN != 0) {
            throw std::invalid_argument(TransTraits<CopyAtom>::AlignMsg);
        }
        TiledCallFunc::call_copy<CopyAtom, dtypeD, dtypeS>(
            dst.data(), src.data(), TransTraits<CopyAtom>::M * AtomShapeM, TransTraits<CopyAtom>::N * AtomShapeN);
    }
};

template <typename CopyAtom, int AtomShapeM>
class TiledCopy<CopyAtom, Shape<Int<AtomShapeM>>> {
public:
    template <typename TiledCopy, typename EngineS, typename LayoutS>
    friend void copy(TiledCopy tiled_copy, Tensor<EngineS, LayoutS> src);

private:
    template <typename dtypeS, typename ShapeS, typename StrideS>
    kupl_always_inline void call(const Tensor<PtrEngine<dtypeS>, Layout<ShapeS, StrideS>> &src)
    {
        prefetch_impl<PrefetchTraits<CopyAtom>::policy>(src);
    }
};

template <int AtomShape>
class TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<AtomShape>>> {
public:
    template <typename EngineD, typename LayoutD, typename EngineS, typename LayoutS>
    friend void copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<AtomShape>>> tiled_copy, Tensor<EngineD, LayoutD> &dst,
                     Tensor<EngineS, LayoutS> src)
    {
        tiled_copy.call(dst, src);
    }

private:
    template <typename dtypeD, typename ShapeD, typename StrideD, typename dtypeS, typename ShapeS, typename StrideS>
    kupl_always_inline void call(Tensor<VectorEngine<dtypeD>, Layout<ShapeD, StrideD>> &dst,
                                 const Tensor<PtrEngine<dtypeS>, Layout<ShapeS, StrideS>> &src)
    {
        static_assert(std::is_same_v<dtypeD, dtypeS>, "SVLOAD requires same dtype for dst and src");
        auto loaded = Tensor<VectorEngine<dtypeD>, Layout<ShapeD, StrideD>>(
            VectorEngine<dtypeD>(SvLoad::apply<dtypeS>(src.data())), dst.layout());
        dst = loaded;
    }
};

template <int AtomShape>
class TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<AtomShape>>> {
public:
    template <typename EngineD, typename LayoutD, typename EngineS, typename LayoutS>
    friend void copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<AtomShape>>> tiled_copy, Tensor<EngineD, LayoutD> &dst,
                     Tensor<EngineS, LayoutS> src)
    {
        tiled_copy.call(dst, src);
    }

private:
    template <typename dtypeD, typename ShapeD, typename StrideD, typename dtypeS, typename ShapeS, typename StrideS>
    kupl_always_inline void call(Tensor<PtrEngine<dtypeD>, Layout<ShapeD, StrideD>> dst,
                                 const Tensor<VectorEngine<dtypeS>, Layout<ShapeS, StrideS>> &src)
    {
        static_assert(std::is_same_v<dtypeD, dtypeS>, "SVSTORE requires same dtype for dst and src");
        SvStore::apply<dtypeD>(dst.data(), src.data());
    }
};

template <int AtomShape>
class TiledCopy<Ops<KP36_VEC_COPY_HOR>, Shape<Int<AtomShape>>> {
public:
    template <typename EngineD, typename LayoutD, typename EngineS, typename LayoutS>
    friend kupl_always_inline void copy(TiledCopy<Ops<KP36_VEC_COPY_HOR>, Shape<Int<AtomShape>>> tiled_copy,
                                        Tensor<EngineD, LayoutD> &dst, Tensor<EngineS, LayoutS> src) KUPL_MMA_INOUT
    {
        tiled_copy.call(dst, src);
    }

private:
    template <typename dtypeD, typename ShapeD, typename StrideD, typename dtypeS, int Tile, typename ShapeS,
              typename StrideS>
    kupl_always_inline void call(Tensor<VectorEngine<dtypeD>, Layout<ShapeD, StrideD>> &dst,
                                 const Tensor<MatrixTileEngine<dtypeS, Tile>, Layout<ShapeS, StrideS>> &src) KUPL_MMA_IN
    {
        static_assert(std::is_same_v<dtypeD, float> && std::is_same_v<dtypeS, float>,
                      "KP36_VEC_COPY_HOR only supports float");
        auto pg = svptrue_for<float>();
        auto slice = src.data();
        auto v = svread_hor_za32_m(dst.data(), pg, Tile, slice);
        dst = Tensor<VectorEngine<dtypeD>, Layout<ShapeD, StrideD>>(VectorEngine<dtypeD>(v), dst.layout());
    }
    template <typename dtypeD, int Tile, typename ShapeD, typename StrideD, typename dtypeS, typename ShapeS,
              typename StrideS>
    kupl_always_inline void call(Tensor<MatrixTileEngine<dtypeD, Tile>, Layout<ShapeD, StrideD>> &dst,
                                 const Tensor<VectorEngine<dtypeS>, Layout<ShapeS, StrideS>> &src) KUPL_MMA_OUT
    {
        static_assert(std::is_same_v<dtypeD, float> && std::is_same_v<dtypeS, float>,
                      "KP36_VEC_COPY_HOR only supports float");
        auto pg = svptrue_for<float>();
        svwrite_hor_za32_m(Tile, dst.data(), pg, src.data());
    }
};

template <>
kupl_always_inline void
TiledCallFunc::call_copy<Ops<KP36_32x16_F64_STORE>, Stride<Int<16>, Int<1>>, double>(double *data) KUPL_MMA_IN
{
    const svbool_t p64 = svwhilelt_b64(0, 8);
    double *matd0 = data;
    double *matd1 = data + 128;
    double *matd2 = data + 256;
    double *matd3 = data + 384;
    for (uint32_t t = 0; t < 8; ++t) {
        svst1_hor_za64(0, t, p64, matd0);
        svst1_hor_za64(4, t, p64, matd0 + 8);
        svst1_hor_za64(1, t, p64, matd1);
        svst1_hor_za64(5, t, p64, matd1 + 8);
        svst1_hor_za64(2, t, p64, matd2);
        svst1_hor_za64(6, t, p64, matd2 + 8);
        svst1_hor_za64(3, t, p64, matd3);
        svst1_hor_za64(7, t, p64, matd3 + 8);
        matd0 += 16;
        matd1 += 16;
        matd2 += 16;
        matd3 += 16;
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_16x64_F32_STORE>, Stride<Int<64>, Int<1>>, float>(float *data)
    KUPL_MMA_IN
{
    const svbool_t p32 = svwhilelt_b32(0, 16);
    float *matd0 = data;
    float *matd1 = data + 16;
    float *matd2 = data + 32;
    float *matd3 = data + 48;
    for (uint32_t t = 0; t < 16; ++t) {
        svst1_hor_za32(0, t, p32, matd0);
        svst1_hor_za32(1, t, p32, matd1);
        svst1_hor_za32(2, t, p32, matd2);
        svst1_hor_za32(3, t, p32, matd3);
        matd0 += 64;
        matd1 += 64;
        matd2 += 64;
        matd3 += 64;
    }
}

template <>
kupl_always_inline void
TiledCallFunc::call_copy<Ops<KP36_16x64_INT32_STORE>, Stride<Int<64>, Int<1>>, int32_t>(int32_t *data) KUPL_MMA_IN
{
    const svbool_t p32 = svwhilelt_b32(0, 16);
    int32_t *matd0 = data;
    int32_t *matd1 = data + 16;
    int32_t *matd2 = data + 32;
    int32_t *matd3 = data + 48;
    for (uint32_t t = 0; t < 16; ++t) {
        svst1_hor_za32(0, t, p32, matd0);
        svst1_hor_za32(1, t, p32, matd1);
        svst1_hor_za32(2, t, p32, matd2);
        svst1_hor_za32(3, t, p32, matd3);
        matd0 += 64;
        matd1 += 64;
        matd2 += 64;
        matd3 += 64;
    }
}

template <>
kupl_always_inline void
TiledCallFunc::call_copy<Ops<KP36_32x32_INT32_STORE>, Stride<Int<32>, Int<1>>, int32_t>(int32_t *data) KUPL_MMA_IN
{
    const svbool_t p32 = svwhilelt_b32(0, 16);
    int32_t *matd0 = data;
    int32_t *matd1 = data + 16;
    int32_t *matd2 = data + 16 * 32;
    int32_t *matd3 = data + 16 * 32 + 16;
    for (uint32_t t = 0; t < 16; ++t) {
        svst1_hor_za32(0, t, p32, matd0);
        svst1_hor_za32(1, t, p32, matd1);
        svst1_hor_za32(2, t, p32, matd2);
        svst1_hor_za32(3, t, p32, matd3);
        matd0 += 32;
        matd1 += 32;
        matd2 += 32;
        matd3 += 32;
    }
}

template <>
kupl_always_inline void
TiledCallFunc::call_copy<Ops<KP36_32x32_F32_STORE>, Stride<Stride<Int<1>, Int<1024>>, Int<16>>, float>(float *data)
    KUPL_MMA_IN
{
    const svbool_t ptrue = svptrue_b16();
    float *matd0 = data;
    float *matd1 = data + 16 * 16;
    float *matd2 = data + 1024;
    float *matd3 = data + 1024 + 16 * 16;
    for (uint32_t t = 0; t < 16; ++t) {
        svst1_ver_za32(0, t, ptrue, matd0);
        svst1_ver_za32(1, t, ptrue, matd1);
        svst1_ver_za32(2, t, ptrue, matd2);
        svst1_ver_za32(3, t, ptrue, matd3);
        matd0 += 16;
        matd1 += 16;
        matd2 += 16;
        matd3 += 16;
    }
}

template <>
kupl_always_inline void
TiledCallFunc::call_copy<Ops<KP36_32x32_F32_STORE>, Stride<Stride<Int<1>, Int<2048>>, Int<16>>, float>(float *data)
    KUPL_MMA_IN
{
    const svbool_t ptrue = svptrue_b16();
    float *matd0 = data;
    float *matd1 = data + 16 * 16;
    float *matd2 = data + 2048;
    float *matd3 = data + 2048 + 16 * 16;
    for (uint32_t t = 0; t < 16; ++t) {
        svst1_ver_za32(0, t, ptrue, matd0);
        svst1_ver_za32(1, t, ptrue, matd1);
        svst1_ver_za32(2, t, ptrue, matd2);
        svst1_ver_za32(3, t, ptrue, matd3);
        matd0 += 16;
        matd1 += 16;
        matd2 += 16;
        matd3 += 16;
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_32x1_F64_TRANS_RM2CM>, double, double>(
    double *data_dst, double *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b64(0, F64_TILE_8);
    for (int tile_m = 0; tile_m < size_m; tile_m += M_32) {
        for (int tile_n = 0; tile_n < size_n; tile_n += F64_TILE_8) {
            // 横着写入向量寄存器
            double *data_src0 = data_src + tile_m * size_n + tile_n;
            double *data_src1 = data_src + (tile_m + F64_TILE_8) * size_n + tile_n;
            double *data_src2 = data_src + (tile_m + F64_TILE_8X2) * size_n + tile_n;
            double *data_src3 = data_src + (tile_m + F64_TILE_8X3) * size_n + tile_n;
            for (uint32_t t = 0; t < F64_TILE_8; ++t) {
                svld1_hor_za64(ZA_TILE_0, t, p, data_src0);
                svld1_hor_za64(ZA_TILE_1, t, p, data_src1);
                svld1_hor_za64(ZA_TILE_2, t, p, data_src2);
                svld1_hor_za64(ZA_TILE_3, t, p, data_src3);
                data_src0 += size_n;
                data_src1 += size_n;
                data_src2 += size_n;
                data_src3 += size_n;
            }

            // 竖着写回buffer
            double *data_dst0 = data_dst + tile_m * size_n + tile_n * M_32;
            double *data_dst1 = data_dst + tile_m * size_n + tile_n * M_32 + F64_TILE_8;
            double *data_dst2 = data_dst + tile_m * size_n + tile_n * M_32 + F64_TILE_8X2;
            double *data_dst3 = data_dst + tile_m * size_n + tile_n * M_32 + F64_TILE_8X3;
            for (uint32_t t = 0; t < F64_TILE_8; ++t) {
                svst1_ver_za64(ZA_TILE_0, t, p, data_dst0);
                svst1_ver_za64(ZA_TILE_1, t, p, data_dst1);
                svst1_ver_za64(ZA_TILE_2, t, p, data_dst2);
                svst1_ver_za64(ZA_TILE_3, t, p, data_dst3);
                data_dst0 += M_32;
                data_dst1 += M_32;
                data_dst2 += M_32;
                data_dst3 += M_32;
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_1x16_F64_TRANS_CM2RM>, double, double>(
    double *data_dst, double *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b64(0, F64_TILE_8);
    for (int tile_n = 0; tile_n < size_n; tile_n += N_16) {
        for (int tile_m = 0; tile_m < size_m; tile_m += F64_TILE_8) {
            // 横着写入向量寄存器
            double *data_src0 = data_src + tile_n * size_m + tile_m;
            double *data_src1 = data_src + (tile_n + F64_TILE_8) * size_m + tile_m;
            for (uint32_t t = 0; t < F64_TILE_8; ++t) {
                svld1_hor_za64(ZA_TILE_0, t, p, data_src0);
                svld1_hor_za64(ZA_TILE_1, t, p, data_src1);
                data_src0 += size_m;
                data_src1 += size_m;
            }

            // 竖着写回buffer
            double *data_dst0 = data_dst + tile_n * size_m + tile_m * N_16;
            double *data_dst1 = data_dst + tile_n * size_m + tile_m * N_16 + F64_TILE_8;
            for (uint32_t t = 0; t < F64_TILE_8; ++t) {
                svst1_ver_za64(ZA_TILE_0, t, p, data_dst0);
                svst1_ver_za64(ZA_TILE_1, t, p, data_dst1);
                data_dst0 += N_16;
                data_dst1 += N_16;
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_16x2_BF16_TRANS_RM2ZZ>, bfloat16_t, bfloat16_t>(
    bfloat16_t *data_dst, bfloat16_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b32(0, F32_TILE_16);
    for (int tile_m = 0; tile_m < size_m; tile_m += M_16) {
        for (int tile_n = 0; tile_n < size_n; tile_n += BF16_TILE_32) {
            // 横着写入向量寄存器
            bfloat16_t *data_src0 = data_src + tile_m * size_n + tile_n;
            for (uint32_t t = 0; t < F32_TILE_16; ++t) {
                svld1_hor_za32(ZA_TILE_0, t, p, reinterpret_cast<float *>(data_src0));
                data_src0 += size_n;
            }

            // 竖着写回buffer
            bfloat16_t *data_dst0 = data_dst + tile_m * size_n + tile_n * M_16;
            for (uint32_t t = 0; t < F32_TILE_16; ++t) {
                svst1_ver_za32(ZA_TILE_0, t, p, reinterpret_cast<float *>(data_dst0));
                data_dst0 += M_16 * N_2;
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_2x64_BF16_TRANS_CM2NN>, bfloat16_t, bfloat16_t>(
    bfloat16_t *data_dst, bfloat16_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b32(0, 16);
    for (int tile_n = 0; tile_n < size_n; tile_n += N_64) {
        for (int tile_m = 0; tile_m < size_m; tile_m += BF16_TILE_32) {
            // 横着写入向量寄存器
            bfloat16_t *data_src0 = data_src + tile_n * size_m + tile_m;
            bfloat16_t *data_src1 = data_src + (tile_n + F32_TILE_16) * size_m + tile_m;
            bfloat16_t *data_src2 = data_src + (tile_n + F32_TILE_16X2) * size_m + tile_m;
            bfloat16_t *data_src3 = data_src + (tile_n + F32_TILE_16X3) * size_m + tile_m;
            for (uint32_t t = 0; t < F32_TILE_16; ++t) {
                svld1_hor_za32(ZA_TILE_0, t, p, reinterpret_cast<float *>(data_src0));
                svld1_hor_za32(ZA_TILE_1, t, p, reinterpret_cast<float *>(data_src1));
                svld1_hor_za32(ZA_TILE_2, t, p, reinterpret_cast<float *>(data_src2));
                svld1_hor_za32(ZA_TILE_3, t, p, reinterpret_cast<float *>(data_src3));
                data_src0 += size_m;
                data_src1 += size_m;
                data_src2 += size_m;
                data_src3 += size_m;
            }

            // 竖着写回buffer
            bfloat16_t *data_dst0 = data_dst + tile_n * size_m + tile_m * N_64;
            bfloat16_t *data_dst1 = data_dst + tile_n * size_m + tile_m * N_64 + BF16_TILE_32;
            bfloat16_t *data_dst2 = data_dst + tile_n * size_m + tile_m * N_64 + BF16_TILE_32X2;
            bfloat16_t *data_dst3 = data_dst + tile_n * size_m + tile_m * N_64 + BF16_TILE_32X3;
            for (uint32_t t = 0; t < 16; ++t) {
                svst1_ver_za32(ZA_TILE_0, t, p, reinterpret_cast<float *>(data_dst0));
                svst1_ver_za32(ZA_TILE_1, t, p, reinterpret_cast<float *>(data_dst1));
                svst1_ver_za32(ZA_TILE_2, t, p, reinterpret_cast<float *>(data_dst2));
                svst1_ver_za32(ZA_TILE_3, t, p, reinterpret_cast<float *>(data_dst3));
                data_dst0 += N_64 * M_2;
                data_dst1 += N_64 * M_2;
                data_dst2 += N_64 * M_2;
                data_dst3 += N_64 * M_2;
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_32x2_BF16_TRANS_RM2ZZ>, bfloat16_t, bfloat16_t>(
    bfloat16_t *data_dst, bfloat16_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b32(0, F32_TILE_16);
    for (int tile_m = 0; tile_m < size_m; tile_m += M_32) {
        for (int tile_n = 0; tile_n < size_n; tile_n += BF16_TILE_32) {
            for (int m = 0; m < M_32; m += M_16) {
                // 横着写入向量寄存器
                bfloat16_t *data_src0 = data_src + tile_m * size_n + m * size_n + tile_n;
                for (uint32_t t = 0; t < F32_TILE_16; ++t) {
                    svld1_hor_za32(ZA_TILE_0, t, p, reinterpret_cast<float *>(data_src0));
                    data_src0 += size_n;
                }

                // 竖着写回buffer
                bfloat16_t *data_dst0 = data_dst + tile_m * size_n + tile_n * M_32 + m * 2;
                for (uint32_t t = 0; t < F32_TILE_16; ++t) {
                    svst1_ver_za32(ZA_TILE_0, t, p, reinterpret_cast<float *>(data_dst0));
                    data_dst0 += M_32 * N_2;
                }
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_2x32_BF16_TRANS_CM2NN>, bfloat16_t, bfloat16_t>(
    bfloat16_t *data_dst, bfloat16_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b32(0, 16);
    for (int tile_n = 0; tile_n < size_n; tile_n += N_32) {
        for (int tile_m = 0; tile_m < size_m; tile_m += BF16_TILE_32) {
            for (int n = 0; n < N_32; n += N_16) {
                // 横着写入向量寄存器
                bfloat16_t *data_src0 = data_src + tile_n * size_m + n * size_m + tile_m;
                for (uint32_t t = 0; t < F32_TILE_16; ++t) {
                    svld1_hor_za32(ZA_TILE_0, t, p, reinterpret_cast<float *>(data_src0));
                    data_src0 += size_m;
                }

                // 竖着写回buffer
                bfloat16_t *data_dst0 = data_dst + tile_n * size_m + tile_m * N_32 + n * 2;
                for (uint32_t t = 0; t < F32_TILE_16; ++t) {
                    svst1_ver_za32(ZA_TILE_0, t, p, reinterpret_cast<float *>(data_dst0));
                    data_dst0 += M_2 * N_32;
                }
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_16x1_BF16_TRANS_RM2CM>, bfloat16_t, bfloat16_t>(
    bfloat16_t *data_dst, bfloat16_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    // 该原子方法直接使用标量方式进行转置
    for (int tile_m = 0; tile_m < size_m; tile_m += M_16) {
        for (int n = 0; n < size_n; ++n) {
            for (int m = tile_m; m < tile_m + M_16; ++m) {
                data_dst[tile_m * size_n + n * M_16 + m] = data_src[m * size_n + n];
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_1x64_BF16_TRANS_CM2RM>, bfloat16_t, bfloat16_t>(
    bfloat16_t *data_dst, bfloat16_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    // 该原子方法直接使用标量方式进行转置
    for (int tile_n = 0; tile_n < size_n; tile_n += N_64) {
        for (int m = 0; m < size_m; ++m) {
            for (int n = tile_n; n < tile_n + N_64; ++n) {
                data_dst[tile_n * size_m + m * N_64 + n] = data_src[n * size_m + m];
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_16x4_INT8_TRANS_RM2ZZ>, int8_t, int8_t>(
    int8_t *data_dst, int8_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b32(0, INT32_TILE_16);
    for (int tile_m = 0; tile_m < size_m; tile_m += M_16) {
        for (int tile_n = 0; tile_n < size_n; tile_n += INT8_TILE_64) {
            // 横着写入向量寄存器
            int8_t *data_src0 = data_src + tile_m * size_n + tile_n;
            for (uint32_t t = 0; t < INT32_TILE_16; ++t) {
                svld1_hor_za32(ZA_TILE_0, t, p, reinterpret_cast<int32_t *>(data_src0));
                data_src0 += size_n;
            }

            // 竖着写回buffer
            int8_t *data_dst0 = data_dst + tile_m * size_n + tile_n * 16;
            for (uint32_t t = 0; t < INT32_TILE_16; ++t) {
                svst1_ver_za32(ZA_TILE_0, t, p, reinterpret_cast<int32_t *>(data_dst0));
                data_dst0 += M_16 * N_4;
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_4x64_INT8_TRANS_CM2NN>, int8_t, int8_t>(
    int8_t *data_dst, int8_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b32(0, INT32_TILE_16);
    for (int tile_n = 0; tile_n < size_n; tile_n += N_64) {
        for (int tile_m = 0; tile_m < size_m; tile_m += INT8_TILE_64) {
            // 横着写入向量寄存器
            int8_t *data_src0 = data_src + tile_n * size_m + tile_m;
            int8_t *data_src1 = data_src + (tile_n + INT32_TILE_16) * size_m + tile_m;
            int8_t *data_src2 = data_src + (tile_n + INT32_TILE_16X2) * size_m + tile_m;
            int8_t *data_src3 = data_src + (tile_n + INT32_TILE_16X3) * size_m + tile_m;
            for (uint32_t t = 0; t < INT32_TILE_16; ++t) {
                svld1_hor_za32(ZA_TILE_0, t, p, reinterpret_cast<int32_t *>(data_src0));
                svld1_hor_za32(ZA_TILE_1, t, p, reinterpret_cast<int32_t *>(data_src1));
                svld1_hor_za32(ZA_TILE_2, t, p, reinterpret_cast<int32_t *>(data_src2));
                svld1_hor_za32(ZA_TILE_3, t, p, reinterpret_cast<int32_t *>(data_src3));
                data_src0 += size_m;
                data_src1 += size_m;
                data_src2 += size_m;
                data_src3 += size_m;
            }

            // 竖着写回buffer
            int8_t *data_dst0 = data_dst + tile_n * size_m + tile_m * N_64;
            int8_t *data_dst1 = data_dst + tile_n * size_m + tile_m * N_64 + INT8_TILE_64;
            int8_t *data_dst2 = data_dst + tile_n * size_m + tile_m * N_64 + INT8_TILE_64X2;
            int8_t *data_dst3 = data_dst + tile_n * size_m + tile_m * N_64 + INT8_TILE_64X3;
            for (uint32_t t = 0; t < INT32_TILE_16; ++t) {
                svst1_ver_za32(ZA_TILE_0, t, p, reinterpret_cast<int32_t *>(data_dst0));
                svst1_ver_za32(ZA_TILE_1, t, p, reinterpret_cast<int32_t *>(data_dst1));
                svst1_ver_za32(ZA_TILE_2, t, p, reinterpret_cast<int32_t *>(data_dst2));
                svst1_ver_za32(ZA_TILE_3, t, p, reinterpret_cast<int32_t *>(data_dst3));
                data_dst0 += N_64 * M_4;
                data_dst1 += N_64 * M_4;
                data_dst2 += N_64 * M_4;
                data_dst3 += N_64 * M_4;
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_32x4_INT8_TRANS_RM2ZZ>, int8_t, int8_t>(
    int8_t *data_dst, int8_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b32(0, 16);
    for (int tile_m = 0; tile_m < size_m; tile_m += M_32) {
        for (int tile_n = 0; tile_n < size_n; tile_n += INT8_TILE_64) {
            // 横着写入向量寄存器
            int8_t *data_src0 = data_src + tile_m * size_n + tile_n;
            int8_t *data_src1 = data_src + (tile_m + INT32_TILE_16) * size_n + tile_n;
            for (uint32_t t = 0; t < INT32_TILE_16; ++t) {
                svld1_hor_za32(ZA_TILE_0, t, p, reinterpret_cast<int32_t *>(data_src0));
                svld1_hor_za32(ZA_TILE_1, t, p, reinterpret_cast<int32_t *>(data_src1));
                data_src0 += size_n;
                data_src1 += size_n;
            }

            // 竖着写回buffer
            int8_t *data_dst0 = data_dst + tile_m * size_n + tile_n * M_32;
            int8_t *data_dst1 = data_dst + tile_m * size_n + tile_n * M_32 + INT8_TILE_64;
            for (uint32_t t = 0; t < INT32_TILE_16; ++t) {
                svst1_ver_za32(0, t, p, reinterpret_cast<int32_t *>(data_dst0));
                svst1_ver_za32(1, t, p, reinterpret_cast<int32_t *>(data_dst1));
                data_dst0 += M_32 * N_4;
                data_dst1 += M_32 * N_4;
            }
        }
    }
}

template <>
kupl_always_inline void TiledCallFunc::call_copy<Ops<KP36_4x32_INT8_TRANS_CM2NN>, int8_t, int8_t>(
    int8_t *data_dst, int8_t *data_src, int size_m, int size_n) KUPL_MMA_OUT
{
    const svbool_t p = svwhilelt_b32(0, INT32_TILE_16);
    for (int tile_n = 0; tile_n < size_n; tile_n += N_32) {
        for (int tile_m = 0; tile_m < size_m; tile_m += INT8_TILE_64) {
            // 横着写入向量寄存器
            int8_t *data_src0 = data_src + tile_n * size_m + tile_m;
            int8_t *data_src1 = data_src + (tile_n + INT32_TILE_16) * size_m + tile_m;
            for (uint32_t t = 0; t < INT32_TILE_16; ++t) {
                svld1_hor_za32(ZA_TILE_0, t, p, reinterpret_cast<int32_t *>(data_src0));
                svld1_hor_za32(ZA_TILE_1, t, p, reinterpret_cast<int32_t *>(data_src1));
                data_src0 += size_m;
                data_src1 += size_m;
            }

            // 竖着写回buffer
            int8_t *data_dst0 = data_dst + tile_n * size_m + tile_m * N_32;
            int8_t *data_dst1 = data_dst + tile_n * size_m + tile_m * N_32 + INT8_TILE_64;
            for (uint32_t t = 0; t < INT32_TILE_16; ++t) {
                svst1_ver_za32(ZA_TILE_0, t, p, reinterpret_cast<int32_t *>(data_dst0));
                svst1_ver_za32(ZA_TILE_1, t, p, reinterpret_cast<int32_t *>(data_dst1));
                data_dst0 += N_32 * M_4;
                data_dst1 += N_32 * M_4;
            }
        }
    }
}

} // namespace tensor

} // namespace kupl

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

#include "kupl_mma_core.h"
#include "kupl_mma_tensor.h"

namespace kupl {

namespace tensor {

typedef enum mma_atom {
    KP36_32x16x1_F64F64F64 = 0,
    KP36_32x16x512_F64F64F64,
    KP36_16x64x2_BF16BF16F32,
    KP36_16x64x1_BF16BF16F32,
    KP36_32x32x2_BF16BF16F32,
    KP36_16x64x4_INT8INT8INT32,
    KP36_32x32x4_INT8INT8INT32
} mma_atom_t;

template <typename MmaAtom, typename Shape>
class TiledMma;

template <typename MmaAtom, typename Shape>
TiledMma<MmaAtom, Shape> make_tiled_mma([[maybe_unused]] MmaAtom mma_atom, [[maybe_unused]] Shape atom_shape)
{
    return TiledMma<MmaAtom, Shape>{};
}

template <typename TiledMma, typename EngineD, typename LayoutD, typename EngineA, typename LayoutA, typename EngineB,
          typename LayoutB, typename EngineC, typename LayoutC>
kupl_always_inline void mma(TiledMma tiled_mma, Tensor<EngineD, LayoutD> D, Tensor<EngineA, LayoutA> A,
                            Tensor<EngineB, LayoutB> B, Tensor<EngineC, LayoutC> C) KUPL_MMA_INOUT
{
    tiled_mma.call(D, A, B, C);
}

template <typename TiledMma, typename EngineD, typename LayoutD, typename EngineA, typename LayoutA, typename EngineB,
          typename LayoutB>
kupl_always_inline void mma(TiledMma tiled_mma, Tensor<EngineD, LayoutD> D, Tensor<EngineA, LayoutA> A,
                            Tensor<EngineB, LayoutB> B) KUPL_MMA_INOUT
{
    tiled_mma.call(D, A, B);
}

template <typename MmaAtom>
struct MmaAtomTraits;

template <>
struct MmaAtomTraits<Ops<KP36_32x16x512_F64F64F64>> {
    static constexpr int M = M_32, N = N_16, K = K_512;
};
template <>
struct MmaAtomTraits<Ops<KP36_32x16x1_F64F64F64>> {
    static constexpr int M = M_32, N = N_16, K = 1;
};
template <>
struct MmaAtomTraits<Ops<KP36_16x64x2_BF16BF16F32>> {
    static constexpr int M = M_16, N = N_64, K = K_2;
};
template <>
struct MmaAtomTraits<Ops<KP36_16x64x1_BF16BF16F32>> {
    static constexpr int M = M_16, N = N_64, K = 1;
};
template <>
struct MmaAtomTraits<Ops<KP36_16x64x4_INT8INT8INT32>> {
    static constexpr int M = M_16, N = N_64, K = K_4;
};
template <>
struct MmaAtomTraits<Ops<KP36_32x32x4_INT8INT8INT32>> {
    static constexpr int M = M_32, N = N_32, K = K_4;
};
template <>
struct MmaAtomTraits<Ops<KP36_32x32x2_BF16BF16F32>> {
    static constexpr int M = M_32, N = N_32, K = K_2;
};

template <typename MmaAtom, int AtomShapeM, int AtomShapeN, int AtomShapeK>
class TiledMma<MmaAtom, Shape<Int<AtomShapeM>, Int<AtomShapeN>, Int<AtomShapeK>>> {
public:
    template <typename TiledMma, typename EngineD, typename LayoutD, typename EngineA, typename LayoutA,
              typename EngineB, typename LayoutB, typename EngineC, typename LayoutC>
    friend void mma(TiledMma tiled_mma, Tensor<EngineD, LayoutD> D, Tensor<EngineA, LayoutA> A,
                    Tensor<EngineB, LayoutB> B, Tensor<EngineC, LayoutC> C) KUPL_MMA_INOUT;
    template <typename TiledMma, typename EngineD, typename LayoutD, typename EngineA, typename LayoutA,
              typename EngineB, typename LayoutB>
    friend kupl_always_inline void mma(TiledMma tiled_mma, Tensor<EngineD, LayoutD> D, Tensor<EngineA, LayoutA> A,
                                       Tensor<EngineB, LayoutB> B) KUPL_MMA_INOUT;

private:
    // 4-arg: A * B + C -> ZA
    template <typename dtypeD, typename ShapeD, typename StrideD, typename dtypeA, typename ShapeA, typename StrideA,
              typename dtypeB, typename ShapeB, typename StrideB, typename dtypeC, typename ShapeC, typename StrideC>
    kupl_always_inline void call([[maybe_unused]] Tensor<MatrixEngine<dtypeD>, Layout<ShapeD, StrideD>> &D,
                                 const Tensor<PtrEngine<dtypeA>, Layout<ShapeA, StrideA>> &A,
                                 const Tensor<PtrEngine<dtypeB>, Layout<ShapeB, StrideB>> &B,
                                 const Tensor<PtrEngine<dtypeC>, Layout<ShapeC, StrideC>> &C) KUPL_MMA_INOUT
    {
        TiledCallFunc::call_mma<MmaAtomTraits<MmaAtom>::M * AtomShapeM, MmaAtomTraits<MmaAtom>::N * AtomShapeN, StrideA,
                                StrideB, StrideC, dtypeA, dtypeB, dtypeC>(A.data(), B.data(), C.data(),
                                                                          MmaAtomTraits<MmaAtom>::K * AtomShapeK);
    }
    // 2-arg: A * B -> ZA
    template <typename dtypeD, typename ShapeD, typename StrideD, typename dtypeA, typename ShapeA, typename StrideA,
              typename dtypeB, typename ShapeB, typename StrideB>
    kupl_always_inline void call([[maybe_unused]] Tensor<MatrixEngine<dtypeD>, Layout<ShapeD, StrideD>> &D,
                                 const Tensor<PtrEngine<dtypeA>, Layout<ShapeA, StrideA>> &A,
                                 const Tensor<PtrEngine<dtypeB>, Layout<ShapeB, StrideB>> &B) KUPL_MMA_INOUT
    {
        TiledCallFunc::call_mma<MmaAtomTraits<MmaAtom>::M * AtomShapeM, MmaAtomTraits<MmaAtom>::N * AtomShapeN, StrideA,
                                StrideB, dtypeA, dtypeB>(A.data(), B.data(), MmaAtomTraits<MmaAtom>::K * AtomShapeK);
    }
};

template <>
kupl_always_inline void
TiledCallFunc::call_mma<32, 16, Stride<Int<1>, Int<32>>, Stride<Int<16>, Int<1>>, Stride<Int<16>, Int<1>>, double,
                        double, double>(double *data_a, double *data_b, double *data_c, int size_k) KUPL_MMA_INOUT
{
    const svbool_t p64 = svwhilelt_b64(0, 8);
    svfloat64_t vc0;
    svfloat64_t vc1;
    svfloat64_t vc2;
    svfloat64_t vc3;
    svfloat64_t vc4;
    svfloat64_t vc5;
    svfloat64_t vc6;
    svfloat64_t vc7;
    double *matc0 = data_c;
    double *matc1 = data_c + 128;
    double *matc2 = data_c + 256;
    double *matc3 = data_c + 384;
    for (uint32_t t = 0; t < 8; ++t) {
        vc0 = svld1_f64(p64, matc0);
        svwrite_hor_za64_m(0, t, p64, vc0);
        vc1 = svld1_f64(p64, matc0 + 8);
        svwrite_hor_za64_m(4, t, p64, vc1);
        vc2 = svld1_f64(p64, matc1);
        svwrite_hor_za64_m(1, t, p64, vc2);
        vc3 = svld1_f64(p64, matc1 + 8);
        svwrite_hor_za64_m(5, t, p64, vc3);
        vc4 = svld1_f64(p64, matc2);
        svwrite_hor_za64_m(2, t, p64, vc4);
        vc5 = svld1_f64(p64, matc2 + 8);
        svwrite_hor_za64_m(6, t, p64, vc5);
        vc6 = svld1_f64(p64, matc3);
        svwrite_hor_za64_m(3, t, p64, vc6);
        vc7 = svld1_f64(p64, matc3 + 8);
        svwrite_hor_za64_m(7, t, p64, vc7);
        matc0 += 16; // ldm_c
        matc1 += 16;
        matc2 += 16;
        matc3 += 16;
    }
    double *data_atmp = data_a;
    double *data_btmp = data_b;
    for (int i = 0; i < size_k; ++i) {
        vc0 = svld1_f64(p64, data_atmp);
        vc4 = svld1_f64(p64, data_btmp);
        svmopa_za64_f64_m(0, p64, p64, vc0, vc4);
        vc1 = svld1_f64(p64, data_atmp + 8);
        svmopa_za64_f64_m(1, p64, p64, vc1, vc4);
        vc2 = svld1_f64(p64, data_atmp + 16);
        svmopa_za64_f64_m(2, p64, p64, vc2, vc4);
        vc3 = svld1_f64(p64, data_atmp + 24);
        svmopa_za64_f64_m(3, p64, p64, vc3, vc4);
        vc5 = svld1_f64(p64, data_btmp + 8);
        svmopa_za64_f64_m(4, p64, p64, vc0, vc5);
        svmopa_za64_f64_m(5, p64, p64, vc1, vc5);
        svmopa_za64_f64_m(6, p64, p64, vc2, vc5);
        svmopa_za64_f64_m(7, p64, p64, vc3, vc5);

        data_atmp += 32; // ldm_a;
        data_btmp += 16; // ldm_b;
    }
}

template <>
kupl_always_inline void
TiledCallFunc::call_mma<16, 64, Stride<Int<2>, Stride<Int<1>, Int<32>>>, Stride<Stride<Int<1>, Int<128>>, Int<2>>,
                        Stride<Int<64>, Int<1>>, bfloat16_t, bfloat16_t, float>(bfloat16_t *data_a, bfloat16_t *data_b,
                                                                                float *data_c,
                                                                                int size_k) KUPL_MMA_INOUT
{
    const svbool_t p16 = svwhilelt_b16(0, 32);
    const svbool_t p32 = svwhilelt_b32(0, 16);
    svfloat32_t vc32_0;
    svfloat32_t vc32_1;
    svfloat32_t vc32_2;
    svfloat32_t vc32_3;
    float *matc0 = data_c;
    float *matc1 = data_c + 16;
    float *matc2 = data_c + 32;
    float *matc3 = data_c + 48;
    for (uint32_t t = 0; t < 16; ++t) {
        vc32_0 = svld1_f32(p32, matc0);
        svwrite_hor_za32_m(0, t, p32, vc32_0);
        vc32_1 = svld1_f32(p32, matc1);
        svwrite_hor_za32_m(1, t, p32, vc32_1);
        vc32_2 = svld1_f32(p32, matc2);
        svwrite_hor_za32_m(2, t, p32, vc32_2);
        vc32_3 = svld1_f32(p32, matc3);
        svwrite_hor_za32_m(3, t, p32, vc32_3);

        matc0 += 64;
        matc1 += 64;
        matc2 += 64;
        matc3 += 64;
    }
    bfloat16_t *data_atmp = data_a;
    bfloat16_t *data_btmp = data_b;
    svbfloat16_t va0;
    svbfloat16_t vb0;
    svbfloat16_t vb1;
    svbfloat16_t vb2;
    svbfloat16_t vb3;
    for (int i = 0; i < size_k / 2; ++i) {
        va0 = svld1_bf16(p16, data_atmp);
        vb0 = svld1_bf16(p16, data_btmp);
        svmopa_za32_bf16_m(0, p16, p16, va0, vb0);
        vb1 = svld1_bf16(p16, data_btmp + 32);
        svmopa_za32_bf16_m(1, p16, p16, va0, vb1);
        vb2 = svld1_bf16(p16, data_btmp + 64);
        svmopa_za32_bf16_m(2, p16, p16, va0, vb2);
        vb3 = svld1_bf16(p16, data_btmp + 96);
        svmopa_za32_bf16_m(3, p16, p16, va0, vb3);

        data_atmp += 32;
        data_btmp += 128;
    }
}

#define KUPL_SVLDNT1_BF16(pg, base)                                                                      \
    [&] {                                                                                                \
        svbfloat16_t res;                                                                                \
        __asm__ volatile("ldnt1h { %0.h }, %1/z, [%2]\n" : "=w"(res) : "Upl"(pg), "r"(base) : "memory"); \
        return res;                                                                                      \
    }()

#define KUPL_SVLDNT1_INDEX_BF16(pg, base, index)                       \
    [&] {                                                              \
        svbfloat16_t res;                                              \
        __asm__ volatile("ldnt1h { %0.h }, %1/z, [%2, %3, lsl #1]\n"   \
                         : "=w"(res)                                   \
                         : "Upl"(pg), "r"(base), "r"((int64_t)(index)) \
                         : "memory");                                  \
        return res;                                                    \
    }()

template <>
kupl_always_inline void
TiledCallFunc::call_mma<32, 32, Stride<Int<2>, Stride<Int<1>, Int<64>>>, Stride<Stride<Int<1>, Int<64>>, Int<2>>,
                        bfloat16_t, bfloat16_t>(bfloat16_t *data_a, bfloat16_t *data_b, int size_k) KUPL_MMA_INOUT
{
    const svbool_t p16 = svwhilelt_b16(0, 32);
    bfloat16_t *data_atmp = data_a;
    bfloat16_t *data_btmp = data_b;
    svbfloat16_t va0;
    svbfloat16_t va1;
    svbfloat16_t vb0;
    svbfloat16_t vb1;
    for (int i = 0; i < size_k / 2; ++i) {
        va0 = svld1(p16, data_atmp);
        vb0 = KUPL_SVLDNT1_BF16(p16, data_btmp);
        svmopa_za32_bf16_m(0, p16, p16, va0, vb0);
        vb1 = KUPL_SVLDNT1_INDEX_BF16(p16, data_btmp, 32);
        svmopa_za32_bf16_m(1, p16, p16, va0, vb1);
        va1 = svld1(p16, data_atmp + 32);
        svmopa_za32_bf16_m(2, p16, p16, va1, vb0);
        svmopa_za32_bf16_m(3, p16, p16, va1, vb1);

        data_atmp += 64;
        data_btmp += 64;
    }
}

template <>
kupl_always_inline void
TiledCallFunc::call_mma<32, 32, Stride<Int<2>, Stride<Int<1>, Int<64>>>, Stride<Int<576>, Int<1>>, bfloat16_t,
                        bfloat16_t>(bfloat16_t *data_a, bfloat16_t *data_b, int size_k) KUPL_MMA_INOUT
{
    const svbool_t p16 = svwhilelt_b16(0, 32);
    bfloat16_t *data_atmp = data_a;
    bfloat16_t *data_btmp = data_b;
    svbfloat16_t va0;
    svbfloat16_t va1;
    svbfloat16_t vb0;
    svbfloat16_t vb1;
    svbfloat16_t vb2;
    svbfloat16_t vb3;
    for (int i = 0; i < size_k / 2; ++i) {
        vb0 = KUPL_SVLDNT1_BF16(p16, data_btmp);
        vb1 = KUPL_SVLDNT1_INDEX_BF16(p16, data_btmp, 576);
        vb2 = svzip1_bf16(vb0, vb1);
        va0 = svld1_bf16(p16, data_atmp);
        svmopa_za32_bf16_m(0, p16, p16, va0, vb2);
        vb3 = svzip2_bf16(vb0, vb1);
        svmopa_za32_bf16_m(1, p16, p16, va0, vb3);
        va1 = svld1_bf16(p16, data_atmp + 32);
        svmopa_za32_bf16_m(2, p16, p16, va1, vb2);
        svmopa_za32_bf16_m(3, p16, p16, va1, vb3);

        data_atmp += 64;
        data_btmp += 2 * 576;
    }
}

#if defined(__clang__)
static kupl_always_inline bfloat16_t float_to_bf16_arm(float x) KUPL_MMA_INOUT
{
    return (bfloat16_t)x;
}
#elif defined(__GNUC__)
static bfloat16_t float_to_bf16_arm(float x)
{
    return vcvth_bf16_f32(x);
}
#endif

template <>
kupl_always_inline void TiledCallFunc::call_mma<16, 64, Stride<Int<1>, Int<16>>, Stride<Int<64>, Int<1>>,
                                                Stride<Int<64>, Int<1>>, bfloat16_t, bfloat16_t, float>(
    bfloat16_t *data_a, bfloat16_t *data_b, float *data_c, int size_k) KUPL_MMA_INOUT
{
    const svbool_t p16_16 = svwhilelt_b16(0, 16);
    const svbool_t p16 = svwhilelt_b16(0, 32);
    const svbool_t p32 = svwhilelt_b32(0, 16);
    svfloat32_t vc32_0;
    svfloat32_t vc32_1;
    svfloat32_t vc32_2;
    svfloat32_t vc32_3;
    float *matc0 = data_c;
    float *matc1 = data_c + 16;
    float *matc2 = data_c + 32;
    float *matc3 = data_c + 48;
    for (uint32_t t = 0; t < 16; ++t) {
        vc32_0 = svld1_f32(p32, matc0);
        svwrite_hor_za32_m(0, t, p32, vc32_0);
        vc32_1 = svld1_f32(p32, matc1);
        svwrite_hor_za32_m(1, t, p32, vc32_1);
        vc32_2 = svld1_f32(p32, matc2);
        svwrite_hor_za32_m(2, t, p32, vc32_2);
        vc32_3 = svld1_f32(p32, matc3);
        svwrite_hor_za32_m(3, t, p32, vc32_3);

        matc0 += 64;
        matc1 += 64;
        matc2 += 64;
        matc3 += 64;
    }
    bfloat16_t zero[16];
    for (int i = 0; i < 16; ++i) {
        zero[i] = float_to_bf16_arm(0.0);
    }
    svbfloat16_t vzero = svld1_bf16(p16_16, zero);
    bfloat16_t *data_atmp = data_a;
    bfloat16_t *data_btmp = data_b;
    svbfloat16_t va0_tmp;
    svbfloat16_t vb0_tmp;
    svbfloat16_t vb1_tmp;
    svbfloat16_t vb2_tmp;
    svbfloat16_t vb3_tmp;
    svbfloat16_t va0;
    svbfloat16_t vb0;
    svbfloat16_t vb1;
    svbfloat16_t vb2;
    svbfloat16_t vb3;
    for (int i = 0; i < size_k; ++i) {
        va0_tmp = svld1_bf16(p16_16, data_atmp);
        va0 = svzip1_bf16(va0_tmp, vzero);
        vb0_tmp = svld1_bf16(p16_16, data_btmp);
        vb0 = svzip1_bf16(vb0_tmp, vzero);
        svmopa_za32_bf16_m(0, p16, p16, va0, vb0);
        vb1_tmp = svld1_bf16(p16_16, data_btmp + 16);
        vb1 = svzip1_bf16(vb1_tmp, vzero);
        svmopa_za32_bf16_m(1, p16, p16, va0, vb1);
        vb2_tmp = svld1_bf16(p16_16, data_btmp + 32);
        vb2 = svzip1_bf16(vb2_tmp, vzero);
        svmopa_za32_bf16_m(2, p16, p16, va0, vb2);
        vb3_tmp = svld1_bf16(p16_16, data_btmp + 48);
        vb3 = svzip1_bf16(vb3_tmp, vzero);
        svmopa_za32_bf16_m(3, p16, p16, va0, vb3);

        data_atmp += 16;
        data_btmp += 64;
    }
}

template <>
kupl_always_inline void
TiledCallFunc::call_mma<16, 64, Stride<Int<4>, Stride<Int<1>, Int<64>>>, Stride<Stride<Int<1>, Int<256>>, Int<4>>,
                        Stride<Int<64>, Int<1>>, int8_t, int8_t, int32_t>(int8_t *data_a, int8_t *data_b,
                                                                          int32_t *data_c, int size_k) KUPL_MMA_INOUT
{
    const svbool_t p8 = svwhilelt_b8(0, 64);
    const svbool_t p32 = svwhilelt_b32(0, 16);
    svint32_t vc32_0;
    svint32_t vc32_1;
    svint32_t vc32_2;
    svint32_t vc32_3;
    int32_t *matc0 = data_c;
    int32_t *matc1 = data_c + 16;
    int32_t *matc2 = data_c + 32;
    int32_t *matc3 = data_c + 48;
    for (uint32_t t = 0; t < 16; ++t) {
        vc32_0 = svld1_s32(p32, matc0);
        svwrite_hor_za32_m(0, t, p32, vc32_0);
        vc32_1 = svld1_s32(p32, matc1);
        svwrite_hor_za32_m(1, t, p32, vc32_1);
        vc32_2 = svld1_s32(p32, matc2);
        svwrite_hor_za32_m(2, t, p32, vc32_2);
        vc32_3 = svld1_s32(p32, matc3);
        svwrite_hor_za32_m(3, t, p32, vc32_3);

        matc0 += 64;
        matc1 += 64;
        matc2 += 64;
        matc3 += 64;
    }
    int8_t *data_atmp = data_a;
    int8_t *data_btmp = data_b;
    svint8_t va0;
    svint8_t vb0;
    svint8_t vb1;
    svint8_t vb2;
    svint8_t vb3;
    for (int i = 0; i < size_k / 4; ++i) {
        va0 = svld1_s8(p8, data_atmp);
        vb0 = svld1_s8(p8, data_btmp);
        svmopa_za32_s8_m(0, p8, p8, va0, vb0);
        vb1 = svld1_s8(p8, data_btmp + 64);
        svmopa_za32_s8_m(1, p8, p8, va0, vb1);
        vb2 = svld1_s8(p8, data_btmp + 128);
        svmopa_za32_s8_m(2, p8, p8, va0, vb2);
        vb3 = svld1_s8(p8, data_btmp + 192);
        svmopa_za32_s8_m(3, p8, p8, va0, vb3);

        data_atmp += 64;
        data_btmp += 256;
    }
}

template <>
kupl_always_inline void
TiledCallFunc::call_mma<32, 32, Stride<Int<4>, Stride<Int<1>, Int<128>>>, Stride<Stride<Int<1>, Int<128>>, Int<4>>,
                        Stride<Int<32>, Int<1>>, int8_t, int8_t, int32_t>(int8_t *data_a, int8_t *data_b,
                                                                          int32_t *data_c, int size_k) KUPL_MMA_INOUT
{
    const svbool_t p8 = svwhilelt_b8(0, 64);
    const svbool_t p32 = svwhilelt_b32(0, 16);
    svint32_t vc32_0;
    svint32_t vc32_1;
    svint32_t vc32_2;
    svint32_t vc32_3;
    int32_t *matc0 = data_c;
    int32_t *matc1 = data_c + 16;
    int32_t *matc2 = data_c + 16 * 32;
    int32_t *matc3 = data_c + 16 * 32 + 16;
    for (uint32_t t = 0; t < 16; ++t) {
        vc32_0 = svld1_s32(p32, matc0);
        svwrite_hor_za32_m(0, t, p32, vc32_0);
        vc32_1 = svld1_s32(p32, matc1);
        svwrite_hor_za32_m(1, t, p32, vc32_1);
        vc32_2 = svld1_s32(p32, matc2);
        svwrite_hor_za32_m(2, t, p32, vc32_2);
        vc32_3 = svld1_s32(p32, matc3);
        svwrite_hor_za32_m(3, t, p32, vc32_3);

        matc0 += 32;
        matc1 += 32;
        matc2 += 32;
        matc3 += 32;
    }
    int8_t *data_atmp = data_a;
    int8_t *data_btmp = data_b;
    svint8_t va0;
    svint8_t va1;
    svint8_t vb0;
    svint8_t vb1;
    for (int i = 0; i < size_k / 4; ++i) {
        va0 = svld1_s8(p8, data_atmp);
        vb0 = svld1_s8(p8, data_btmp);
        svmopa_za32_s8_m(0, p8, p8, va0, vb0);
        vb1 = svld1_s8(p8, data_btmp + 64);
        svmopa_za32_s8_m(1, p8, p8, va0, vb1);
        va1 = svld1_s8(p8, data_atmp + 64);
        svmopa_za32_s8_m(2, p8, p8, va1, vb0);
        svmopa_za32_s8_m(3, p8, p8, va1, vb1);

        data_atmp += 128;
        data_btmp += 128;
    }
}

} // namespace tensor

} // namespace kupl

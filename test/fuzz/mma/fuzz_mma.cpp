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
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <arm_bf16.h>
#include <cstdint>
#include "kupl_mma.h"
#include "common/fuzz_common.h"

using namespace kupl::tensor;

#if defined(__clang__)
#define test_kupl_za        __kupl_mma
#define test_kupl_streaming
#elif defined(__GNUC__)
#define test_kupl_za        __arm_new("za")
#define test_kupl_streaming __arm_streaming
#endif

test_kupl_za
void KP36_32x16x512_F64F64F64_kernel(double *data_a, double *data_b, double *data_c) test_kupl_streaming
{
    auto shape_a = make_shape(Int<32>{}, Int<512>{});
    auto shape_b = make_shape(Int<512>{}, Int<16>{});
    auto shape_c = make_shape(Int<32>{}, Int<16>{});

    auto stride_a = make_stride(Int<1>{}, Int<32>{});
    auto stride_b = make_stride(Int<16>{}, Int<1>{});
    auto stride_c = make_stride(Int<16>{}, Int<1>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_c = make_layout(shape_c, stride_c);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<1>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_32x16x512_F64F64F64>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_32x16_F64_STORE>{}, atom_store_shape);

    auto tensor_a = make_tensor(data_a, layout_a);
    auto tensor_b = make_tensor(data_b, layout_b);
    auto tensor_c = make_tensor(data_c, layout_c);
    auto za_d = make_tensor<decltype(tensor_c)::element_type>(layout_c);
    mma(tiled_mma, za_d, tensor_a, tensor_b, tensor_c);
    copy(tiled_store, tensor_c, za_d);
}

void KP36_32x16x512_F64F64F64_coverage()
{
    const int M = 32;
    const int N = 16;
    const int K = 512;
    double *data_a = (double*)malloc(sizeof(double) * M * K);
    double *data_b = (double*)malloc(sizeof(double) * K * N);
    double *data_c = (double*)malloc(sizeof(double) * M * N);

    KP36_32x16x512_F64F64F64_kernel(data_a, data_b, data_c);

    free(data_c);
    free(data_b);
    free(data_a);
}

test_kupl_za
void KP36_16x64x2_BF16BF16F32_kernel(bfloat16_t *data_a, bfloat16_t *data_b, float *data_c) test_kupl_streaming
{
    auto shape_a = make_shape(Int<16>{}, make_shape(Int<2>{}, Int<288>{}));
    auto shape_b = make_shape(make_shape(Int<2>{}, Int<288>{}), Int<64>{});
    auto shape_c = make_shape(Int<16>{}, Int<64>{});

    auto stride_a = make_stride(Int<2>{}, make_stride(Int<1>{}, Int<32>{}));
    auto stride_b = make_stride(make_stride(Int<1>{}, Int<128>{}), Int<2>{});
    auto stride_c = make_stride(Int<64>{}, Int<1>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_c = make_layout(shape_c, stride_c);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<288>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_16x64x2_BF16BF16F32>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_16x64_F32_STORE>{}, atom_store_shape);

    auto tensor_a = make_tensor(data_a, layout_a);
    auto tensor_b = make_tensor(data_b, layout_b);
    auto tensor_c = make_tensor(data_c, layout_c);
    auto za_d = make_tensor<decltype(tensor_c)::element_type>(layout_c);
    mma(tiled_mma, za_d, tensor_a, tensor_b, tensor_c);
    copy(tiled_store, tensor_c, za_d);
}

void KP36_16x64x2_BF16BF16F32_coverage()
{
    const int M = 16;
    const int N = 64;
    const int K = 576;
    bfloat16_t *data_a = (bfloat16_t *)malloc(sizeof(bfloat16_t) * M * K);
    bfloat16_t *data_b = (bfloat16_t *)malloc(sizeof(bfloat16_t) * K * N);
    float *data_c = (float *)malloc(sizeof(float) * M * N);

    KP36_16x64x2_BF16BF16F32_kernel(data_a, data_b, data_c);

    free(data_a);
    free(data_b);
    free(data_c);
}

test_kupl_za
void KP36_16x64x1_BF16BF16F32_kernel(bfloat16_t *data_a, bfloat16_t *data_b, float *data_c) test_kupl_streaming
{
    auto shape_a = make_shape(Int<16>{}, Int<576>{});
    auto shape_b = make_shape(Int<576>{}, Int<64>{});
    auto shape_c = make_shape(Int<16>{}, Int<64>{});

    auto stride_a = make_stride(Int<1>{}, Int<16>{});
    auto stride_b = make_stride(Int<64>{}, Int<1>{});
    auto stride_c = make_stride(Int<64>{}, Int<1>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_c = make_layout(shape_c, stride_c);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<576>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_16x64x1_BF16BF16F32>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_16x64_F32_STORE>{}, atom_store_shape);

    auto tensor_a = make_tensor(data_a, layout_a);
    auto tensor_b = make_tensor(data_b, layout_b);
    auto tensor_c = make_tensor(data_c, layout_c);
    auto za_d = make_tensor<decltype(tensor_c)::element_type>(layout_c);
    mma(tiled_mma, za_d, tensor_a, tensor_b, tensor_c);
    copy(tiled_store, tensor_c, za_d);
}

void KP36_16x64x1_BF16BF16F32_coverage()
{
    const int M = 16;
    const int N = 64;
    const int K = 576;
    bfloat16_t *data_a = (bfloat16_t *)malloc(sizeof(bfloat16_t) * M * K);
    bfloat16_t *data_b = (bfloat16_t *)malloc(sizeof(bfloat16_t) * K * N);
    float *data_c = (float *)malloc(sizeof(float) * M * N);

    KP36_16x64x1_BF16BF16F32_kernel(data_a, data_b, data_c);

    free(data_a);
    free(data_b);
    free(data_c);
}

test_kupl_za
void KP36_16x64x4_INT8INT8INT32_kernel(int8_t *data_a, int8_t *data_b, int32_t *data_c) test_kupl_streaming
{
    auto shape_a = make_shape(Int<16>{}, make_shape(Int<4>{}, Int<144>{}));
    auto shape_b = make_shape(make_shape(Int<4>{}, Int<144>{}), Int<64>{});
    auto shape_c = make_shape(Int<16>{}, Int<64>{});

    auto stride_a = make_stride(Int<4>{}, make_stride(Int<1>{}, Int<64>{}));
    auto stride_b = make_stride(make_stride(Int<1>{}, Int<256>{}), Int<4>{});
    auto stride_c = make_stride(Int<64>{}, Int<1>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_c = make_layout(shape_c, stride_c);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<144>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_16x64x4_INT8INT8INT32>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_16x64_INT32_STORE>{}, atom_store_shape);

    auto tensor_a = make_tensor(data_a, layout_a);
    auto tensor_b = make_tensor(data_b, layout_b);
    auto tensor_c = make_tensor(data_c, layout_c);
    auto za_d = make_tensor<decltype(tensor_c)::element_type>(layout_c);
    mma(tiled_mma, za_d, tensor_a, tensor_b, tensor_c);
    copy(tiled_store, tensor_c, za_d);
}

void KP36_16x64x4_INT8INT8INT32_coverage()
{
    const int M = 16;
    const int N = 64;
    const int K = 576;
    int8_t *data_a = (int8_t *)malloc(sizeof(int8_t) * M * K);
    int8_t *data_b = (int8_t *)malloc(sizeof(int8_t) * K * N);
    int32_t *data_c = (int32_t *)malloc(sizeof(int32_t) * M * N);

    KP36_16x64x4_INT8INT8INT32_kernel(data_a, data_b, data_c);

    free(data_a);
    free(data_b);
    free(data_c);
}

test_kupl_za
void KP36_32x32x4_INT8INT8INT32_kernel(int8_t *data_a, int8_t *data_b, int32_t *data_c) test_kupl_streaming
{
    auto shape_a = make_shape(Int<32>{}, make_shape(Int<4>{}, Int<144>{}));
    auto shape_b = make_shape(make_shape(Int<4>{}, Int<144>{}), Int<32>{});
    auto shape_c = make_shape(Int<32>{}, Int<32>{});

    auto stride_a = make_stride(Int<4>{}, make_stride(Int<1>{}, Int<128>{}));
    auto stride_b = make_stride(make_stride(Int<1>{}, Int<128>{}), Int<4>{});
    auto stride_c = make_stride(Int<32>{}, Int<1>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_c = make_layout(shape_c, stride_c);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<144>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_32x32x4_INT8INT8INT32>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_32x32_INT32_STORE>{}, atom_store_shape);

    auto tensor_a = make_tensor(data_a, layout_a);
    auto tensor_b = make_tensor(data_b, layout_b);
    auto tensor_c = make_tensor(data_c, layout_c);
    auto za_d = make_tensor<decltype(tensor_c)::element_type>(layout_c);
    mma(tiled_mma, za_d, tensor_a, tensor_b, tensor_c);
    copy(tiled_store, tensor_c, za_d);
}

void KP36_32x32x4_INT8INT8INT32_coverage()
{
    const int M = 32;
    const int N = 32;
    const int K = 576;
    int8_t *data_a = (int8_t *)malloc(sizeof(int8_t) * M * K);
    int8_t *data_b = (int8_t *)malloc(sizeof(int8_t) * K * N);
    int32_t *data_c = (int32_t *)malloc(sizeof(int32_t) * M * N);

    KP36_32x32x4_INT8INT8INT32_kernel(data_a, data_b, data_c);

    free(data_a);
    free(data_b);
    free(data_c);
}

test_kupl_za
void KP36_32x32x2_BF16BF16F32_kernel(bfloat16_t *pack_data_a, bfloat16_t *pack_data_b, float *data_d,
                                     int size_k) test_kupl_streaming
{
    auto shape_a = make_shape(Int<32>{}, make_shape(Int<2>{}, Int<1>{}));
    auto shape_b = make_shape(make_shape(Int<2>{}, Int<1>{}), Int<32>{});
    auto shape_d = make_shape(make_shape(Int<16>{}, Int<2>{}), Int<32>{});

    auto stride_a = make_stride(Int<2>{}, make_stride(Int<1>{}, Int<64>{}));
    auto stride_b = make_stride(make_stride(Int<1>{}, Int<64>{}), Int<2>{});
    auto stride_d = make_stride(make_stride(Int<1>{}, Int<1024>{}), Int<16>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_d = make_layout(shape_d, stride_d);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<1>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_32x32x2_BF16BF16F32>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_32x32_F32_STORE>{}, atom_store_shape);

    auto tensor_d = make_tensor(data_d, layout_d);

    auto shape_za = make_shape(Int<32>{}, Int<32>{});
    auto stride_za = make_stride(Int<32>{}, Int<1>{});
    auto layout_za = make_layout(shape_za, stride_za);
    auto za_d = make_tensor<decltype(tensor_d)::element_type>(layout_za);
    clear(za_d);
    for (int kt = 0; kt < size_k / 2; ++kt) {
        auto tensor_a = make_tensor(pack_data_a + kt * 64, layout_a);
        auto tensor_b = make_tensor(pack_data_b + kt * 64, layout_b);
        mma(tiled_mma, za_d, tensor_a, tensor_b);
    }
    copy(tiled_store, tensor_d, za_d);
}

void KP36_32x32x2_BF16BF16F32_coverage()
{
    const int M = 32;
    const int N = 32;
    const int K = 576;
    const int VER_STORE_LOWER_HALF = M / 2 * N;
    bfloat16_t *pack_data_a = (bfloat16_t *)malloc(sizeof(bfloat16_t) * M * K);
    bfloat16_t *pack_data_b = (bfloat16_t *)malloc(sizeof(bfloat16_t) * K * N);
    float *data_d = (float *)malloc(sizeof(float) * (M * N + VER_STORE_LOWER_HALF));

    KP36_32x32x2_BF16BF16F32_kernel(pack_data_a, pack_data_b, data_d, K);

    free(data_d);
    free(pack_data_b);
    free(pack_data_a);
}

test_kupl_za
void KP36_32x32x2_BF16BF16F32_KMajB_kernel(bfloat16_t *pack_data_a, bfloat16_t *data_b, float *data_d,
                                           int size_k) test_kupl_streaming
{
    auto shape_a = make_shape(Int<32>{}, make_shape(Int<2>{}, Int<1>{}));
    auto shape_b = make_shape(Int<576>{}, Int<32>{});
    auto shape_d = make_shape(make_shape(Int<16>{}, Int<2>{}), Int<32>{});

    auto stride_a = make_stride(Int<2>{}, make_stride(Int<1>{}, Int<64>{}));
    auto stride_b = make_stride(Int<576>{}, Int<1>{});
    auto stride_d = make_stride(make_stride(Int<1>{}, Int<1024>{}), Int<16>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_d = make_layout(shape_d, stride_d);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<1>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_32x32x2_BF16BF16F32>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_32x32_F32_STORE>{}, atom_store_shape);

    auto tensor_d = make_tensor(data_d, layout_d);

    auto shape_za = make_shape(Int<32>{}, Int<32>{});
    auto stride_za = make_stride(Int<32>{}, Int<1>{});
    auto layout_za = make_layout(shape_za, stride_za);
    auto za_d = make_tensor<decltype(tensor_d)::element_type>(layout_za);
    clear(za_d);
    for (int kt = 0; kt < size_k / 2; ++kt) {
        auto tensor_a = make_tensor(pack_data_a + kt * 64, layout_a);
        auto tensor_b = make_tensor(data_b + kt * 2 * 576, layout_b);
        mma(tiled_mma, za_d, tensor_a, tensor_b);
    }
    copy(tiled_store, tensor_d, za_d);
}

void KP36_32x32x2_BF16BF16F32_KMajB_coverage()
{
    const int M = 32;
    const int N = 32;
    const int K = 576;
    const int VER_STORE_LOWER_HALF = M / 2 * N;
    bfloat16_t *pack_data_a = (bfloat16_t *)malloc(sizeof(bfloat16_t) * M * K);
    bfloat16_t *data_b = (bfloat16_t *)malloc(sizeof(bfloat16_t) * K * K);
    float *data_d = (float *)malloc(sizeof(float) * (M * N + VER_STORE_LOWER_HALF));

    KP36_32x32x2_BF16BF16F32_KMajB_kernel(pack_data_a, data_b, data_d, K);

    free(data_d);
    free(data_b);
    free(pack_data_a);
}

test_kupl_za
void KP36_32x32x2_BF16BF16F32_Store2048_kernel(bfloat16_t *pack_data_a, bfloat16_t *pack_data_b, float *data_d,
                                               int size_k) test_kupl_streaming
{
    auto shape_a = make_shape(Int<32>{}, make_shape(Int<2>{}, Int<1>{}));
    auto shape_b = make_shape(make_shape(Int<2>{}, Int<1>{}), Int<32>{});
    auto shape_d = make_shape(make_shape(Int<16>{}, Int<2>{}), Int<32>{});

    auto stride_a = make_stride(Int<2>{}, make_stride(Int<1>{}, Int<64>{}));
    auto stride_b = make_stride(make_stride(Int<1>{}, Int<64>{}), Int<2>{});
    auto stride_d = make_stride(make_stride(Int<1>{}, Int<2048>{}), Int<16>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_d = make_layout(shape_d, stride_d);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<1>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_32x32x2_BF16BF16F32>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_32x32_F32_STORE>{}, atom_store_shape);

    auto tensor_d = make_tensor(data_d, layout_d);

    auto shape_za = make_shape(Int<32>{}, Int<32>{});
    auto stride_za = make_stride(Int<32>{}, Int<1>{});
    auto layout_za = make_layout(shape_za, stride_za);
    auto za_d = make_tensor<decltype(tensor_d)::element_type>(layout_za);
    clear(za_d);
    for (int kt = 0; kt < size_k / 2; ++kt) {
        auto tensor_a = make_tensor(pack_data_a + kt * 64, layout_a);
        auto tensor_b = make_tensor(pack_data_b + kt * 64, layout_b);
        mma(tiled_mma, za_d, tensor_a, tensor_b);
    }
    copy(tiled_store, tensor_d, za_d);
}

void KP36_32x32x2_BF16BF16F32_Store2048_coverage()
{
    const int M = 32;
    const int N = 32;
    const int K = 576;
    bfloat16_t *pack_data_a = (bfloat16_t *)malloc(sizeof(bfloat16_t) * M * K);
    bfloat16_t *pack_data_b = (bfloat16_t *)malloc(sizeof(bfloat16_t) * K * N);
    float *data_d = (float *)malloc(sizeof(float) * (4 * M * N));

    KP36_32x32x2_BF16BF16F32_Store2048_kernel(pack_data_a, pack_data_b, data_d, K);

    free(data_d);
    free(pack_data_b);
    free(pack_data_a);
}

void mma_coverage()
{
    KP36_32x16x512_F64F64F64_coverage();
    KP36_16x64x2_BF16BF16F32_coverage();
    KP36_16x64x1_BF16BF16F32_coverage();
    KP36_16x64x4_INT8INT8INT32_coverage();
    KP36_32x32x4_INT8INT8INT32_coverage();
    KP36_32x32x2_BF16BF16F32_coverage();
    KP36_32x32x2_BF16BF16F32_KMajB_coverage();
    KP36_32x32x2_BF16BF16F32_Store2048_coverage();
}

test_kupl_za
void KP36_32x16x1_F64F64F64_kernel(double *data_a, double *data_b, double *data_c) test_kupl_streaming
{
    auto shape_a = make_shape(Int<32>{}, Int<512>{});
    auto shape_b = make_shape(Int<512>{}, Int<16>{});
    auto shape_c = make_shape(Int<32>{}, Int<16>{});

    auto stride_a = make_stride(Int<1>{}, Int<32>{});
    auto stride_b = make_stride(Int<16>{}, Int<1>{});
    auto stride_c = make_stride(Int<16>{}, Int<1>{});

    auto layout_a = make_layout(shape_a, stride_a);
    auto layout_b = make_layout(shape_b, stride_b);
    auto layout_c = make_layout(shape_c, stride_c);

    auto atom_mma_shape = make_shape(Int<1>{}, Int<1>{}, Int<512>{});
    auto tiled_mma = make_tiled_mma(Ops<KP36_32x16x1_F64F64F64>{}, atom_mma_shape);
    auto atom_store_shape = make_shape(Int<1>{}, Int<1>{});
    auto tiled_store = make_tiled_copy(Ops<KP36_32x16_F64_STORE>{}, atom_store_shape);

    auto tensor_a = make_tensor(data_a, layout_a);
    auto tensor_b = make_tensor(data_b, layout_b);
    auto tensor_c = make_tensor(data_c, layout_c);
    auto za_d = make_tensor<decltype(tensor_c)::element_type>(layout_c);
    mma(tiled_mma, za_d, tensor_a, tensor_b, tensor_c);
    copy(tiled_store, tensor_c, za_d);
}

void mma_base_example(int test_count)
{
    mma_coverage();
    const int M = 32;
    const int N = 16;
    const int K = 512;
    double *data_a_bak = (double*)malloc(sizeof(double) * M * K);
    double *data_b_bak = (double*)malloc(sizeof(double) * K * N);
    double *data_c_bak = (double*)malloc(sizeof(double) * M * N);
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < M; j++) {
            if (i == j) {
                data_a_bak[i * M + j] = 1.0;
            } else {
                data_a_bak[i * M + j] = 0.0;
            }
        }
    }
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < N; j++) {
            data_b_bak[i * N + j] = (double)(i * N + j);
        }
    }
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            data_c_bak[i * N + j] = 0.0;
        }
    }

    printf("start -- %s\n", __func__);
    DT_Enable_Leak_Check(0, 0);
    DT_FUZZ_START(0, test_count, (char *)__func__, 0)
    {
        int cnt = 0;
        double *data_a = (double*)DT_SetGetFixBlob(&g_Element[cnt++], sizeof(double) * M * K,
                                                   sizeof(double) * M * K, (char*)data_a_bak);
        double *data_b = (double*)DT_SetGetFixBlob(&g_Element[cnt++], sizeof(double) * K * N,
                                                   sizeof(double) * K * N, (char*)data_b_bak);
        double *data_c = (double*)DT_SetGetFixBlob(&g_Element[cnt++], sizeof(double) * M * N,
                                                   sizeof(double) * M * N, (char*)data_c_bak);

        KP36_32x16x1_F64F64F64_kernel(data_a, data_b, data_c);
    }
    DT_FUZZ_END();
    printf("end -- %s\n", __func__);

    free(data_c_bak);
    free(data_b_bak);
    free(data_a_bak);
}

void mma_32x32x2_example(int test_count)
{
    const int M = 32;
    const int N = 32;
    const int K = 576;
    const int VER_STORE_LOWER_HALF = M / 2 * N;
    bfloat16_t *pack_data_a_bak = (bfloat16_t *)malloc(sizeof(bfloat16_t) * M * K);
    bfloat16_t *pack_data_b_bak = (bfloat16_t *)malloc(sizeof(bfloat16_t) * K * N);
    float *data_d_bak = (float *)malloc(sizeof(float) * (M * N + VER_STORE_LOWER_HALF));
    memset(pack_data_a_bak, 0, sizeof(bfloat16_t) * M * K);
    memset(pack_data_b_bak, 0, sizeof(bfloat16_t) * K * N);
    memset(data_d_bak, 0, sizeof(float) * (M * N + VER_STORE_LOWER_HALF));

    printf("start -- %s\n", __func__);
    DT_Enable_Leak_Check(0, 0);
    DT_FUZZ_START(0, test_count, (char *)__func__, 0)
    {
        int cnt = 0;
        bfloat16_t *pack_data_a = (bfloat16_t *)DT_SetGetFixBlob(
            &g_Element[cnt++], sizeof(bfloat16_t) * M * K, sizeof(bfloat16_t) * M * K, (char *)pack_data_a_bak);
        bfloat16_t *pack_data_b = (bfloat16_t *)DT_SetGetFixBlob(
            &g_Element[cnt++], sizeof(bfloat16_t) * K * N, sizeof(bfloat16_t) * K * N, (char *)pack_data_b_bak);
        float *data_d = (float *)DT_SetGetFixBlob(&g_Element[cnt++], sizeof(float) * (M * N + VER_STORE_LOWER_HALF),
                                                  sizeof(float) * (M * N + VER_STORE_LOWER_HALF), (char *)data_d_bak);

        KP36_32x32x2_BF16BF16F32_kernel(pack_data_a, pack_data_b, data_d, K);
    }
    DT_FUZZ_END();
    printf("end -- %s\n", __func__);

    free(data_d_bak);
    free(pack_data_b_bak);
    free(pack_data_a_bak);
}
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
void KP36_1x16_F64_TRANS_CM2RM_kernel(double *dst, double *src) test_kupl_streaming
{
    constexpr int M = 512;
    constexpr int N = 16;
    auto shape_d = make_shape(Int<M>{}, Int<N>{});
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(Int<16>{}, Int<1>{});
    auto stride_s = make_stride(Int<1>{}, Int<M>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<M / 1>{}, Int<1>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_1x16_F64_TRANS_CM2RM>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_1x16_F64_TRANS_CM2RM_coverage()
{
    constexpr int M = 512;
    constexpr int N = 16;
    double *dst = static_cast<double *>(malloc(sizeof(double) * M * N));
    double *src = static_cast<double *>(malloc(sizeof(double) * M * N));

    KP36_1x16_F64_TRANS_CM2RM_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_16x2_BF16_TRANS_RM2ZZ_kernel(bfloat16_t *dst, bfloat16_t *src) test_kupl_streaming
{
    constexpr int M = 16;
    constexpr int N = 576;
    auto shape_d = make_shape(Int<M>{}, make_shape(Int<2>{}, Int<N / 2>{}));
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(Int<2>{}, make_stride(Int<1>{}, Int<32>{}));
    auto stride_s = make_stride(Int<N>{}, Int<1>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<1>{}, Int<N / 2>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_16x2_BF16_TRANS_RM2ZZ>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_16x2_BF16_TRANS_RM2ZZ_coverage()
{
    constexpr int M = 16;
    constexpr int N = 576;
    bfloat16_t *dst = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * N));
    bfloat16_t *src = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * N));

    KP36_16x2_BF16_TRANS_RM2ZZ_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_2x64_BF16_TRANS_CM2NN_kernel(bfloat16_t *dst, bfloat16_t *src) test_kupl_streaming
{
    constexpr int M = 576;
    constexpr int N = 64;
    auto shape_d = make_shape(make_shape(Int<2>{}, Int<M / 2>{}), Int<N>{});
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(make_stride(Int<1>{}, Int<128>{}), Int<2>{});
    auto stride_s = make_stride(Int<1>{}, Int<M>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<M / 2>{}, Int<1>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_2x64_BF16_TRANS_CM2NN>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_2x64_BF16_TRANS_CM2NN_coverage()
{
    constexpr int M = 576;
    constexpr int N = 64;
    bfloat16_t *dst = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * N));
    bfloat16_t *src = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * N));

    KP36_2x64_BF16_TRANS_CM2NN_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_16x1_BF16_TRANS_RM2CM_kernel(bfloat16_t *dst, bfloat16_t *src) test_kupl_streaming
{
    constexpr int M = 16;
    constexpr int N = 576;
    auto shape_d = make_shape(Int<M>{}, Int<N>{});
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(Int<1>{}, Int<16>{});
    auto stride_s = make_stride(Int<N>{}, Int<1>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<1>{}, Int<N>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_16x1_BF16_TRANS_RM2CM>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_16x1_BF16_TRANS_RM2CM_coverage()
{
    constexpr int M = 16;
    constexpr int N = 576;
    bfloat16_t *dst = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * N));
    bfloat16_t *src = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * N));

    KP36_16x1_BF16_TRANS_RM2CM_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_1x64_BF16_TRANS_CM2RM_kernel(bfloat16_t *dst, bfloat16_t *src) test_kupl_streaming
{
    constexpr int M = 576;
    constexpr int N = 64;
    auto shape_d = make_shape(Int<M>{}, Int<N>{});
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(Int<64>{}, Int<1>{});
    auto stride_s = make_stride(Int<1>{}, Int<M>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<M>{}, Int<1>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_1x64_BF16_TRANS_CM2RM>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_1x64_BF16_TRANS_CM2RM_coverage()
{
    constexpr int M = 576;
    constexpr int N = 64;
    bfloat16_t *dst = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * N));
    bfloat16_t *src = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * N));

    KP36_1x64_BF16_TRANS_CM2RM_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_16x4_INT8_TRANS_RM2ZZ_kernel(int8_t *dst, int8_t *src) test_kupl_streaming
{
    constexpr int M = 16;
    constexpr int N = 576;
    auto shape_d = make_shape(Int<M>{}, make_shape(Int<4>{}, Int<N / 4>{}));
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(Int<4>{}, make_stride(Int<1>{}, Int<64>{}));
    auto stride_s = make_stride(Int<N>{}, Int<1>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<1>{}, Int<N / 4>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_16x4_INT8_TRANS_RM2ZZ>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_16x4_INT8_TRANS_RM2ZZ_coverage()
{
    constexpr int M = 16;
    constexpr int N = 576;
    int8_t *dst = static_cast<int8_t *>(malloc(sizeof(int8_t) * M * N));
    int8_t *src = static_cast<int8_t *>(malloc(sizeof(int8_t) * M * N));

    KP36_16x4_INT8_TRANS_RM2ZZ_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_4x64_INT8_TRANS_CM2NN_kernel(int8_t *dst, int8_t *src) test_kupl_streaming
{
    constexpr int M = 576;
    constexpr int N = 64;

    auto shape_d = make_shape(make_shape(Int<4>{}, Int<M / 4>{}), Int<N>{});
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(make_stride(Int<1>{}, Int<256>{}), Int<4>{});
    auto stride_s = make_stride(Int<1>{}, Int<M>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<M / 4>{}, Int<1>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_4x64_INT8_TRANS_CM2NN>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_4x64_INT8_TRANS_CM2NN_coverage()
{
    constexpr int M = 576;
    constexpr int N = 64;
    int8_t *dst = static_cast<int8_t *>(malloc(sizeof(int8_t) * M * N));
    int8_t *src = static_cast<int8_t *>(malloc(sizeof(int8_t) * M * N));

    KP36_4x64_INT8_TRANS_CM2NN_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_32x4_INT8_TRANS_RM2ZZ_kernel(int8_t *dst, int8_t *src) test_kupl_streaming
{
    constexpr int M = 32;
    constexpr int N = 576;
    auto shape_d = make_shape(Int<M>{}, make_shape(Int<4>{}, Int<N / 4>{}));
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(Int<4>{}, make_stride(Int<1>{}, Int<128>{}));
    auto stride_s = make_stride(Int<N>{}, Int<1>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<1>{}, Int<N / 4>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_32x4_INT8_TRANS_RM2ZZ>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_32x4_INT8_TRANS_RM2ZZ_coverage()
{
    constexpr int M = 32;
    constexpr int N = 576;
    int8_t *dst = static_cast<int8_t *>(malloc(sizeof(int8_t) * M * N));
    int8_t *src = static_cast<int8_t *>(malloc(sizeof(int8_t) * M * N));

    KP36_32x4_INT8_TRANS_RM2ZZ_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_4x32_INT8_TRANS_CM2NN_kernel(int8_t *dst, int8_t *src) test_kupl_streaming
{
    constexpr int M = 576;
    constexpr int N = 32;
    auto shape_d = make_shape(make_shape(Int<4>{}, Int<M / 4>{}), Int<N>{});
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(make_stride(Int<1>{}, Int<128>{}), Int<4>{});
    auto stride_s = make_stride(Int<1>{}, Int<M>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<M / 4>{}, Int<1>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_4x32_INT8_TRANS_CM2NN>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_4x32_INT8_TRANS_CM2NN_coverage()
{
    constexpr int M = 576;
    constexpr int N = 32;
    int8_t *dst = static_cast<int8_t *>(malloc(sizeof(int8_t) * M * N));
    int8_t *src = static_cast<int8_t *>(malloc(sizeof(int8_t) * M * N));

    KP36_4x32_INT8_TRANS_CM2NN_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_32x2_BF16_TRANS_RM2ZZ_kernel(bfloat16_t *dst, bfloat16_t *src) test_kupl_streaming
{
    constexpr int M = 32;
    constexpr int K = 576;
    auto shape_d = make_shape(Int<M>{}, make_shape(Int<2>{}, Int<K / 2>{}));
    auto shape_s = make_shape(Int<M>{}, Int<K>{});

    auto stride_d = make_stride(Int<2>{}, make_stride(Int<1>{}, Int<64>{}));
    auto stride_s = make_stride(Int<K>{}, Int<1>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<1>{}, Int<K / 2>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_32x2_BF16_TRANS_RM2ZZ>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_32x2_BF16_TRANS_RM2ZZ_coverage()
{
    constexpr int M = 32;
    constexpr int K = 576;
    bfloat16_t *dst = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * K));
    bfloat16_t *src = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * M * K));

    KP36_32x2_BF16_TRANS_RM2ZZ_kernel(dst, src);

    free(src);
    free(dst);
}

test_kupl_za
void KP36_2x32_BF16_TRANS_CM2NN_kernel(bfloat16_t *dst, bfloat16_t *src) test_kupl_streaming
{
    constexpr int K = 576;
    constexpr int N = 32;
    auto shape_d = make_shape(make_shape(Int<2>{}, Int<K / 2>{}), Int<N>{});
    auto shape_s = make_shape(Int<K>{}, Int<N>{});

    auto stride_d = make_stride(make_stride(Int<1>{}, Int<64>{}), Int<2>{});
    auto stride_s = make_stride(Int<1>{}, Int<K>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<K / 2>{}, Int<1>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_2x32_BF16_TRANS_CM2NN>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void KP36_2x32_BF16_TRANS_CM2NN_coverage()
{
    constexpr int K = 576;
    constexpr int N = 32;
    bfloat16_t *dst = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * K * N));
    bfloat16_t *src = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * K * N));

    KP36_2x32_BF16_TRANS_CM2NN_kernel(dst, src);

    free(src);
    free(dst);
}

void prefetch_coverage()
{
    auto atom_prefetch_shape = make_shape(Int<1>{});
    auto tiled_l1strm = make_tiled_copy(Ops<KP36_PREFETCH_L1STRM>{}, atom_prefetch_shape);
    auto tiled_l1keep = make_tiled_copy(Ops<KP36_PREFETCH_L1KEEP>{}, atom_prefetch_shape);
    auto tiled_l2strm = make_tiled_copy(Ops<KP36_PREFETCH_L2STRM>{}, atom_prefetch_shape);
    auto tiled_l2keep = make_tiled_copy(Ops<KP36_PREFETCH_L2KEEP>{}, atom_prefetch_shape);

    double buf_ds[8] = {0};
    float buf_fs[16] = {0};
    auto buf_bs = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * 32));
    int8_t buf_is[64] = {0};

    auto layout_ds = make_layout(make_shape(Int<8>{}), make_stride(Int<1>{}));
    auto layout_fs = make_layout(make_shape(Int<16>{}), make_stride(Int<1>{}));
    auto layout_bs = make_layout(make_shape(Int<32>{}), make_stride(Int<1>{}));
    auto layout_is = make_layout(make_shape(Int<64>{}), make_stride(Int<1>{}));

    auto tensor_ds = make_tensor(buf_ds, layout_ds);
    auto tensor_fs = make_tensor(buf_fs, layout_fs);
    auto tensor_bs = make_tensor(buf_bs, layout_bs);
    auto tensor_is = make_tensor(buf_is, layout_is);

    copy(tiled_l1strm, tensor_ds);
    copy(tiled_l1keep, tensor_fs);
    copy(tiled_l2strm, tensor_bs);
    copy(tiled_l2keep, tensor_is);

    free(buf_bs);
}

void vec_load_store_coverage()
{
    const int Nf = 16;
    const int Nb = 32;
    auto layout_f = make_layout(make_shape(Int<Nf>{}), make_stride(Int<1>{}));
    auto layout_b = make_layout(make_shape(Int<Nb>{}), make_stride(Int<1>{}));

    float buf_f[Nf] = {0};
    auto buf_b = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * Nb));
    auto tensor_f = make_tensor(buf_f, layout_f);
    auto tensor_b = make_tensor(buf_b, layout_b);

    auto vec_f = make_tensor<float>(layout_f);
    auto vec_b = make_tensor<bfloat16_t>(layout_b);

    copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, vec_f, tensor_f);
    copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, tensor_f, vec_f);
    copy(TiledCopy<Ops<KP36_VEC_STORE_NT>, Shape<Int<1>>>{}, tensor_f, vec_f);

    copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, vec_b, tensor_b);
    copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, tensor_b, vec_b);
    copy(TiledCopy<Ops<KP36_VEC_STORE_NT>, Shape<Int<1>>>{}, tensor_b, vec_b);

    free(buf_b);
}

test_kupl_za
void KP36_VEC_COPY_HOR_kernel(float *out) test_kupl_streaming
{
    svbool_t pg = svptrue_b32();
    auto sve_layout = make_layout(make_shape(Int<16>{}), make_stride(Int<1>{}));
    auto vec = make_tensor<float>(sve_layout);

    auto c_layout = make_layout(make_shape(Int<16>{}, Int<64>{}), make_stride(Int<64>{}, Int<1>{}));
    auto za = make_tensor<float>(c_layout);
    auto za_tile = za.tile(Int<0>{});
    for (uint32_t row = 0; row < 16; ++row) {
        float seed[16];
        for (int i = 0; i < 16; ++i) {
            seed[i] = static_cast<float>(row * 16 + i);
        }
        auto src_mem = make_tensor(seed, sve_layout);
        copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, vec, src_mem);
        auto dst_tile = za_tile(make_coord(static_cast<int>(row), Underscore{}));
        copy(TiledCopy<Ops<KP36_VEC_COPY_HOR>, Shape<Int<1>>>{}, dst_tile, vec);
        svfloat32_t v = svread_hor_za32_m(vec.data(), pg, 0, row);
        svst1_f32(pg, out + row * 16, v);
    }
    for (uint32_t row = 0; row < 16; ++row) {
        float seed[16];
        for (int i = 0; i < 16; ++i) {
            seed[i] = static_cast<float>(row * 16 + i);
        }
        svfloat32_t v = svld1_f32(pg, seed);
        svwrite_hor_za32_m(0, row, pg, v);
        copy(TiledCopy<Ops<KP36_VEC_COPY_HOR>, Shape<Int<1>>>{}, vec,
             za_tile(make_coord(static_cast<int>(row), Underscore{})));
        auto dst_mem = make_tensor(out + 16 * 16 + row * 16, sve_layout);
        copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, dst_mem, vec);
    }
}

void KP36_VEC_COPY_HOR_coverage()
{
    float *out = static_cast<float *>(malloc(sizeof(float) * (2 * 16 * 16)));

    KP36_VEC_COPY_HOR_kernel(out);

    free(out);
}

void copy_coverage()
{
    KP36_1x16_F64_TRANS_CM2RM_coverage();
    KP36_16x2_BF16_TRANS_RM2ZZ_coverage();
    KP36_2x64_BF16_TRANS_CM2NN_coverage();
    KP36_16x1_BF16_TRANS_RM2CM_coverage();
    KP36_1x64_BF16_TRANS_CM2RM_coverage();
    KP36_16x4_INT8_TRANS_RM2ZZ_coverage();
    KP36_4x64_INT8_TRANS_CM2NN_coverage();
    KP36_32x4_INT8_TRANS_RM2ZZ_coverage();
    KP36_4x32_INT8_TRANS_CM2NN_coverage();
    KP36_32x2_BF16_TRANS_RM2ZZ_coverage();
    KP36_2x32_BF16_TRANS_CM2NN_coverage();
    prefetch_coverage();
    vec_load_store_coverage();
    KP36_VEC_COPY_HOR_coverage();
}

test_kupl_za
void KP36_32x1_F64_TRANS_RM2CM_kernel(double *dst, double *src) test_kupl_streaming
{
    constexpr int M = 32;
    constexpr int N = 512;
    auto shape_d = make_shape(Int<M>{}, Int<N>{});
    auto shape_s = make_shape(Int<M>{}, Int<N>{});

    auto stride_d = make_stride(Int<1>{}, Int<32>{});
    auto stride_s = make_stride(Int<N>{}, Int<1>{});

    auto layout_d = make_layout(shape_d, stride_d);
    auto layout_s = make_layout(shape_s, stride_s);

    auto atom_copy_shape = make_shape(Int<1>{}, Int<N / 1>{});
    auto tiled_copy = make_tiled_copy(Ops<KP36_32x1_F64_TRANS_RM2CM>{}, atom_copy_shape);

    auto tensor_d = make_tensor(dst, layout_d);
    auto tensor_s = make_tensor(src, layout_s);
    copy(tiled_copy, tensor_d, tensor_s);
}

void copy_base_example(int test_count)
{
    copy_coverage();
    constexpr int M = 32;
    constexpr int N = 512;
    double *dst_bak = static_cast<double *>(malloc(sizeof(double) * M * N));
    double *src_bak = static_cast<double *>(malloc(sizeof(double) * M * N));
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            dst_bak[m * N + n] = 0.0;
            src_bak[m * N + n] = 1.0 * m * N + n;
        }
    }

    printf("start -- %s\n", __func__);
    DT_Enable_Leak_Check(0, 0);
    DT_FUZZ_START(0, test_count, (char *)__func__, 0)
    {
        int cnt = 0;
        double *dst = (double*)DT_SetGetFixBlob(&g_Element[cnt++], sizeof(double) * M * N,
                                                sizeof(double) * M * N, (char*)dst_bak);
        double *src = (double*)DT_SetGetFixBlob(&g_Element[cnt++], sizeof(double) * M * N,
                                                sizeof(double) * M * N, (char*)src_bak);

        KP36_32x1_F64_TRANS_RM2CM_kernel(dst, src);
    }
    DT_FUZZ_END();
    printf("end -- %s\n", __func__);

    free(src_bak);
    free(dst_bak);
}
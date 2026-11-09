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
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <arm_bf16.h>
#include <cstdint>
#include "kupl_mma.h"
#include "common/fuzz_common.h"

using namespace kupl::tensor;

#if defined(__clang__)
static bfloat16_t to_bf16(float x)
{
    return (bfloat16_t)x;
}
#elif defined(__GNUC__)
static bfloat16_t to_bf16(float x)
{
    return vcvth_bf16_f32(x);
}
#endif

void tensor_vec_arith_coverage()
{
    const int N = 16;
    float fa[N];
    float fb[N];
    float fc[N] = {0};
    for (int i = 0; i < N; i++) {
        fa[i] = static_cast<float>(i + 1);
        fb[i] = static_cast<float>(i * 2 + 1);
    }

    auto layout = make_layout(make_shape(Int<16>{}), make_stride(Int<1>{}));
    auto ta = make_tensor(fa, layout);
    auto tb = make_tensor(fb, layout);
    auto tc = make_tensor(fc, layout);

    auto va = make_tensor<float>(layout);
    auto vb = make_tensor<float>(layout);
    copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, va, ta);
    copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, vb, tb);

    va = va + vb;
    va = va - vb;
    va = va * vb;
    va = va / vb;
    va = va + 1.0f;
    va = va - 1.0f;
    va = va * 2.0f;
    va = va / 2.0f;
    va = min(va, vb);
    va = max(va, vb);
    va = min(va, 8.0f);
    va = max(va, 8.0f);
    copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, tc, va);

    // scalar broadcast (dup)
    va = 1.0f;
    copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, tc, va);

    auto layout_b = make_layout(make_shape(Int<32>{}), make_stride(Int<1>{}));
    bfloat16_t *buf_b = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * 32));
    auto tb16 = make_tensor(buf_b, layout_b);
    auto vb16 = make_tensor<bfloat16_t>(layout_b);
    vb16 = to_bf16(2.0f);
    copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, tb16, vb16);
    free(buf_b);
}

void tensor_cvt_exp2f_coverage()
{
    const int Nf = 16;
    const int Nb = 32;
    float fa[Nf];
    float fb[Nf];
    for (int i = 0; i < Nf; i++) {
        fa[i] = static_cast<float>(i + 1);
        fb[i] = static_cast<float>(i + 10);
    }

    auto layout_f = make_layout(make_shape(Int<16>{}), make_stride(Int<1>{}));
    auto layout_b = make_layout(make_shape(Int<32>{}), make_stride(Int<1>{}));
    auto ta = make_tensor(fa, layout_f);
    auto tb = make_tensor(fb, layout_f);

    auto va = make_tensor<float>(layout_f);
    auto vb = make_tensor<float>(layout_f);
    copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, va, ta);
    copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, vb, tb);

    bfloat16_t *buf_c = static_cast<bfloat16_t *>(malloc(sizeof(bfloat16_t) * Nb));
    auto tc = make_tensor(buf_c, layout_b);
    auto result = cvt<bfloat16_t>(va, vb);
    copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, tc, result);
    free(buf_c);

    // exp2f: normal + overflow/underflow branch
    float fe[Nf] = {0};
    for (int i = 0; i < Nf; i++) {
        fe[i] = static_cast<float>(i - 7);
    }
    auto te = make_tensor(fe, layout_f);
    auto ve = make_tensor<float>(layout_f);
    copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, ve, te);
    ve = exp2f(ve);
    copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, te, ve);

    float fov[Nf] = {0};
    fov[0] = 128.0f;
    fov[1] = -128.0f;
    fov[2] = 2.0f;
    auto tov = make_tensor(fov, layout_f);
    auto vov = make_tensor<float>(layout_f);
    copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, vov, tov);
    vov = exp2f(vov);
    copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, tov, vov);
}

void tensor_ptr_arith_coverage()
{
    const int M = 3;
    const int N = 8;
    float da[M * N];
    float db[M * N];
    float dc[M * N];
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            da[i * N + j] = static_cast<float>(i * N + j);
            db[i * N + j] = static_cast<float>(M * N - i * N - j);
            dc[i * N + j] = 0.0f;
        }
    }

    auto layout = make_layout(make_shape(Int<3>{}, Int<8>{}), make_stride(Int<8>{}, Int<1>{}));
    auto ta = make_tensor(da, layout);
    auto tb = make_tensor(db, layout);
    auto tc = make_tensor(dc, layout);

    tc = ta + tb;
    tb = ta * 2.0f;
    tb = 2.0f * ta;

    // runtime crd2idx branch
    int ri = 1;
    int rj = 2;
    volatile float sink = ta(make_coord(ri, rj));
    (void)sink;

    // slice path: (Int, Underscore) then 1D coord
    auto slice = ta(make_coord(Int<1>{}, Underscore{}));
    volatile float sink2 = slice(make_coord(Int<2>{}));
    (void)sink2;
}

void tensor_nullptr_coverage()
{
    auto layout = make_layout(make_shape(Int<16>{}), make_stride(Int<1>{}));
    try {
        auto bad = make_tensor<float>(static_cast<float *>(nullptr), layout);
        (void)bad;
    } catch (const std::invalid_argument &) {
    }
}

void tensor_coverage()
{
    tensor_vec_arith_coverage();
    tensor_cvt_exp2f_coverage();
    tensor_ptr_arith_coverage();
    tensor_nullptr_coverage();
}

void tensor_base_example(int test_count)
{
    tensor_coverage();
    const int N = 16;
    float a_bak[N];
    for (int i = 0; i < N; i++) {
        a_bak[i] = static_cast<float>(i - 8);
    }
    float out[N] = {0};

    printf("start -- %s\n", __func__);
    DT_Enable_Leak_Check(0, 0);
    DT_FUZZ_START(0, test_count, (char *)__func__, 0)
    {
        int cnt = 0;
        float *a = (float *)DT_SetGetFixBlob(&g_Element[cnt++], sizeof(float) * N, sizeof(float) * N, (char *)a_bak);

        auto layout = make_layout(make_shape(Int<16>{}), make_stride(Int<1>{}));
        auto t_a = make_tensor(a, layout);
        auto t_out = make_tensor(out, layout);
        auto v_a = make_tensor<float>(layout);
        copy(TiledCopy<Ops<KP36_VEC_LOAD>, Shape<Int<1>>>{}, v_a, t_a);
        auto v_r = exp2f(v_a);
        v_r = v_r + 1.0f;
        copy(TiledCopy<Ops<KP36_VEC_STORE>, Shape<Int<1>>>{}, t_out, v_r);
    }
    DT_FUZZ_END();
    printf("end -- %s\n", __func__);
}

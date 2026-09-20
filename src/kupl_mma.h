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

// Core: compiler helpers + constants + KUPL_MMA_IN macros + TiledCallFunc
#include "mma/kupl_mma_core.h"
// Layout metaprogramming: Val/Int/Ops/Shape/Stride/Layout/crd2idx/slice/make_*
#include "mma/kupl_mma_layout.h"
// Engines and SVE primitives: PtrEngine/VectorEngine/MatrixEngine + Sv* ops
#include "mma/kupl_mma_engine.h"
// Tensor abstraction: Tensor/arith/cvt/exp2f/make_tensor/clear/prefetch_impl
#include "mma/kupl_mma_tensor.h"
// TiledMma: MmaAtomTraits + TiledMma + call_mma specializations
#include "mma/kupl_mma_mma.h"
// TiledCopy: StoreTraits/TransTraits/PrefetchTraits + TiledCopy + call_copy specializations
#include "mma/kupl_mma_copy.h"

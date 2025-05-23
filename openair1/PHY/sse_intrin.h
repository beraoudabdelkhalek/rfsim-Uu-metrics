/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file PHY/sse_intrin.h
 * \brief SSE includes and compatibility functions.
 *
 * This header collects all SSE compatibility functions. To use SSE inside a source file, include only sse_intrin.h.
 * The host CPU needs to have support for SSE2 at least. SSE3 and SSE4.1 functions are emulated if the CPU lacks support for them.
 * This will slow down the softmodem, but may be valuable if only offline signal processing is required.
 *
 * 
 * Has been changed in August 2022 to rely on SIMD Everywhere (SIMDE) from MIT
 * by bruno.mongazon-cazavet@nokia-bell-labs.com
 *
 * All AVX22 code is mapped to SIMDE which transparently relies on AVX2 HW (avx2-capable host) or SIMDE emulation
 * (non-avx2-capable host).
 * To force using SIMDE emulation on avx2-capable host use the --noavx2 flag. 
 * avx512 code is not mapped to SIMDE. It depends on --noavx512 flag.
 * If the --noavx512 is set the OAI AVX512 emulation using AVX2 is used.
 * If the --noavx512 is not set, AVX512 HW is used on avx512-capable host while OAI AVX512 emulation using AVX2
 * is used on non-avx512-capable host. 
 *
 * \author S. Held, Laurent THOMAS
 * \email sebastian.held@imst.de, laurent.thomas@open-cells.com	
 * \company IMST GmbH, Open Cells Project
 * \date 2019
 * \version 0.2
*/

#ifndef SSE_INTRIN_H
#define SSE_INTRIN_H


#include <simde/x86/avx2.h>
#include <simde/x86/fma.h>
#if defined(__x86_64) || defined(__i386__)

/* x86 processors */

#if defined(__AVX512BW__) || defined(__AVX512F__)
#include <immintrin.h>
#endif
#elif defined(__arm__) || defined(__aarch64__)

/* ARM processors */
// note this fails on some x86 machines, with an error like:
// /usr/lib/gcc/x86_64-redhat-linux/8/include/gfniintrin.h:57:1: error: inlining failed in call to always_inline ‘_mm_gf2p8affine_epi64_epi8’: target specific option mismatch
#include <simde/x86/clmul.h>

#include <simde/arm/neon.h>
#endif // x86_64 || i386
#include <stdbool.h>
#include "assertions.h"

/*
 * OAI specific
 */

static const short minusConjug128[8] __attribute__((aligned(16))) = {-1, 1, -1, 1, -1, 1, -1, 1};
static inline simde__m128i mulByConjugate128(simde__m128i *a, simde__m128i *b, int8_t output_shift)
{
  simde__m128i realPart = simde_mm_madd_epi16(*a, *b);
  realPart = simde_mm_srai_epi32(realPart, output_shift);
  simde__m128i imagPart = simde_mm_shufflelo_epi16(*b, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
  imagPart = simde_mm_shufflehi_epi16(imagPart, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
  imagPart = simde_mm_sign_epi16(imagPart, *(simde__m128i *)minusConjug128);
  imagPart = simde_mm_madd_epi16(imagPart, *a);
  imagPart = simde_mm_srai_epi32(imagPart, output_shift);
  simde__m128i lowPart = simde_mm_unpacklo_epi32(realPart, imagPart);
  simde__m128i highPart = simde_mm_unpackhi_epi32(realPart, imagPart);
  return (simde_mm_packs_epi32(lowPart, highPart));
}

#define displaySamples128(vect)                                      \
  {                                                                  \
    simde__m128i x = vect;                                           \
    printf("vector: %s = (%hd,%hd) (%hd,%hd) (%hd,%hd) (%hd,%hd)\n", \
           #vect,                                                    \
           simde_mm_extract_epi16(x, 0),                             \
           simde_mm_extract_epi16(x, 1),                             \
           simde_mm_extract_epi16(x, 2),                             \
           simde_mm_extract_epi16(x, 3),                             \
           simde_mm_extract_epi16(x, 4),                             \
           simde_mm_extract_epi16(x, 5),                             \
           simde_mm_extract_epi16(x, 6),                             \
           simde_mm_extract_epi16(x, 7));                            \
  }

__attribute__((always_inline)) static inline int64_t simde_mm_average_sse(simde__m128i *a, int length, int shift)
{
  // compute average level with shift (64-bit verstion)
  simde__m128i avg128 = simde_mm_setzero_si128();
  for (int i = 0; i < length >> 2; i++) {
    const simde__m128i in1 = a[i];
    avg128 = simde_mm_add_epi32(avg128, simde_mm_srai_epi32(simde_mm_madd_epi16(in1, in1), shift));
  }

  // Horizontally add pairs
  // 1st [A + C, B + D]
  simde__m128i sum_pairs = simde_mm_add_epi64(simde_mm_unpacklo_epi32(avg128, simde_mm_setzero_si128()), // [A, B] → [A, 0, B, 0]
                                              simde_mm_unpackhi_epi32(avg128, simde_mm_setzero_si128())  // [C, D] → [C, 0, D, 0]
  );

  // 2nd [A + B + C + D, ...]
  simde__m128i total_sum = simde_mm_add_epi64(sum_pairs, simde_mm_shuffle_epi32(sum_pairs, SIMDE_MM_SHUFFLE(1, 0, 3, 2)));

  // Extract horizontal sum as a scalar int64_t result
  return simde_mm_cvtsi128_si64(total_sum);
}

__attribute__((always_inline)) static inline int64_t simde_mm_average_avx2(simde__m256i *a, int length, int shift)
{
  simde__m256i avg256 = simde_mm256_setzero_si256();
  for (int i = 0; i < length >> 3; i++) {
    const simde__m256i in1 = simde_mm256_loadu_si256(&a[i]); // unaligned load
    avg256 = simde_mm256_add_epi32(avg256, simde_mm256_srai_epi32(simde_mm256_madd_epi16(in1, in1), shift));
  }

  // Split the 256-bit vector into two 128-bit halves and convert to 64-bit
  // [A + E, B + F, C + G, D + H]
  simde__m256i sum_pairs = simde_mm256_add_epi64(
      simde_mm256_cvtepi32_epi64(simde_mm256_castsi256_si128(avg256)),     // [A, B, C, D] → [A, 0, B, 0, C, 0, D, 0]
      simde_mm256_cvtepi32_epi64(simde_mm256_extracti128_si256(avg256, 1)) // [E, F, G, H] → [E, 0, F, 0, G, 0, H, 0]
  );

  // Horizontal sum within the 256-bit vector
  // [A + E + B + F, C + G + D + H]
  simde__m128i total_sum = simde_mm_add_epi64(simde_mm256_castsi256_si128(sum_pairs), simde_mm256_extracti128_si256(sum_pairs, 1));

  // [A + E + B + F + C + G + D + H, ...]
  total_sum = simde_mm_add_epi64(total_sum, simde_mm_shuffle_epi32(total_sum, SIMDE_MM_SHUFFLE(1, 0, 3, 2)));

  // Extract horizontal sum as a scalar int64_t result
  return simde_mm_cvtsi128_si64(total_sum);
}

__attribute__((always_inline)) static inline int32_t simde_mm_average(simde__m128i *a, int length, int shift, int16_t scale)
{
  int64_t avg = 0;

#if defined(__x86_64__) || defined(__i386__)
  if (__builtin_cpu_supports("avx2")) {
    avg += simde_mm_average_avx2((simde__m256i *)a, length, shift);

    // tail processing by SSE
    a += ((length & ~7) >> 2);
    length -= (length & ~7);
  }
#endif

  avg += simde_mm_average_sse(a, length, shift);

  return (uint32_t)(avg / scale);
}

#endif // SSE_INTRIN_H

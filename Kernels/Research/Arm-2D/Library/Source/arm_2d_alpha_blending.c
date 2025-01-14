/*
 * Copyright (C) 2010-2020 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ----------------------------------------------------------------------
 * Project:      Arm-2D Library
 * Title:        arm-2d.c
 * Description:  Basic Tile operations
 *
 * $Date:        01. December 2020
 * $Revision:    V.0.5.0
 *
 * Target Processor:  Cortex-M cores
 *
 * -------------------------------------------------------------------- */

#define __ARM_2D_IMPL__

#include "arm_2d.h"
#include "__arm_2d_impl.h"

#ifdef   __cplusplus
extern "C" {
#endif

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#   pragma clang diagnostic ignored "-Wcast-qual"
#   pragma clang diagnostic ignored "-Wcast-align"
#   pragma clang diagnostic ignored "-Wextra-semi-stmt"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#   pragma clang diagnostic ignored "-Wunused-function"
#   pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#   pragma clang diagnostic ignored "-Wdouble-promotion"
#   pragma clang diagnostic ignored "-Wunused-parameter"
#   pragma clang diagnostic ignored "-Wimplicit-float-conversion"
#   pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#   pragma clang diagnostic ignored "-Wtautological-pointer-compare"
#endif


/*----------------------------------------------------------------------------*
 * Accelerable Low Level APIs                                                 *
 *----------------------------------------------------------------------------*/

void __arm_2d_impl_rgb565_alpha_blending(
                                        uint16_t *__RESTRICT phwSource,
                                        int16_t iSourceStride,
                                        uint16_t *__RESTRICT phwTarget,
                                        int16_t iTargetStride,
                                        arm_2d_size_t *__RESTRICT ptCopySize,
                                        uint_fast8_t chRatio);

void __arm_2d_impl_rgb888_alpha_blending(
                                        uint32_t *__RESTRICT pwSource,
                                        int16_t iSourceStride,
                                        uint32_t *__RESTRICT pwTarget,
                                        int16_t iTargetStride,
                                        arm_2d_size_t *__RESTRICT ptCopySize,
                                        uint_fast8_t chRatio);

void __arm_2d_impl_rgb565_alpha_blending_direct(
                                        const uint16_t *phwSource,
                                        const uint16_t *phwBackground,
                                        uint16_t *phwDestination,
                                        uint32_t wPixelCount,
                                        uint_fast8_t chRatio);

void __arm_2d_impl_rgb888_alpha_blending_direct(
                                        const uint32_t *__RESTRICT pwSource,
                                        const uint32_t *__RESTRICT pwBackground,
                                        uint32_t *pwDestination,
                                        uint32_t wPixelCount,
                                        uint_fast8_t chRatio);

void __arm_2d_impl_rgb565_alpha_blending_colour_masking(
                                        uint16_t *__RESTRICT phwSource,
                                        int16_t iSourceStride,
                                        uint16_t *__RESTRICT phwTarget,
                                        int16_t iTargetStride,
                                        arm_2d_size_t *__RESTRICT ptCopySize,
                                        uint_fast8_t chRatio,
                                        uint_fast16_t hwColour);

void __arm_2d_impl_rgb888_alpha_blending_colour_masking(
                                        uint32_t *__RESTRICT pwSource,
                                        int16_t iSourceStride,
                                        uint32_t *__RESTRICT pwTarget,
                                        int16_t iTargetStride,
                                        arm_2d_size_t *__RESTRICT ptCopySize,
                                        uint_fast8_t chRatio,
                                        uint_fast32_t wColour);

/*----------------------------------------------------------------------------*
 * Copy tile to destination with specified transparency ratio (0~255)         *
 *----------------------------------------------------------------------------*/

ARM_NONNULL(1,2,3)
arm_fsm_rt_t arm_2d_rgb565_alpha_blending(
                                     const arm_2d_tile_t *ptSource,
                                     const arm_2d_tile_t *ptTarget,
                                     const arm_2d_region_t *ptRegion,
                                     uint_fast8_t chRatio)
{
    assert(NULL != ptSource);
    assert(NULL != ptTarget);

    ARM_2D_IMPL(arm_2d_op_alpha_t);
    memset(ptThis, 0, sizeof(*ptThis));

    OP_CORE.ptOp = &ARM_2D_OP_TABLE[ARM_2D_OP_ALPHA_BLENDING_RGB565];

    this.Target.ptTile = ptTarget;
    this.Target.ptRegion = ptRegion;
    this.Source.ptTile = ptSource;
    this.wMode = 0;
    this.chRatio = chRatio;

    return __arm_2d_op_invoke(NULL);
}

ARM_NONNULL(1,2,3)
arm_fsm_rt_t arm_2d_rgb888_alpha_blending(   const arm_2d_tile_t *ptSource,
                                             const arm_2d_tile_t *ptTarget,
                                             const arm_2d_region_t *ptRegion,
                                             uint_fast8_t chRatio)
{
    assert(NULL != ptSource);
    assert(NULL != ptTarget);

    ARM_2D_IMPL(arm_2d_op_alpha_t);
    memset(ptThis, 0, sizeof(*ptThis));

    OP_CORE.ptOp = &ARM_2D_OP_TABLE[ARM_2D_OP_ALPHA_BLENDING_RGB888];

    this.Target.ptTile = ptTarget;
    this.Target.ptRegion = ptRegion;
    this.Source.ptTile = ptSource;
    this.wMode = 0;
    this.chRatio = chRatio;

    return __arm_2d_op_invoke(NULL);
}

arm_fsm_rt_t __arm_2d_sw_alpha_blending(__arm_2d_sub_task_t *ptTask)
{
    ARM_2D_IMPL(arm_2d_op_alpha_t, ptTask->ptOP)

    if (    ptTask->Param.tCopy.iSourceStride
        ==  ptTask->Param.tCopy.iTargetStride
        ==  ptTask->Param.tCopy.tCopySize.iWidth) {

        //! direct blending
        switch (OP_CORE.ptOp->Info.Colour.u3ColourSZ) {
            case ARM_2D_COLOUR_SZ_16BIT:
                __arm_2d_impl_rgb565_alpha_blending_direct(
                                            ptTask->Param.tCopy.pSource,
                                            ptTask->Param.tCopy.pTarget,
                                            ptTask->Param.tCopy.pTarget,
                                            ptTask->Param.tCopy.tCopySize.iHeight
                                                * ptTask->Param.tCopy.tCopySize.iWidth,
                                            this.chRatio);
                break;
            case ARM_2D_COLOUR_SZ_32BIT:
                __arm_2d_impl_rgb888_alpha_blending_direct(
                                            ptTask->Param.tCopy.pSource,
                                            ptTask->Param.tCopy.pTarget,
                                            ptTask->Param.tCopy.pTarget,
                                            ptTask->Param.tCopy.tCopySize.iHeight
                                                * ptTask->Param.tCopy.tCopySize.iWidth,
                                            this.chRatio);
                break;
            default:
                return (arm_fsm_rt_t)ARM_2D_ERR_NOT_SUPPORT;
        }
    } else {

        switch (OP_CORE.ptOp->Info.Colour.u3ColourSZ) {
            case ARM_2D_COLOUR_SZ_16BIT:
                __arm_2d_impl_rgb565_alpha_blending(
                                            ptTask->Param.tCopy.pSource,
                                            ptTask->Param.tCopy.iSourceStride,
                                            ptTask->Param.tCopy.pTarget,
                                            ptTask->Param.tCopy.iTargetStride,
                                            &(ptTask->Param.tCopy.tCopySize),
                                            this.chRatio);
                break;
            case ARM_2D_COLOUR_SZ_32BIT:
                __arm_2d_impl_rgb888_alpha_blending(
                                            ptTask->Param.tCopy.pSource,
                                            ptTask->Param.tCopy.iSourceStride,
                                            ptTask->Param.tCopy.pTarget,
                                            ptTask->Param.tCopy.iTargetStride,
                                            &(ptTask->Param.tCopy.tCopySize),
                                            this.chRatio);
                break;
            default:
                return (arm_fsm_rt_t)ARM_2D_ERR_NOT_SUPPORT;

        }
    }

    return arm_fsm_rt_cpl;
}



/*----------------------------------------------------------------------------*
 * Draw a point whose cordinates is stored as float point.                    *
 *----------------------------------------------------------------------------*/

static arm_2d_region_t *__arm_2d_calculate_region(  const arm_2d_point_float_t *ptLocation,
                                                    arm_2d_region_t *ptRegion)
{
    assert(NULL != ptLocation);
    assert(NULL != ptRegion);

    /* +-----+-----+
     * |  P0 |  P1 |
     * +---- p ----+
     * |  P2 |  -- |
     * +-----+-----+
     */

    arm_2d_location_t tPoints[3];

    tPoints[0].iX = (int16_t)ptLocation->fX;
    tPoints[2].iX = (int16_t)ptLocation->fX;
    tPoints[1].iX = (int16_t)(ptLocation->fX + 0.99f);
    ptRegion->tSize.iWidth = tPoints[1].iX - tPoints[0].iX + 1;

    tPoints[0].iY = (int16_t)ptLocation->fY;
    tPoints[2].iY = (int16_t)ptLocation->fY;
    tPoints[1].iY = (int16_t)(ptLocation->fY + 0.99f);
    ptRegion->tSize.iHeight = tPoints[2].iY - tPoints[0].iY + 1;

    ptRegion->tLocation = tPoints[0];

    return ptRegion;
}


/*----------------------------------------------------------------------------*
 * Blend tile and background with a specified transparency ratio(0~255) and a *
 * specified transparency color mask                                          *
 *----------------------------------------------------------------------------*/
ARM_NONNULL(1,2,3)
arm_fsm_rt_t arm_2d_rbg565_alpha_blending_with_colour_masking(
                                            const arm_2d_tile_t *ptSource,
                                            const arm_2d_tile_t *ptTarget,
                                            const arm_2d_region_t *ptRegion,
                                            uint_fast8_t chRatio,
                                            arm_2d_color_rgb565_t tColour)
{
    assert(NULL != ptSource);
    assert(NULL != ptTarget);

    ARM_2D_IMPL(arm_2d_op_alpha_cl_msk_t);
    memset(ptThis, 0, sizeof(*ptThis));

    OP_CORE.ptOp =
        &ARM_2D_OP_TABLE[
            ARM_2D_OP_ALPHA_BLENDING_WITH_COLOUR_MASKING_RGB565];

    this.Target.ptTile = ptTarget;
    this.Target.ptRegion = ptRegion;
    this.Source.ptTile = ptSource;
    //this.wMode = 0;
    this.chRatio = chRatio;
    this.hwColour = tColour.tValue;

    return __arm_2d_op_invoke(NULL);
}


ARM_NONNULL(1,2,3)
arm_fsm_rt_t arm_2d_rbg888_alpha_blending_with_colour_masking(
                                            const arm_2d_tile_t *ptSource,
                                            const arm_2d_tile_t *ptTarget,
                                            const arm_2d_region_t *ptRegion,
                                            uint_fast8_t chRatio,
                                            arm_2d_color_rgb888_t tColour)
{
    assert(NULL != ptSource);
    assert(NULL != ptTarget);

    ARM_2D_IMPL(arm_2d_op_alpha_cl_msk_t);
    memset(ptThis, 0, sizeof(*ptThis));

    OP_CORE.ptOp =
        &ARM_2D_OP_TABLE[
            ARM_2D_OP_ALPHA_BLENDING_WITH_COLOUR_MASKING_RGB888];

    this.Target.ptTile = ptTarget;
    this.Target.ptRegion = ptRegion;
    this.Source.ptTile = ptSource;
    //this.wMode = 0;
    this.chRatio = chRatio;
    this.wColour = tColour.tValue;

    return __arm_2d_op_invoke(NULL);
}

arm_fsm_rt_t __arm_2d_sw_alpha_blending_with_colour_masking(
                                        __arm_2d_sub_task_t *ptTask)
{
    ARM_2D_IMPL(arm_2d_op_alpha_cl_msk_t, ptTask->ptOP)


    switch (OP_CORE.ptOp->Info.Colour.u3ColourSZ) {
        case ARM_2D_COLOUR_SZ_16BIT:
            __arm_2d_impl_rgb565_alpha_blending_colour_masking(
                                            ptTask->Param.tCopy.pSource ,
                                            ptTask->Param.tCopy.iSourceStride,
                                            ptTask->Param.tCopy.pTarget,
                                            ptTask->Param.tCopy.iTargetStride,
                                            &ptTask->Param.tCopy.tCopySize,
                                            this.chRatio,
                                            this.hwColour);
            break;
        case ARM_2D_COLOUR_SZ_32BIT:
            __arm_2d_impl_rgb888_alpha_blending_colour_masking(
                                            ptTask->Param.tCopy.pSource ,
                                            ptTask->Param.tCopy.iSourceStride,
                                            ptTask->Param.tCopy.pTarget,
                                            ptTask->Param.tCopy.iTargetStride,
                                            &ptTask->Param.tCopy.tCopySize,
                                            this.chRatio,
                                            this.wColour);
            break;
        default:
            return (arm_fsm_rt_t)ARM_2D_ERR_NOT_SUPPORT;

    }

    return arm_fsm_rt_cpl;
}

/*----------------------------------------------------------------------------*
 * Accelerable Low Level APIs                                                 *
 *----------------------------------------------------------------------------*/

__WEAK
void __arm_2d_impl_rgb565_alpha_blending(   uint16_t *__RESTRICT phwSourceBase,
                                    int16_t iSourceStride,
                                    uint16_t *__RESTRICT phwTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast8_t chRatio)
{
    uint32_t iHeight = ptCopySize->iHeight;
    uint32_t iWidth  = ptCopySize->iWidth;

    for (uint32_t y = 0; y < iHeight; y++) {

#if (defined (__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ == 1))
        /* M55 NOMVE optimization */
        register unsigned loopCnt  __asm("lr");
        loopCnt = iWidth;

        __asm volatile(
            "   dls             lr, lr                                  \n"
            ".p2align 2                                                 \n"
            "1:                                                         \n"
            /* read source / target pixel  */
            "   ldrh            r4, [%[src]], #2                        \n"
            "   ldrh            r5, [%[pTarget]]                        \n"

            /* unpack R */
            "   and             r0, r4, #31                             \n"
            "   and             r1, r5, #31                             \n"

            /* unpack B */
            "   lsr             r2, r4, #11                             \n"
            "   lsr             r3, r5, #11                             \n"

            /* unpack G */
            "   ubfx            r4, r4, #5, #6                          \n"
            "   ubfx            r5, r5, #5, #6                          \n"

            /* mix */
            "   mul             r0, r0, %[ratio0]                       \n"
            "   mul             r4, r4, %[ratio0]                       \n"
            "   mul             r2, r2, %[ratio0]                       \n"

            "   mla             r0, %[ratio1], r1, r0                   \n"
            "   mla             r1, %[ratio1], r5, r4                   \n"

            /* pack R */
            "   ubfx            r0, r0, #8, #5                          \n"
            /* shift & mask G */
            "   and             r1, %[cst], r1, lsr #3                  \n"
            /* MLA moved here to fill stall */
            "   mla             r5, %[ratio1], r3, r2                   \n"

            /* pack G */
            "   add             r0, r0, r1                              \n"
            /* pack R */
            "   bic             r1, r5, #255                            \n"
            "   orr             r0, r0, r1, lsl #3                      \n"

            "   strh            r0, [%[pTarget]], #2                    \n"
            "   le              lr, 1b                                  \n"

            : [src] "+r" (phwSourceBase), [pTarget] "+r" (phwTargetBase),
              [cnt] "+r" (loopCnt)
            : [ratio1] "r" ((256 - (uint_fast16_t)chRatio) ),
              [ratio0] "r" (chRatio), [cst] "r" (0x7e0 /* mask G */)
            : "r0", "r1", "r2", "r3",
              "r4", "r5", "memory"
        );
#else
        uint16_t        ratioCompl = 256 - chRatio;
        for (uint32_t x = 0; x < iWidth; x++) {
            __arm_2d_color_fast_rgb_t srcPix, targetPix;

            __arm_2d_rgb565_unpack(*phwSourceBase++, &srcPix);
            __arm_2d_rgb565_unpack(*phwTargetBase, &targetPix);

            for (int i = 0; i < 3; i++) {
                uint16_t        tmp =
                    (uint16_t) (srcPix.RGB[i] * chRatio) +
                    (targetPix.RGB[i] * ratioCompl);
                targetPix.RGB[i] = (uint16_t) (tmp >> 8);
            }
            /* pack merged stream */
            *phwTargetBase++ = __arm_2d_rgb565_pack(&targetPix);
        }
#endif
        phwSourceBase += (iSourceStride - iWidth);
        phwTargetBase += (iTargetStride - iWidth);
    }
}

__WEAK
void __arm_2d_impl_rgb888_alpha_blending(   uint32_t *__RESTRICT pwSourceBase,
                                            int16_t iSourceStride,
                                            uint32_t *__RESTRICT pwTargetBase,
                                            int16_t iTargetStride,
                                            arm_2d_size_t *__RESTRICT ptCopySize,
                                            uint_fast8_t chRatio)
{

    for (int_fast16_t y = 0; y < ptCopySize->iHeight; y++) {

        const uint32_t *pwSource = pwSourceBase;
        uint32_t *pwTarget = pwTargetBase;

        for (int_fast16_t x = 0; x < ptCopySize->iWidth; x++) {

            uint_fast8_t n = sizeof(uint32_t);
            const uint8_t *pchSrc = (uint8_t *)(pwSource++);
            uint8_t *pchDes = (uint8_t *)(pwTarget++);

            do {
                *pchDes = ( ((uint_fast16_t)(*pchSrc++) * chRatio)
                          + (   (uint_fast16_t)(*pchDes)
                            *   (256 - (uint_fast16_t)chRatio))) >> 8;
                 pchDes++;
            } while(--n);
        }

        pwSourceBase += iSourceStride;
        pwTargetBase += iTargetStride;
    }

}

__WEAK
void __arm_2d_impl_rgb565_alpha_blending_direct(const uint16_t *phwSource,
                                                const uint16_t *phwBackground,
                                                uint16_t *phwDestination,
                                                uint32_t wPixelCount,
                                                uint_fast8_t chRatio)
{
#if (defined (__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ == 1))
        /* M55 NOMVE optimization */
        register unsigned loopCnt  __asm("lr");
        loopCnt = wPixelCount;

        __asm volatile(
            "   dls             lr, lr                                  \n"
            ".p2align 2                                                 \n"
            "1:                                                         \n"
            /* read source / target pixel  */
            "   ldrh            r4, [%[src]], #2                        \n"
            "   ldrh            r5, [%[backgrd]], #2                    \n"

            /* unpack R */
            "   and             r0, r4, #31                             \n"
            "   and             r1, r5, #31                             \n"

            /* unpack B */
            "   lsr             r2, r4, #11                             \n"
            "   lsr             r3, r5, #11                             \n"

            /* unpack G */
            "   ubfx            r4, r4, #5, #6                          \n"
            "   ubfx            r5, r5, #5, #6                          \n"

            /* mix */
            "   mul             r0, r0, %[ratio0]                       \n"
            "   mul             r4, r4, %[ratio0]                       \n"
            "   mul             r2, r2, %[ratio0]                       \n"

            "   mla             r0, %[ratio1], r1, r0                   \n"
            "   mla             r1, %[ratio1], r5, r4                   \n"

            /* pack R */
            "   ubfx            r0, r0, #8, #5                          \n"
            /* shift & mask G */
            "   and             r1, %[cst], r1, lsr #3                  \n"
            /* MLA moved here to fill stall */
            "   mla             r5, %[ratio1], r3, r2                   \n"

            /* pack G */
            "   add             r0, r0, r1                              \n"
            /* pack R */
            "   bic             r1, r5, #255                            \n"
            "   orr             r0, r0, r1, lsl #3                      \n"

            "   strh            r0, [%[pTarget]], #2                    \n"
            "   le              lr, 1b                                  \n"

            : [src] "+r" (phwSource), [backgrd] "+r" (phwBackground),
              [pTarget] "+r" (phwDestination), [cnt] "+r" (loopCnt)
            : [ratio1] "r" ((256 - (uint_fast16_t)chRatio) ),
              [ratio0] "r" (chRatio), [cst] "r" (0x7e0 /* mask G */)
            : "r0", "r1", "r2", "r3",
              "r4", "r5", "memory"
        );
#else
    do {
        __arm_2d_color_fast_rgb_t wSourcePixel, wBackgroundPixel, wTargetPixel;
        uint16_t        ratioCompl = 256 - chRatio;

        __arm_2d_rgb565_unpack(*phwSource++, &wSourcePixel);
        __arm_2d_rgb565_unpack(*phwBackground++, &wBackgroundPixel);

        for (int i = 0; i < 3; i++) {
            uint16_t        tmp =
                (uint16_t) (wSourcePixel.RGB[i] * chRatio) +
                (wBackgroundPixel.RGB[i] * ratioCompl);
            wTargetPixel.RGB[i] = (uint16_t) (tmp >> 8);
        }

        *phwDestination++ = __arm_2d_rgb565_pack(&wTargetPixel);
    } while (--wPixelCount);

#endif
}

__WEAK
void __arm_2d_impl_rgb888_alpha_blending_direct(const uint32_t *pwSource,
                                                const uint32_t *pwBackground,
                                                uint32_t *pwDestination,
                                                uint32_t wPixelCount,
                                                uint_fast8_t chRatio)
{
    do {
        uint_fast8_t n = sizeof(uint32_t);
        const uint8_t *pchSrc = (uint8_t *)(pwSource++);
        const uint8_t *pchBG = (uint8_t *)(pwBackground++);
        uint8_t *pchDes = (uint8_t *)(pwDestination++);

        do {
            *pchDes++ = (   ((uint_fast16_t)(*pchSrc++) * chRatio)
                        +   (   (uint_fast16_t)(*pchBG++)
                            *   (256 - (uint_fast16_t)chRatio))) >> 8;
        } while(--n);

    } while(--wPixelCount);
}




__WEAK
void __arm_2d_impl_rgb565_alpha_blending_colour_masking(
                                                uint16_t * __RESTRICT phwSource,
                                                int16_t         iSourceStride,
                                                uint16_t * __RESTRICT phwTarget,
                                                int16_t         iTargetStride,
                                                arm_2d_size_t * __RESTRICT ptCopySize,
                                                uint_fast8_t    chRatio,
                                                uint_fast16_t   hwColour)
{
    uint32_t        iHeight = ptCopySize->iHeight;
    uint32_t        iWidth = ptCopySize->iWidth;
    uint16_t        ratioCompl = 256 - chRatio;

    for (uint32_t y = 0; y < iHeight; y++) {
        for (uint32_t x = 0; x < iWidth; x++) {

            if (*phwSource != (uint16_t) hwColour) {
                __arm_2d_color_fast_rgb_t srcPix, targetPix;

                __arm_2d_rgb565_unpack(*phwSource, &srcPix);
                __arm_2d_rgb565_unpack(*phwTarget, &targetPix);

                for (int i = 0; i < 3; i++) {
                    uint16_t        tmp =
                        (uint16_t) (srcPix.RGB[i] * chRatio) +
                        (targetPix.RGB[i] * ratioCompl);
                    srcPix.RGB[i] = (uint16_t) (tmp >> 8);
                }
                /* pack merged stream */
                *phwTarget = __arm_2d_rgb565_pack(&srcPix);
            }
            phwSource++;
            phwTarget++;
        }
        phwSource += (iSourceStride - iWidth);
        phwTarget += (iTargetStride - iWidth);
    }
}


__WEAK
void __arm_2d_impl_rgb888_alpha_blending_colour_masking(
                                        uint32_t *__RESTRICT pwSourceBase,
                                        int16_t iSourceStride,
                                        uint32_t *__RESTRICT pwTargetBase,
                                        int16_t iTargetStride,
                                        arm_2d_size_t *__RESTRICT ptCopySize,
                                        uint_fast8_t chRatio,
                                        uint_fast32_t wColour)
{
    for (int_fast16_t y = 0; y < ptCopySize->iHeight; y++) {

        const uint32_t *__RESTRICT pwSource = pwSourceBase;
        uint32_t *__RESTRICT pwTarget = pwTargetBase;

        for (int_fast16_t x = 0; x < ptCopySize->iWidth; x++) {

            if (*pwSource != wColour) {
                uint_fast8_t n = sizeof(uint32_t);
                const uint8_t *__RESTRICT pchSrc = (uint8_t *)(pwSource++);
                uint8_t *__RESTRICT pchDes = (uint8_t *)(pwTarget++);

                do {
                    *pchDes = ( ((uint16_t)(*pchSrc++) * chRatio)
                              + (   (uint16_t)(*pchDes)
                                *   (256 - (uint16_t)chRatio))) >> 8;
                     pchDes++;
                } while(--n);
            }
        }

        pwSourceBase += iSourceStride;
        pwTargetBase += iTargetStride;
    }
}

#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#ifdef   __cplusplus
}
#endif

/*
 * Copyright (C) 2020 Arm Limited or its affiliates. All rights reserved.
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
 * Title:        #include "arm_2d.h"
 * Description:  Public header file to contain the all avaialble Arm-2D 
 *               interface header files 
 *
 * $Date:        01. December 2020
 * $Revision:    V.0.5.0
 *
 * Target Processor:  Cortex-M cores
 * -------------------------------------------------------------------- */
 
#ifndef __ARM_2D_H__
#define __ARM_2D_H__

/*============================ INCLUDES ======================================*/
#include "arm_2d_types.h"
#include "arm_2d_tile.h"
#include "arm_2d_draw.h"
#include "arm_2d_conversion.h"
#include "arm_2d_alpha_blending.h"

#ifdef   __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

typedef struct {
    uint8_t     TREAT_OUT_OF_RANGE_AS_COMPLETE          : 1;
    uint8_t     HAS_DEDICATED_THREAD_FOR_2D_TASK        : 1;
    uint8_t                                             : 6;
} arm_2d_runtime_feature_t;

/*============================ GLOBAL VARIABLES ==============================*/

extern
arm_2d_runtime_feature_t ARM_2D_RUNTIME_FEATURE;

/*============================ PROTOTYPES ====================================*/

#if defined(__ARM_2D_HAS_ASYNC__) && __ARM_2D_HAS_ASYNC__
/*! \brief initialise the whole arm-2d service
 *! \param none
 *! \return none
 */
extern
void __arm_2d_async_init(void);
#endif

#if defined(__ARM_2D_HAS_HELIUM__) && __ARM_2D_HAS_HELIUM__
/*! \brief initialise the helium service
 *! \param none
 *! \return none
 */
extern
void __arm_2d_helium_init(void);
#endif

#if defined(__ARM_2D_HAS_CDE__) && __ARM_2D_HAS_CDE__
/*! \brief initialise the cde service
 *! \param none
 *! \return none
 */
extern
void __arm_2d_cde_init(void);
#endif



#if defined(__ARM_2D_HAS_HW_ACC__) && __ARM_2D_HAS_HW_ACC__
/*! \brief initialise the hardware accelerator adapter
 *! \param none
 *! \return none
 */
extern
void __arm_2d_acc_init(void);
#endif


/*! \brief initialise the whole arm-2d service
 *! \param none
 *! \return none
 */
extern
void __arm_2d_init(void);

 /*! \brief arm-2d pixel pipeline task entery
  *! \note  This function is *TRHEAD-SAFE*. If you use dedicated RTOS thread 
  *!        to run arm_2d_task, please set runtime feature option
  *!        ARM_2D_RUNTIME_FEATURE.HAS_DEDICATED_THREAD_FOR_2D_TASK to true(1).
  *!
  *! \param none
  *! \retval arm_fsm_rt_cpl The sub-task FIFO is empty, the caller i.e. the host
  *!            RTOS thread can block itself by waiting for a semphare which is
  *!            set by arm_2d_notif_sub_task_fifo_task_arrive()
  *! \retval arm_fsm_rt_on_going The arm-2d pixel pipleline is just finished 
  *!            handling one operation and decide to yield. You should keep 
  *!            calling arm_2d_task again.
  *! \retval (<0) Errors are detected.
  */
extern
arm_fsm_rt_t arm_2d_task(void);

#ifdef   __cplusplus
}
#endif


#endif

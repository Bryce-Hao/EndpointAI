/*
 * Copyright (C) 2010-2021 Arm Limited or its affiliates. All rights reserved.
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
 * Title:        arm-2d_async.c
 * Description:  Basic Tile operations
 *
 * $Date:        29. Jan 2021
 * $Revision:    V.0.5.0
 *
 * Target Processor:  Cortex-M cores
 *
 * -------------------------------------------------------------------- */


/*============================ INCLUDES ======================================*/
#define __ARM_2D_IMPL__

#include "arm_2d.h"
#include "__arm_2d_impl.h"

#ifdef   __cplusplus
extern "C" {
#endif

#if defined(__ARM_2D_HAS_ASYNC__) && __ARM_2D_HAS_ASYNC__

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
#   pragma clang diagnostic ignored "-Wsign-compare"
#   pragma clang diagnostic ignored "-Wmissing-prototypes"
#   pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#   pragma clang diagnostic ignored "-Wswitch-enum"
#   pragma clang diagnostic ignored "-Wswitch"
#endif

/*============================ MACROS ========================================*/
#ifndef __ARM_2D_DEFAULT_SUB_TASK_POOL_SIZE
#   define __ARM_2D_DEFAULT_SUB_TASK_POOL_SIZE        4
#endif
#if __ARM_2D_DEFAULT_SUB_TASK_POOL_SIZE < 4
#   warning The __ARM_2D_DEFAULT_SUB_TASK_POOL_SIZE should be larger than or\
 equal to 3, set it to the default value 4.
#   undef __ARM_2D_DEFAULT_SUB_TASK_POOL_SIZE
#   define __ARM_2D_DEFAULT_SUB_TASK_POOL_SIZE    4
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ IMPLEMENTATION ================================*/

/*----------------------------------------------------------------------------*
 * Subtask Pool                                                               *
 *----------------------------------------------------------------------------*/

static void __arm_2d_sub_task_booking(uint_fast16_t hwCount)
{
    arm_irq_safe {
        ARM_2D_CTRL.hwBookCount += hwCount;
    }
}

static void __arm_2d_sub_task_cancel_booking(void)
{
    arm_irq_safe {
        ARM_2D_CTRL.hwBookCount = 0;
    }
}

static uint_fast16_t __arm_2d_sub_task_available_count(void)
{
    uint_fast16_t hwResult = 0;
    
    arm_irq_safe {
        hwResult = ARM_2D_CTRL.hwFreeCount;
        hwResult = (hwResult > ARM_2D_CTRL.hwBookCount)
                    ?   hwResult - ARM_2D_CTRL.hwBookCount
                    :   0;
    }
    
    return hwResult;
}

static
__arm_2d_sub_task_t *__arm_2d_sub_task_new(void)
{
    __arm_2d_sub_task_t *ptTask = NULL;
    
    arm_irq_safe {    
        if (NULL != ARM_2D_CTRL.ptFreeList) {
            ARM_LIST_STACK_POP(ARM_2D_CTRL.ptFreeList, ptTask);
            if (ARM_2D_CTRL.hwBookCount) {
                ARM_2D_CTRL.hwBookCount--;
            }
            
            ARM_2D_CTRL.hwFreeCount--;
        }
    }
    
    memset(ptTask, 0 ,sizeof(__arm_2d_sub_task_t));
    
    return ptTask;
}

static 
void __arm_2d_sub_task_free(__arm_2d_sub_task_t *ptTask)
{
    if (NULL == ptTask) {
        return ;
    }
    
    arm_irq_safe {
        ARM_LIST_STACK_PUSH(ARM_2D_CTRL.ptFreeList, ptTask);
        ARM_2D_CTRL.hwFreeCount++;
    }
}

/*----------------------------------------------------------------------------*
 * Subtask FIFO                                                               *
 *----------------------------------------------------------------------------*/

__WEAK 
void arm_2d_notif_sub_task_fifo_task_arrive(void)
{
}

static 
void __arm_2d_sub_task_add(__arm_2d_sub_task_t *ptTask)
{
    assert(NULL != ptTask);
    bool bIsEmpty = false;
    arm_irq_safe {
        bIsEmpty = (0 == ARM_2D_CTRL.hwTaskCount);
        
        ARM_LIST_QUEUE_ENQUEUE( ARM_2D_CTRL.TaskFIFO.ptHead, 
                                ARM_2D_CTRL.TaskFIFO.ptTail,
                                ptTask);
        ARM_2D_CTRL.hwTaskCount++;
    }
    
    if (bIsEmpty) {
        arm_2d_notif_sub_task_fifo_task_arrive();
    }
}

static 
__arm_2d_sub_task_t * __arm_2d_sub_task_fetch(void)
{
    __arm_2d_sub_task_t *ptTask = NULL;
    
    arm_irq_safe {
        if (NULL != ARM_2D_CTRL.TaskFIFO.ptHead) {
            ARM_LIST_QUEUE_DEQUEUE( ARM_2D_CTRL.TaskFIFO.ptHead, 
                                    ARM_2D_CTRL.TaskFIFO.ptTail,
                                    ptTask);
            ARM_2D_CTRL.hwTaskCount--;
        }
    }
    
    return ptTask;
}


/*----------------------------------------------------------------------------*
 * Subtask Processing                                                         *
 *----------------------------------------------------------------------------*/
 
static 
arm_fsm_rt_t __arm_2d_call_default_io(  __arm_2d_sub_task_t *ptTask, 
                                        uint_fast8_t chInterfaceIndex)
{
    assert(NULL != ptTask);

    arm_fsm_rt_t tResult = (arm_fsm_rt_t)ARM_2D_ERR_NOT_SUPPORT;
    ARM_2D_IMPL(arm_2d_op_t, ptTask->ptOP);
    uint_fast8_t chAccPreference = OP_CORE.Preference.u2ACCMethods;
    
    do {
        if (chAccPreference != ARM_2D_PREF_ACC_SW_ONLY) {
        /* call hardware accelerator */
        /*! \note the HW accelerator service routine will check the paratmer
         *!       and decide whether the requested service is supported or
         *!       not:
         *!       - if it is not supported, ARM_2D_ERR_NOT_SUPPORT will be
         *!         returned. 
         *!
         *!       - if it is supported and the service is to be done later,
         *!         the sub-task will be queued and arm_fsm_rt_async is 
         *!         returned. Once the sub-task is complete, 
         *!         __arm_2d_notify_sub_task_cpl() should be called by HW 
         *!         accelerator service routine/tasks.
         *!
         *!       - if it is supported and the service is complete immediately
         *!         the arm_fsm_rt_cpl is returned. 
         *!
         *!       - if there are any error detected, negative error code will be
         *!         returned. 
         */
             
            ARM_2D_TRY_ACCELERATION( chInterfaceIndex, __arm_2d_io_func_t ); 
                            
            assert(tResult != arm_fsm_rt_on_going);
            assert(tResult != arm_fsm_rt_wait_for_obj);
            
            switch (tResult) {
                case arm_fsm_rt_wait_for_obj:
                case arm_fsm_rt_on_going:
                    /*! \note HW accelerator should NOT return on_going or 
                     *!       wait_for_obj 
                     */
                    if (chAccPreference == ARM_2D_PREF_ACC_DONT_CARE) {
                        /* since people choose don't care, then use SW */
                        break;
                    }
                    return (arm_fsm_rt_t)ARM_2D_ERR_IO_BUSY;
                case ARM_2D_ERR_NOT_SUPPORT:
                    break;
                
                case arm_fsm_rt_async:
                case arm_fsm_rt_cpl:
                default:
                    /* other errors */
                    return tResult;
            }
             
            if (    (chAccPreference == ARM_2D_PREF_ACC_HW_ONLY)  
                &&  (ARM_2D_ERR_NOT_SUPPORT == tResult)) {                
                //! the hardware acceleration isn't avaialble
                break;
            }
        }

        /* call default software implementation */
        /*! \note the default software implemenation will only return following
         *!       values:
         *!       - arm_fsm_rt_cpl   the service is complete.
         *!       - negative error code
         *!       
         *! \note the following value will not be returned:
         *!       - arm_fsm_rt_on_going
         *!       - arm_fsm_rt_wait_for_obj
         */
         
        ARM_2D_RUN_DEFAULT(  chInterfaceIndex, __arm_2d_io_func_t  );
                            
        assert(tResult != arm_fsm_rt_on_going);
        assert(tResult != arm_fsm_rt_wait_for_obj);
    } while(0);
    return tResult;
}

static 
arm_fsm_rt_t __arm_2d_call_default_tile_process_io(__arm_2d_sub_task_t *ptTask)
{
    assert(NULL != ptTask);

    arm_fsm_rt_t tResult = (arm_fsm_rt_t)ARM_2D_ERR_NOT_SUPPORT;
    ARM_2D_IMPL(arm_2d_op_t, ptTask->ptOP);
    uint_fast8_t chAccPreference = OP_CORE.Preference.u2ACCMethods;
    
    do {
        if (chAccPreference != ARM_2D_PREF_ACC_SW_ONLY) {
        /* call hardware accelerator */
        /*! \note the HW accelerator service routine will check the paratmer
         *!       and decide whether the requested service is supported or
         *!       not:
         *!       - if it is not supported, ARM_2D_ERR_NOT_SUPPORT will be
         *!         returned. 
         *!
         *!       - if it is supported and the service is to be done later,
         *!         the sub-task will be queued and arm_fsm_rt_async is 
         *!         returned. Once the sub-task is complete, 
         *!         __arm_2d_notify_sub_task_cpl() should be called by HW 
         *!         accelerator service routine/tasks.
         *!
         *!       - if it is supported and the service is complete immediately
         *!         the arm_fsm_rt_cpl is returned. 
         *!
         *!       - if there are any error detected, negative error code will be
         *!         returned. 
         */
            ARM_2D_TRY_ACCELERATION(  
                            OP_CORE.ptOp->Info.LowLevelInterfaceIndex.TileProcessLike, 
                            __arm_2d_io_tile_default_tile_process_t,
                            ptTask->Param.TileProcess.pTarget,
                            ptTask->Param.TileProcess.iStride,
                            &(ptTask->Param.TileProcess.tSize));
                            
            assert(tResult != arm_fsm_rt_on_going);
            assert(tResult != arm_fsm_rt_wait_for_obj);
            
            switch (tResult) {
                case arm_fsm_rt_wait_for_obj:
                case arm_fsm_rt_on_going:
                    /*! \note HW accelerator should NOT return on_going or 
                     *!       wait_for_obj 
                     */
                    if (chAccPreference == ARM_2D_PREF_ACC_DONT_CARE) {
                        /* since people choose don't care, then use SW */
                        break;
                    }
                    return (arm_fsm_rt_t)ARM_2D_ERR_IO_BUSY;
                case ARM_2D_ERR_NOT_SUPPORT:
                    break;
                
                case arm_fsm_rt_async:
                case arm_fsm_rt_cpl:
                default:
                    /* other errors */
                    return tResult;
            }
             
            if (    (chAccPreference == ARM_2D_PREF_ACC_HW_ONLY)  
                &&  (ARM_2D_ERR_NOT_SUPPORT == tResult)) {                
                //! the hardware acceleration isn't avaialble
                break;
            }
        }
    
        /* call default software implementation */
        /*! \note the default software implemenation will only return following
         *!       values:
         *!       - arm_fsm_rt_cpl   the service is complete.
         *!       - negative error code
         *!       
         *! \note the following value will not be returned:
         *!       - arm_fsm_rt_on_going
         *!       - arm_fsm_rt_wait_for_obj
         */
        ARM_2D_RUN_DEFAULT(       
                            OP_CORE.ptOp->Info.LowLevelInterfaceIndex.TileProcessLike, 
                            __arm_2d_io_tile_default_tile_process_t,
                            ptTask->Param.TileProcess.pTarget,
                            ptTask->Param.TileProcess.iStride,
                            &(ptTask->Param.TileProcess.tSize));
                            
        assert(tResult != arm_fsm_rt_on_going);
        assert(tResult != arm_fsm_rt_wait_for_obj);
    } while(0);
    return tResult;
}


__WEAK 
void arm_2d_notif_aync_sub_task_cpl(void)
{
}

/*! \note This API should be called by both arm_2d_task and hardware 
 *!       accelerator to indicate the completion of a sub task
 */
void __arm_2d_notify_sub_task_cpl(__arm_2d_sub_task_t *ptTask, 
                                    arm_fsm_rt_t tResult,
                                    bool bFromHW)
{
    assert(NULL != ptTask);
    assert(ptTask->ptOP->Status.u4SubTaskCount > 0);
    
    //! error detected
    if (tResult < 0) {
        //! update error info
        ptTask->ptOP->tResult = tResult;
        ptTask->ptOP->Status.bIOError = true;
    }
    
    if (arm_fsm_rt_async == tResult){
        if (ptTask->ptOP->tResult >= 0) {
            ptTask->ptOP->tResult = arm_fsm_rt_async;
        }
    }
    
    //! handle target OP
    if (arm_fsm_rt_async != tResult) {
        ptTask->ptOP->Status.u4SubTaskCount--;
        if (0 == ptTask->ptOP->Status.u4SubTaskCount) {
            //! this is the last sub task 
            
            
            //! call Operation Complete event handler
            if (NULL != ptTask->ptOP->evt2DOpCpl.fnHandler) {
                ptTask->ptOP->Status.bOpCpl = 
                    (ptTask->ptOP->evt2DOpCpl.fnHandler) (
                                            ptTask->ptOP, 
                                            ptTask->ptOP->tResult,
                                            ptTask->ptOP->evt2DOpCpl.pTarget);
            } else {
                /*! \note complete doesn't mean no err */
                ptTask->ptOP->Status.bOpCpl = true;
            }
            
            //! only clear busy flag after bOpCpl is set properly.
            ptTask->ptOP->Status.bIsBusy = false;
        }
    }
    
    //! free sub task
    __arm_2d_sub_task_free(ptTask);
    
    if (bFromHW) {
        arm_2d_notif_aync_sub_task_cpl();
    }
}

/*! \note You can override this to add support for new types of interface
 *! \param ptTask pointer of target sub task
 *! \return the result
 */ 
arm_fsm_rt_t __arm_2d_sub_task_dispatch(__arm_2d_sub_task_t *ptTask)
{
    arm_fsm_rt_t tResult = (arm_fsm_rt_t)ARM_2D_ERR_INVALID_OP;
    ARM_2D_IMPL(arm_2d_op_t, ptTask->ptOP);
    
    //! call low level io
    switch (ptTask->chIOType) {
        case __ARM_2D_IO_TYPE_DEFAULT_TILE_PROCESS:
            tResult = __arm_2d_call_default_tile_process_io(ptTask);
            break;
    
        case __ARM_2D_IO_TYPE_DEFAULT_COPY: 
            tResult = __arm_2d_call_default_io(
                            ptTask, 
                            OP_CORE.ptOp->Info.LowLevelInterfaceIndex.CopyLike);
            break;
            
        case __ARM_2D_IO_TYPE_DEFAULT_FILL:                                   
            tResult = __arm_2d_call_default_io(
                            ptTask, 
                            OP_CORE.ptOp->Info.LowLevelInterfaceIndex.FillLike);
            break;
    }
    
    return tResult;
}


 /*! \brief arm-2d pixel pipeline task entery
  *! \note  This function is *TRHEAD-SAFE*
  *! \param none
  *! \retval arm_fsm_rt_cpl The sub-task FIFO is empty, the caller, i.e. the host
  *!            RTOS thread can block itself by waiting for a semaphore which is
  *!            set by arm_2d_notif_sub_task_fifo_task_arrive()
  *! \retval arm_fsm_rt_async The arm-2d pixel pipleline is waiting hw 
  *!            accelerator to finish a sub task. RTOS thread can block itself by
  *!            waiting for a semaphore which is set by 
  *!            arm_2d_notif_aync_sub_task_cpl()
  *! \retval (<0) Errors are detected.
  */
arm_fsm_rt_t arm_2d_task(void)
{

    arm_fsm_rt_t tResult = (arm_fsm_rt_t)ARM_2D_ERR_INVALID_OP;
        
    do {
        //! fetch a sub task from FIFO
        __arm_2d_sub_task_t *ptTask = __arm_2d_sub_task_fetch();
        if (NULL == ptTask) {
            return arm_fsm_rt_cpl;
        }
        
        //! try-loop for dispatching sub task
        do {
            //! dispatch sub tasks
            tResult = __arm_2d_sub_task_dispatch(ptTask);
            
            assert(tResult != arm_fsm_rt_on_going);
            assert(tResult != arm_fsm_rt_wait_for_obj);
            
            if (tResult != arm_fsm_rt_async) {
                //! get result immediately
                __arm_2d_notify_sub_task_cpl(ptTask, tResult, false);
                break;                                                          //!< move to next task
            }
       
        } while(true);
        
        //! unsupported operation
        if (ARM_2D_ERR_INVALID_OP == tResult){
            break;
        }

    } while(true);
    
    
    return tResult;
}


/*----------------------------------------------------------------------------*
 * Overridden Implementations                                                 *
 *----------------------------------------------------------------------------*/

__OVERRIDE_WEAK
bool __arm_2d_op_ensure_resource(   arm_2d_op_core_t *ptOP, 
                                    uint_fast16_t hwRequired)
{
    bool bResult = false;
    
    ARM_2D_UNUSED(ptOP);
    
    arm_irq_safe {
        bResult = (__arm_2d_sub_task_available_count() >= hwRequired);
        if (bResult) {
            __arm_2d_sub_task_booking(hwRequired);
        }
    }

    return bResult;
}

__OVERRIDE_WEAK
arm_fsm_rt_t __arm_2d_op_frontend_on_leave( arm_2d_op_core_t *ptThis, 
                                            arm_fsm_rt_t tResult)
{

    if (    (arm_fsm_rt_cpl == tResult) 
        ||  ((arm_fsm_rt_t)ARM_2D_ERR_OUT_OF_REGION == tResult)
        ||  (tResult < 0)) {
        
        this.Status.bIsBusy = 0;
        this.Status.bOpCpl = true;
        this.Status.bIOError = (tResult < 0);
        this.tResult = tResult;
    } 
  
    if (!ARM_2D_RUNTIME_FEATURE.HAS_DEDICATED_THREAD_FOR_2D_TASK) {
        arm_2d_task();
    }
    
    /* release resources here */
    __arm_2d_sub_task_cancel_booking();
        
    return tResult;
}

__OVERRIDE_WEAK
arm_fsm_rt_t __arm_2d_issue_sub_task_tile_process(  
                                        arm_2d_op_t *ptThis,
                                        void *__RESTRICT pTargetBase,
                                        int16_t iTargetStride,
                                        arm_2d_size_t *__RESTRICT ptTargetSize)
{    
    __arm_2d_sub_task_t *ptTask = __arm_2d_sub_task_new();
    assert(NULL != ptTask);         

    (*ptTask) = (__arm_2d_sub_task_t) {
                    .ptOP = &(ptThis->use_as__arm_2d_op_core_t),
                    .chIOType = __ARM_2D_IO_TYPE_DEFAULT_TILE_PROCESS,
                    .Param.TileProcess = {           
                        .pTarget = pTargetBase,                               
                        .iStride = iTargetStride,         
                        .tSize = *ptTargetSize,
                    },
                };
    
    OP_CORE.Status.u4SubTaskCount++;
    
    __arm_2d_sub_task_add(ptTask);
    
    return arm_fsm_rt_async;
}

__OVERRIDE_WEAK
arm_fsm_rt_t __arm_2d_issue_sub_task_fill(arm_2d_op_cp_t *ptThis,
                                    void *__RESTRICT pSourceBase,
                                    int16_t iSourceStride,
                                    arm_2d_region_t *__RESTRICT ptSourceRegion,
                                    void *__RESTRICT pTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_region_t *__RESTRICT ptTargetRegion)
{
    __arm_2d_sub_task_t *ptTask = __arm_2d_sub_task_new();
    assert(NULL != ptTask);         

    (*ptTask) = (__arm_2d_sub_task_t) {
                    .ptOP = &(ptThis->use_as__arm_2d_op_core_t),
                    .chIOType = __ARM_2D_IO_TYPE_DEFAULT_FILL,
                    .Param.tFill = {
                        .pSource = pSourceBase,                               
                        .iSourceStride = iSourceStride,         
                        .tSourceRegion = *ptSourceRegion,                 
                        .pTarget = pTargetBase,                               
                        .iTargetStride = iTargetStride,         
                        .tTargetRegion = *ptTargetRegion,
                    },
                };
    
    OP_CORE.Status.u4SubTaskCount++;
    
    __arm_2d_sub_task_add(ptTask);

    return arm_fsm_rt_async;
}

__OVERRIDE_WEAK
arm_fsm_rt_t __arm_2d_issue_sub_task_copy(arm_2d_op_cp_t *ptThis,
                                    void *__RESTRICT pSource,
                                    int16_t iSourceStride,
                                    arm_2d_region_t *__RESTRICT ptSourceRegion,
                                    void *__RESTRICT pTarget,
                                    int16_t iTargetStride,
                                    arm_2d_region_t *__RESTRICT ptTargetRegion,
                                    arm_2d_size_t * __RESTRICT ptCopySize)
{
    __arm_2d_sub_task_t *ptTask = __arm_2d_sub_task_new();
    assert(NULL != ptTask);         

    (*ptTask) = (__arm_2d_sub_task_t) {
                    .ptOP = &(ptThis->use_as__arm_2d_op_core_t),
                    .chIOType = __ARM_2D_IO_TYPE_DEFAULT_COPY,
                    .Param.tCopy = {
                        .pSource = pSource,                               
                        .iSourceStride = iSourceStride,         
                        .tSourceRegion = *ptSourceRegion,                 
                        .pTarget = pTarget,                               
                        .iTargetStride = iTargetStride,         
                        .tTargetRegion = *ptTargetRegion,
                        .tCopySize = *ptCopySize,
                    },
                };
    OP_CORE.Status.u4SubTaskCount++;
    
    __arm_2d_sub_task_add(ptTask);

    return arm_fsm_rt_async;
}

/*! \brief initialise the whole arm-2d service
 *! \param none
 *! \return none
 */
void __arm_2d_async_init(void)
{   
    //! initialise sub task pool
    do {
        static __arm_2d_sub_task_t 
            s_tDefaultTaskPool[__ARM_2D_DEFAULT_SUB_TASK_POOL_SIZE];
        
        arm_foreach(__arm_2d_sub_task_t, s_tDefaultTaskPool) {
            __arm_2d_sub_task_free(_);
        }
        
    } while(0);
}



#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#ifdef   __cplusplus
}
#endif

#endif

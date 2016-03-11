/*
 * all.h
 *
 *  Created on: 2016-3-2
 *      Author: root
 */

#ifndef THREAD_ALL_H_
#define THREAD_ALL_H_

#include "../pb.h"

/*
 * Macro use in thread_task.
 *  BINGO_THREAD_TASK_MUTEX_IMP_DEBUG
 *  BINGO_THREAD_TASK_ATOMIC_IMP_DEBUG
 */
#include "spinlock.h"
#include "task/thread_task.h"
#include "task/task_data.h"
#include "task/mutex_imp_one_to_one.h"
#include "task/mutex_imp_one_to_many.h"
#include "task/atomic_imp_one_to_one.h"
#include "task/atomic_imp_one_to_many.h"


#endif /* THREAD_ALL_H_ */

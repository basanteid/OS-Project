/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <kern/cpu/cpu.h>
#include <inc/string.h>
#include <inc/disk.h>

/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
// TODO: [PROJECT'26.GM#4] BLOCKING - [0] GIVEN FUNCTIONS
//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}
/************************************************************************************/

//=====================================================================================//
//============================== REQUIRED FUNCTIONS ===================================//
//=====================================================================================//
//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code
void sleep(struct Channel *chan, struct kspinlock *lk)
{
	// TODO: [PROJECT'26.GM#4] BLOCKING - [13] Channel sleep
	// Your code is here
	// Comment the following line
	// panic("sleep() is not implemented yet...!!");
	acquire_kspinlock(&ProcessQueues.qlock); // lock the q to prevent missed wake up
	struct Env *cur_p = get_cpu_proc();		 // get a copy of the address of the process on the CPU
	cur_p->env_status = ENV_BLOCKED;		 // makes it's status BLOCKED
	enqueue(&(chan->queue), cur_p);			 // push it in the channel (queue)
	release_kspinlock(lk);					 // release the spin lock on the sleep lock
	sched();								 // switch to another process
	release_kspinlock(&ProcessQueues.qlock); // release the lock on q
	acquire_kspinlock(lk);					 // lock the spin lock on the sleep lock
}

//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan)
{
	// TODO: [PROJECT'26.GM#4] BLOCKING - [14] Channel wakeup_one
	// Your code is here
	// Comment the following line
	// panic("wakeup_one() is not implemented yet...!!");

	acquire_kspinlock(&ProcessQueues.qlock);
	struct Env *cur_p;
	if (queue_size(&(chan->queue)) > 0)
	{
		cur_p = dequeue(&(chan->queue));
		// cur_p->env_status = ENV_READY;
		sched_insert_ready(cur_p);
	}
	release_kspinlock(&ProcessQueues.qlock);
}

//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_all(struct Channel *chan)
{
	// TODO: [PROJECT'26.GM#4] BLOCKING - [15] Channel wakeup_all
	// Your code is here
	// Comment the following line
	// panic("wakeup_all() is not implemented yet...!!");
	acquire_kspinlock(&ProcessQueues.qlock);
	struct Env *cur_p;
	while (queue_size(&(chan->queue)) > 0)
	{
		cur_p = dequeue(&(chan->queue));
		// cur_p->env_status = ENV_READY;
		sched_insert_ready(cur_p);
	}
	release_kspinlock(&ProcessQueues.qlock);
}
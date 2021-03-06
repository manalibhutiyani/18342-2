/**
 * @file device.c
 *
 * @brief Implements simulated devices.
 * @author Kartik Subramanian <ksubrama@andrew.cmu.edu>
 * @author Jeff Brandon <jdbrando@andrew.cmu.edu>
 * @author Keane Lucas <kjlucas@andrew.cmu.edu>
 * @date 2014-11-24
 */

#include <types.h>
#include <assert.h>

#include <task.h>
#include <sched.h>
#include <device.h>
#include <arm/reg.h>
#include <arm/psr.h>
#include <arm/exception.h>
#include <bits/errno.h>

void dotime(unsigned*);

/**
 * @brief Fake device maintainence structure.
 * Since our tasks are periodic, we can represent 
 * tasks with logical devices. 
 * These logical devices should be signalled periodically 
 * so that you can instantiate a new job every time period.
 * Devices are signaled by calling dev_update 
 * on every timer interrupt. In dev_update check if it is 
 * time to create a tasks new job. If so, make the task runnable.
 * There is a wait queue for every device which contains the tcbs of
 * all tasks waiting on the device event to occur.
 */

struct dev
{
	tcb_t* sleep_queue;
	unsigned long   next_match;
};
typedef struct dev dev_t;

/* devices will be periodically signaled at the following frequencies */
const unsigned long dev_freq[NUM_DEVICES] = {100, 200, 500, 50};
static dev_t devices[NUM_DEVICES];

/**
 * @brief Initialize the sleep queues and match values for all devices.
 */
void dev_init(void)
{
	unsigned stamp;
	dotime(&stamp);
	int i;
	for(i = 0; i < NUM_DEVICES; i++){	//for every device, set the first timeout for the first period
		devices[i].next_match = (unsigned long)stamp + dev_freq[i];
	}	
}


/**
 * @brief Puts a task to sleep on the sleep queue until the next
 * event is signalled for the device.
 *
 * @param dev  Device number.
 */
int dev_wait(unsigned int dev)
{
	tcb_t* ctcb = get_cur_tcb();
	if(ctcb->holds_lock){
		return -EHOLDSLOCK;
	}
	runqueue_remove(ctcb->cur_prio);
	tcb_t* temp = devices[dev].sleep_queue;
	if(temp == NULL){
		devices[dev].sleep_queue = ctcb;
	}else{
		while(temp->sleep_queue != NULL){
			temp = temp->sleep_queue;
		}
		temp->sleep_queue = ctcb;
	}
	ctcb->sleep_queue = NULL;
	return 1;
}


/**
 * @brief Signals the occurrence of an event on all applicable devices. 
 * This function should be called on timer interrupts to determine that 
 * the interrupt corresponds to the event frequency of a device. If the 
 * interrupt corresponded to the interrupt frequency of a device, this 
 * function should ensure that the task is made ready to run 
 *
 * @param millis  milliseconds since start
 */
void dev_update(unsigned long millis)
{
	int i;
	tcb_t* current;
	for(i=0; i < NUM_DEVICES; i++){
		//if device has match, activate all tasks on sleep queue
		if(devices[i].next_match <= millis){
			current = devices[i].sleep_queue;
			while(current != NULL){
				runqueue_add(current, current->cur_prio);
				current = current->sleep_queue;
			}
			devices[i].next_match += dev_freq[i];
			devices[i].sleep_queue = NULL;
		}
	}
}


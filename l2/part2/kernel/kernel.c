/*
 * kernel.c: Kernel main (entry) function
 *
 * Author: Jeff Brandon <jdbrando@andrew.cmu.edu> 
 * Date: Sun Oct 12 19:14:20 UTC 2014
 */

/* Define constants in this region */
#define SWI_ADDR 0x5c0009c0
#define SDRAM_BASE 0xa0000000
#define SDRAM_LIMIT 0xa3ffffff
#define SFROM_BASE 0x0
#define SFROM_LIMIT 0xffffff 
#define EOT 4
#define BACK_SPACE 8
#define DELETE 127
#define NEW_LINE 10
#define RETURN 13

/* include necessary header files */
#include <bits/types.h>
#include <bits/errno.h>
#include <bits/fileno.h>
#include <bits/swi.h>
#include <exports.h>

/* declare functions */

/* swi_handler - Assembly function that performs preliminary tasks for swi 
   handling before calling a c swi handler.
*/
extern void swi_handler(void*);

/* user_mode - Assembly function that switches processor state from service
   mode to user mode, then begins execution of a user program. 
	Parameters:
	argc - int: provided to kernel main and passed to user program main
	argv - char*[]: similar to argc, passed to user program main. 
	lr_ptr - unsigned*: a pointer to a variable where the kernel value
				of lr will be saved.
	sp_ptr - unsigned*: a pointer to a variable where the kernel value
				of sp will be saved.
	Returns: exit status of the user program.
*/
extern int user_mode(int, char*[], unsigned*, unsigned*);

/* exit_user - performs steps to exit user mode
	Parameters:
	kernel_lr - value of kernel link register
	kernel_sp - value of kernel stack pointer
	exit_status - exit status of user program
*/
extern void exit_user(unsigned, unsigned, unsigned);

inline void install_handler(int, interrupt_handler_t, unsigned*);

/* global variables */
unsigned instr1;	//first instruction we clobber
unsigned instr2;	//second instruction we clobber
unsigned lr_k; 		//store value of kernel link register  
unsigned sp_k;		//store value of kernel stack pointer

/* restore_old_swi - Restores the instructions at the 
   uboot swi handler that are clobbered when this kernel
   installs its own swi handler.
*/
void restore_old_swi(){
	unsigned* restore = (unsigned*) SWI_ADDR;
	*restore++ = instr1;
	*restore = instr2;
}

/* main - installs custom swi handler, executes a user program, restores
   the default swi handler, and returns the exit status of the user program. 
*/
int main(int argc, char *argv[]) {
	int ret;
	install_handler(SWI_ADDR, swi_handler, (unsigned*)0);
	ret = user_mode(argc, argv, &lr_k, &sp_k);
	restore_old_swi();
	return ret;
}

/* install_handler - Injects a given custom handler at a specified location.
   Specifically, at the given address an instruction is written the causes
   execution to jump to the custom handler.
	Parameters:
	vec - The address of memory where the injected instructions 
		should be placed.
	handler - the address of the custom swi handler.
	arg - pointer to arguments for the hander (currently unused)
*/
inline void install_handler(int vec, interrupt_handler_t handler, unsigned *arg){
	unsigned *addr;
	addr = (unsigned*) vec;
	instr1 = *addr;
	*addr = 0xe51ff004; // ldr pc, [pc, #-4]
	addr++;
	instr2 = *addr;
	*addr = (unsigned) handler;
}

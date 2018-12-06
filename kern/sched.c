
#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(void)
{	
	struct Env *idle;
	int idx;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.
	// LAB 4: Your code here.
	if(!curenv)
	{
		idle =  &envs[0];
		idx = 0;
	}
	else{
		idle = curenv;
		idx = (curenv - envs) + 1;
	}

	int iterations = 0;
	//cprintf("Empiezo la iteracion.\n");
	while(iterations < (NENV-1) )
	{	// tengo otro proceso runnable? si es así, lo uso
		
		// cprintf("iteracion %d, indice %d\n", iterations, idx);
		if(envs[idx].env_status == ENV_RUNNABLE)
		{
			//if(curenv != NULL) {
				//cprintf("Set al curenv environment en Runnable. Esto implica que voy a correr otro\n");
				//curenv->env_status = ENV_RUNNABLE;
			//}
			//cprintf("Env run, iteracion %d\n", iterations);
			env_run(&envs[idx]);
		}

		iterations++;
		idx++;
		if(idx == NENV) {
			//cprintf("reseteo el idx.\n");
			idx = 0;
		}
	}
	 // sino le vuelvo a dar el time slice al que estaba corriendo
	if (curenv && curenv->env_status == ENV_RUNNING)
    	env_run(curenv);

	// else, si no esta running y no tengo ninguno runnable, entonces voy a sched_halt
	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}

// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	int perm = uvpt[PGNUM(addr)];

	cprintf("Pasé por acá con dir %x y perm %x \n", addr, perm);

	if( (!(err & FEC_PR)) || (!(err & FEC_WR)) || (!(perm & PTE_COW)) )
		panic("Pgfault accedida en condiciones anómalas\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	addr = ROUNDDOWN(addr, PGSIZE);
	if( (r = sys_page_alloc(0, PFTEMP, PTE_W|PTE_U|PTE_P)) <0) 
		panic("Pgfault allocating error\n");
	memmove(PFTEMP, addr, PGSIZE);

	if( (r = sys_page_map(0, PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P)) < 0 )
		panic("Pgfault mapping error \n");
	if( (r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("Pgfault unmapping error\n");

}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	// LAB 4: Your code here.
	//panic("duppage not implemented");
	//return 0;

	void* va = (void*) (pn * PGSIZE);

	int perm = (uvpt[pn] & PTE_SYSCALL);

	if(perm & PTE_W || perm & PTE_COW)
	{
		perm = (perm & (~PTE_W)) | PTE_COW;
		if( (r = sys_page_map(0, va, envid, va, perm)) < 0)
			panic("sys_page_map: %e", r);

		if( (r = sys_page_map(0, va, 0, va, perm)) < 0)

			panic("sys_page_map: %e", r);
	}
	else
	{
		if( (r = sys_page_map(0, va, envid, va, perm)) < 0)
			panic("sys_page_map: %e", r);
	}

	
	return 0;
}

static void
dup_or_share(envid_t dstenv, void *va, int perm)
{
	int r;

	if( perm & PTE_W )
	{
		if ((r = sys_page_alloc(dstenv, va, perm)) < 0)
		panic("sys_page_alloc: %e", r);
	}
	if ((r = sys_page_map(dstenv, va, 0, UTEMP, perm)) < 0)
		panic("sys_page_map: %e", r);
	memmove(UTEMP, va, PGSIZE);
	if ((r = sys_page_unmap(0, UTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
	
}


envid_t fork_v0(void)
{
	envid_t envid;

	envid = sys_exofork();
	if(envid < 0)
	{
		panic("sys_exofork: %e", envid);
	}
	if(envid == 0) //Hijo
	{

		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	//Padre
	uint32_t va;
	for(va = 0; va < UTOP; va += PGSIZE)
	{
		if( (uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_U) )
			dup_or_share(envid, (void*) va, uvpt[PGNUM(va)] & PTE_SYSCALL);
	}

	// Start the child environment running
	if( sys_env_set_status(envid, ENV_RUNNABLE) < 0) 
		panic("sys_env_set_status");
	
	return envid;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	//return fork_v0();
	// panic("fork not implemented");

	envid_t envid;
	set_pgfault_handler( (void*) pgfault);

	envid = sys_exofork();
	if(envid < 0)
		panic("sys_exofork: %e", envid);

	if(envid == 0) {  //Hijo 
		cprintf("Estoy en el hije\n");
		thisenv = &envs[ENVX(sys_getenvid())];


		return 0;
	}

	uint32_t va;
	uint32_t PTE_P_U = PTE_P | PTE_U;
	for(va = 0; va < UTOP; va += PGSIZE)
	{
		if ( (UXSTACKTOP-PGSIZE)<=va && va<=(UXSTACKTOP-1) )
			continue;		// el stack de excepciones no se mapea.


		if( (uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & (PTE_P_U)) )
			duppage(envid, PGNUM(va));

		//cprintf("Otra iteración %x \n", va);
	}

	if( sys_page_alloc(envid, (void*) UXSTACKTOP - PGSIZE, PTE_U|PTE_W|PTE_P) < 0)
			panic("Falló sys_page_alloc en set_pgfault_handler");

	extern void _pgfault_upcall();
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);

	// Start the child environment running
	if( sys_env_set_status(envid, ENV_RUNNABLE) < 0) 
		panic("sys_env_set_status");


	return envid;
}


// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

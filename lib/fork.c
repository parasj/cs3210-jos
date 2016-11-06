// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW         0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
  void *addr = (void*)utf->utf_fault_va;
  uint32_t err = utf->utf_err;
  int r;

  // Check that the faulting access was (1) a write, and (2) to a
  // copy-on-write page.  If not, panic.
  // Hint:
  //   Use the read-only page table mappings at uvpt
  //   (see <inc/memlayout.h>).

  // LAB 4: Your code here.

  // Allocate a new page, map it at a temporary location (PFTEMP),
  // copy the data from the old page to the new page, then move the new
  // page to the old page's address.
  // Hint:
  //   You should make three system calls.

  // LAB 4: Your code here.

  panic("pgfault not implemented");
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
  panic("duppage not implemented");
  return 0;
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
  // The parent installs pgfault() as the C-level page fault handler, using the set_pgfault_handler() function you implemented above.
  // The parent calls sys_exofork() to create a child environment.
  // For each writable or copy-on-write page in its address space below UTOP, the parent calls duppage, which should map the page copy-on-write into the address space of the child and then remap the page copy-on-write in its own address space. duppage sets both PTEs so that the page is not writeable, and to contain PTE_COW in the “avail” field to distinguish copy-on-write pages from genuine read-only pages.
  // The exception stack is not remapped this way, however. Instead you need to allocate a fresh page in the child for the exception stack. Since the page fault handler will be doing the actual copying and the page fault handler runs on the exception stack, the exception stack cannot be made copy-on-write: who would copy it?
  // fork() also needs to handle pages that are present, but not writable or copy-on-write.
  // The parent sets the user page fault entrypoint for the child to look like its own.
  // The child is now ready to run, so the parent marks it runnable.

  // 1. install pgfault
  set_pgfault_handler(pgfault);

  // 2. call exofork
  envid_t e = sys_exofork();
  if (e < 0)
    panic("fork: sys_exofork err %e", e);
  if (e == 0) {
    // child
    thisenv = &envs[ENVX(sys_getenvid())];
    return 0;
  }

  // 3. duppage for each writable or copy-on-write page
  for (uintptr_t va = 0; va < USTACKTOP; va += PGSIZE)
    if ((uvpd[PDX(va)] & (PTE_P | PTE_U)) == (PTE_P | PTE_U) && (uvpt[PGNUM(va)] & (PTE_P | PTE_U)) == (PTE_P | PTE_U))
      duppage(e, PGNUM(va));

  // 4. allocate exception stack
  int rval = sys_page_alloc(e, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W);
  if (rval < 0)
    panic("fork: sys_page_alloc err %e", rval);

  // 5. set user page fault entrypoint
  rval = sys_env_set_pgfault_upcall(e, thisenv->env_pgfault_upcall);
  if (rval < 0)
    panic("fork: sys_env_set_pgfault_upcall err %e", rval);

  // 6. Set runnable
  rval = sys_env_set_status(e, ENV_RUNNABLE);
  if (rval < 0)
    panic("fork: sys_env_set_status err %e", rval);

  return 0;
}

// Challenge!
int
sfork(void)
{
  panic("sfork not implemented");
  return -E_INVAL;
}

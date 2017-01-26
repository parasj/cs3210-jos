#include <inc/lib.h>

// Waits until 'envid' exits.
void
wait(envid_t envid)
{
  const volatile struct Env *e;

  assert(envid != 0);
  e = &envs[ENVX(envid)];
  while (e->env_id == envid && e->env_status != ENV_FREE);
}

void
interruptible_wait(envid_t envid)
{
  sys_ccheckc(3); // move cursor to now
  const volatile struct Env *e;

  assert(envid != 0);
  e = &envs[ENVX(envid)];
  while (e->env_id == envid && e->env_status != ENV_FREE) {
     int c = sys_ccheckc(3);
     if (c) {
     	while (sys_cgetc() != 3); // move cursor until ctrl-c
     	cprintf("\nCTRL-C interrupt\n");
     	sys_env_destroy(envid);
     	return;
     }
     sys_yield();
   }
}
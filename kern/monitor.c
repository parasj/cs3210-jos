// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE 80 // enough for one VGA text line


struct Command {
  const char *name;
  const char *desc;
  // return -1 to force monitor to exit
  int (*func)(int argc, char **argv, struct Trapframe * tf);
};

static struct Command commands[] = {
  { "help",      "Display this list of commands",        mon_help       },
  { "info-kern", "Display information about the kernel", mon_infokern   },
  { "backtrace", "Display backtrace", mon_backtrace   },
  { "showmappings", "Show the VA -> PA mappings" , mon_showmappings },
  { "pageperm", "Set permission bit for a page" , mon_pageperm },
  { "dumpvirt", "Dump a range of virtual memory addresses" , mon_dumpvirt },
  { "dumpphys", "Dump a range of physical memory addresses" , mon_dumpphys },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
  int i;

  for (i = 0; i < NCOMMANDS; i++)
    cprintf("%s - %s\n", commands[i].name, commands[i].desc);
  return 0;
}

int
mon_infokern(int argc, char **argv, struct Trapframe *tf)
{
  extern char _start[], entry[], etext[], edata[], end[];

  cprintf("Special kernel symbols:\n");
  cprintf("  _start                  %08x (phys)\n", _start);
  cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
  cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
  cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
  cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
  cprintf("Kernel executable memory footprint: %dKB\n",
          ROUNDUP(end - entry, 1024) / 1024);
  return 0;
}


int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
  // Calling convention:
  // 0 saved ebp <----- read_ebp
  // 1 return addr (eip)
  // 2 arg0
  // 3 arg1
  // 4 arg2
  // 5 arg3
  // 6 arg4
  
  cprintf("Stack backtrace:\n");

  // read ebp
  uint32_t *ebp = (uint32_t*) read_ebp();

  // info struct (allocated on stack)
  struct Eipdebuginfo info;

  // while the ebp is not zero (movl  $0x0,%ebp sets base of stack)
  while (ebp != NULL) {
    // read in struct from symbol table
    debuginfo_eip(ebp[1], &info);
    cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", ebp, ebp[1], ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);
    cprintf("    %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, (int) (ebp[1] - info.eip_fn_addr));
    
    // dereference ebp to retrieve saved base stack pointer
    ebp = (uint32_t*) *ebp;
  }
  
  return 0;
}


int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{ 
  if (argc != 3) {
    cprintf("help: %s 0x[start] 0x[end]\n", argv[0]);
  } else {
    // start and end virtual addresses
    uintptr_t sva = (uint32_t)strtol(argv[1], NULL, 0);
    uintptr_t eva = (uint32_t)strtol(argv[2], NULL, 0);

    // round to nearest page boundaries
    uintptr_t begin = ROUNDDOWN(sva, PGSIZE);
    uintptr_t end = ROUNDUP(eva, PGSIZE);

    cprintf("Mappings for [0x%08x, 0x%08x], pages 0x%08x to 0x%08x\n", sva, eva, begin, end);

    while (begin <= end) {
      pte_t *pte = pgdir_walk(kern_pgdir, (void *) begin, 1);

      if (!pte) panic("mon_showmappings: Page table entry for 0x%08x resulted in failed allocation in pgdir_walk", begin);
      if (!(*pte & PTE_P)) panic("mon_showmappings: Page table entry for 0x%08x not present", begin);

      cprintf("  %08x -> %08x\t", begin, PTE_ADDR(*pte));
      
      if (*pte & PTE_U) cprintf(" USER");
      else cprintf(" KERN");

      if (*pte & PTE_W) cprintf(" READ WRITE");
      else cprintf(" READ      ");

      if (*pte & PTE_P) cprintf(" PRESENT");
      else cprintf(" NOT_PRESENT");

      cprintf("\n");

      begin += PGSIZE;
    }
  }

  return 0;
}

int
mon_pageperm(int argc, char **argv, struct Trapframe *tf) {
  if (argc != 4 ) {
    cprintf("help: %s 0x[address] [P|W|U] [0|1]\n", argv[0]);
  } else {
    // read in from terminal
    uintptr_t addr = (uint32_t)strtol(argv[1], NULL, 0);
    char *mode = argv[2];
    char *bit = argv[3];

    uint32_t mask = 0;
    if (*mode == 'P') {
      mask = PTE_P;
    } else if (*mode == 'W') {
      mask = PTE_W;
    } else if (*mode == 'U') {
      mask = PTE_U;
    }

    cprintf("Setting permission bit for 0x%08x[%c %d] = %c\n", addr, *mode, mask, *bit);


    pte_t *pte = pgdir_walk(kern_pgdir, (void *) addr, 1);

    if (*bit == '0') {
      *pte = *pte & ~mask;
    } else if (*bit == '1') {
      *pte = *pte | mask;
    }
  }

  return 0;
}


int
mon_dumpvirt(int argc, char **argv, struct Trapframe *tf) {
  if (argc != 3) {
    cprintf("help: %s 0x[start] 0x[end]\n", argv[0]);
  } else {
    uintptr_t sva = (uint32_t)strtol(argv[1], NULL, 0);
    uintptr_t eva = (uint32_t)strtol(argv[2], NULL, 0);

    cprintf("Dumping virtual addresses [0x%08x, 0x%08x]\n", sva, eva);

    for (uintptr_t i = sva; i <= eva; i += sizeof(uint32_t)) {
      cprintf("  0x%08x = 0x%08x\n", i, *((uint32_t *) i));
    }
  }

  return 0;
}

int
mon_dumpphys(int argc, char **argv, struct Trapframe *tf) {
  if (argc != 3) {
    cprintf("help: %s 0x[start] 0x[end]\n", argv[0]);
  } else {
    physaddr_t spa = (uint32_t)strtol(argv[1], NULL, 0);
    physaddr_t epa = (uint32_t)strtol(argv[2], NULL, 0);

    uintptr_t sva = (uintptr_t) KADDR(spa);
    uintptr_t eva = (uintptr_t) KADDR(epa);

    cprintf("Dumping physical addresses [0x%08x, 0x%08x] -> [0x%08x, 0x%08x]\n", spa, epa, sva, eva);

    for (uintptr_t i = sva; i <= eva; i += sizeof(uint32_t)) {
      cprintf("  0x%08x (0x%08x) = 0x%08x\n", PADDR((void *) i), i, *((uint32_t *) i));
    }
  }

  return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
  int argc;
  char *argv[MAXARGS];
  int i;

  // Parse the command buffer into whitespace-separated arguments
  argc = 0;
  argv[argc] = 0;
  while (1) {
    // gobble whitespace
    while (*buf && strchr(WHITESPACE, *buf))
      *buf++ = 0;
    if (*buf == 0)
      break;

    // save and scan past next arg
    if (argc == MAXARGS-1) {
      cprintf("Too many arguments (max %d)\n", MAXARGS);
      return 0;
    }
    argv[argc++] = buf;
    while (*buf && !strchr(WHITESPACE, *buf))
      buf++;
  }
  argv[argc] = 0;

  // Lookup and invoke the command
  if (argc == 0)
    return 0;
  for (i = 0; i < NCOMMANDS; i++)
    if (strcmp(argv[0], commands[i].name) == 0)
      return commands[i].func(argc, argv, tf);
  cprintf("Unknown command '%s'\n", argv[0]);
  return 0;
}

void
monitor(struct Trapframe *tf)
{
  char *buf;

  cprintf("Welcome to the JOS kernel monitor!\n");
  cprintf("Type 'help' for a list of commands.\n");


  while (1) {
    buf = readline("K> ");
    if (buf != NULL)
      if (runcmd(buf, tf) < 0)
        break;
  }
}

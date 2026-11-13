"""Exercise the production prefetch stack startup on a host C compiler.

    py tools/test_prefetch_stack.py --cc gcc
"""
import argparse
import re
import shlex
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Nintendont/kernel/slippi_online/so_prefetch.c'

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cc', default='gcc')
    args = ap.parse_args()
    source = SOURCE.read_text()
    start = source.index('void so_pf_init(void)')
    end = source.index('\nvoid so_pf_close(void)', start)
    function = source[start:end]
    constants = '\n'.join(re.search(r'^#define ' + name + r'\s+\S+', source, re.M).group()
                          for name in ('PF_STACK_SIZE', 'PF_PRIORITY'))
    stub = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
typedef unsigned char u8;
typedef unsigned int u32;
static u8 *stack;
static u8 space[0x2000] __attribute__((aligned(32)));
static int fail_alloc, fail_thread, allocations, frees, threads, logs;
static u32 pf_thread(void *arg) { (void)arg; return 0; }
static void *so_malloc(u32 n) {
    assert(n == sizeof(space)); allocations++;
    return fail_alloc ? NULL : space;
}
static void so_free(void *p) { assert(p == space); frees++; }
static void so_log(const char *s) { assert(s); logs++; }
static int so_thread_start(u32 (*entry)(void *), void *arg, void *top, u32 n, u32 priority) {
    assert(entry == pf_thread && !arg && top == space + sizeof(space));
    assert(n == sizeof(space) && priority == PF_PRIORITY && !((uintptr_t)space & 31));
    threads++; return fail_thread ? -1 : 0;
}
'''
    test = r'''
int main(void) {
    fail_alloc = 1; so_pf_init();
    assert(allocations == 1 && threads == 0 && frees == 0 && !stack);
    fail_alloc = 0; fail_thread = 1; so_pf_init();
    assert(allocations == 2 && threads == 1 && frees == 1 && !stack);
    fail_thread = 0; so_pf_init();
    assert(allocations == 3 && threads == 2 && frees == 1 && stack == space);
    so_pf_init();
    assert(allocations == 3 && threads == 2 && frees == 1 && logs == 2);
    puts("PASS prefetch stack: allocation failure/retry, thread failure/free/retry, alignment, size, lifetime and repeated init");
    return 0;
}
'''
    with tempfile.TemporaryDirectory() as tmp:
        c, exe = Path(tmp) / 'prefetch.c', Path(tmp) / 'prefetch.exe'
        c.write_text(constants + stub + function + test)
        subprocess.run([*shlex.split(args.cc), str(c), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)

if __name__ == '__main__':
    main()

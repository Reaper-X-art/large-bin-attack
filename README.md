Testing the Large Bin Attack Exploit in C
Overview
This guide explains how to test the Large Bin Attack exploitation demo project in C for Linux (glibc-based systems like Ubuntu 22.04). The project includes a vulnerable program (vuln.c) with an off-by-one heap overflow and an exploit PoC (exploit.c) that grooms large bins, corrupts pointers, and hijacks __free_hook to execute system("/bin/sh"). Testing is for educational purposes only—run in an isolated virtual machine (VM) to avoid risks. Disable ASLR for predictability, as the code uses placeholder addresses.
Prerequisites:

Ubuntu 22.04 LTS VM (or similar with glibc 2.35).
Installed tools: gcc, gdb (install via sudo apt update && sudo apt install gcc gdb).
Compiled files: Use gcc vuln.c -o vuln -no-pie -g and gcc exploit.c -o exploit -g.
Ethical reminder: This simulates a vulnerability; do not use on real systems.

Step 1: Disable ASLR and Setup
ASLR randomizes addresses, making exploits unpredictable. Disable it for the vulnerable process:

Run the vulnerable program with: setarch $(uname -m) -R ./vuln.
This command disables ASLR only for that session. For persistent testing, echo 0 to /proc/sys/kernel/randomize_va_space (requires root; revert after).

Verify setup:

Compile both files as above.
Ensure glibc version matches: ldd --version should show ~2.35. If not, adjust offsets or use a compatible VM.

Step 2: Find Real Offsets Using GDB
The code uses placeholders for __free_hook, system, and libc base. These vary by system—use GDB to find them:

Start GDB on the vulnerable program: gdb ./vuln.
Set a breakpoint: break main (or b main).
Run the program: run (or r).
Find libc base (with ASLR off via setarch): Use info proc mappings—look for the start address of libc.so.6 (e.g., 0x7ffff7c00000). Update LIBC_BASE in exploit.c.
Find __free_hook offset: print &__free_hook—subtract libc base to get the offset (e.g., if address is 0x7ffff7ffeb28, offset = 0x7ffff7ffeb28 - LIBC_BASE). Update FREE_HOOK_OFFSET in exploit.c.
Find system offset: print &system—subtract libc base. Update SYSTEM_OFFSET.
Recompile exploit.c after updates: gcc exploit.c -o exploit -g.

Tip: If __free_hook is not available (deprecated in newer glibc), adapt to target __exit_funcs or use one_gadget tool (install via Ruby: gem install one_gadget, then one_gadget /lib/x86_64-linux-gnu/libc.so.6 for alternative ROP gadgets).
Step 3: Test Manually (Interactive Mode)
Run the vulnerable program and simulate the exploit steps manually to verify the vulnerability:

Start vuln: setarch $(uname -m) -R ./vuln.
Follow grooming steps (matching exploit.c):

Choose 1 (Alloc), index 0, size 1280 (0x500 in decimal).
Alloc index 1, size 1536 (0x600).
Alloc index 2, size 1792 (0x700).
Alloc index 3, size 1280 (0x500) — this is the victim chunk.
Choose 3 (Free), free indices 0, 1, 2 (populates large bins).


Overflow via edit: Choose 2 (Edit), index 3. Input a long string: 1280 'A's + forged bk data. Use Python to generate: python3 -c "print('A'*1280 + '\x00'*24 + '\xXX\xXX...' )" (replace XX with little-endian bytes of (LIBC_BASE + FREE_HOOK_OFFSET) - 0x20).
Trigger the attack: Alloc a new chunk (size 2048/0x800), then free it. This inserts into large bins, triggering partial unlink and corrupting __free_hook.
Overwrite and trigger: Alloc a small chunk (size 256), edit to "/bin/sh", free it. If successful, a shell spawns (e.g., /bin/sh prompt).

Expected Outcome: If offsets are correct, you'll get a shell. Otherwise, segfault—debug next.
Step 4: Automate with the Exploit
The exploit.c grooms and overflows conceptually but needs integration for full automation (it doesn't directly interact with vuln). For testing:

Modify vuln.c to accept batch inputs if needed (e.g., read from stdin in a loop).
Run exploit and pipe if possible: ./exploit | setarch $(uname -m) -R ./vuln (may require vuln adjustments for non-interactive mode).
Alternatively, run vuln in one terminal, input exploit-generated payload manually during edit.
Success: After running, check for shell spawn. Use ps aux | grep sh to confirm.

Note: If piping fails, use tools like expect script or modify exploit to use popen("./vuln", "w") and fprintf menu choices/payloads.
Step 5: Debugging Failures
Use GDB for in-depth analysis:

Run: gdb --args setarch $(uname -m) -R ./vuln.
Install GEF (GDB plugin for heap analysis): wget -q -O- https://github.com/hugsy/gef/raw/master/scripts/gef.sh | sh.
In GDB: gef config context.layout "legend code source stack threads trace".
Set breakpoints: break free, break malloc.
Run and step through: Use heap bins to inspect large bins (should show corrupted bk after overflow).
Watch variables: watch &__free_hook to see overwrite.
Common issues & fixes:

Segfault: Wrong offsets—recheck with GDB.
No corruption: Adjust payload offset (0x500 + 0x18 may vary by alignment).
Tcache interference: Use even larger sizes (>0x1000) to force large bins.
glibc mismatch: If __free_hook fails, target __malloc_hook or use FSOP (File Stream Oriented Programming) for stdout hijack.



Troubleshooting and Extensions

No Shell? Verify with p *__free_hook in GDB—it should equal system address after trigger.
glibc 2.35+ Issues: __free_hook is deprecated; extend to overwrite __exit_funcs (find offset similarly) or use one_gadget for single-call shell.
Add Leak Stage: For real ASLR, add unsorted bin leak: Free large chunk, read its bk (contains libc base), compute dynamically in exploit.
Resources: Refer to how2heap GitHub for similar examples; CTF writeups on large bin attacks.

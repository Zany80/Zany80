# Zany80
## Bank mapping

### Situation

4 banks can be mapped at a time, each one is 16kb.

If a function on one banks calls a function on another bank, a simple `ret` will
no longer suffice. There needs to be a way to call functions between banks that
doesn't take significant overhead and is as transparent as possible.

### Potential solutions

#### Solution 1

In order to ensure code banking works, need specific addresses, and need to have
a way of keeping track of calls. If a function on bank 1 calls a function on
bank 3 which calls a separate function on bank 2, need them to return to the
correct banks.

How?

replace nops. In other words, whenever a `nop` is read as an opcode
from RAM, instead of running a `nop`, run a `pop_bank` command, which uses a
special stack to keep track of what banks to return to.

Pros:

* Extremely simple to implement.
* Minimal overhead

Cons:

* Would remove the ability to execute nops.
	* Among other things, this would make delays more painful to implement.

#### Solution 2

The first solution operates under the assumption that only one bank of code can
be mapped at once. Solution 2 ignores that assumption completely. Instead of
keeping track of calls, have up to three code banks in use at once.

E.g. allow any type of bank to be mapped to virtual banks 0 - 3, and reserve
bank 4 for the stack.

Could then also have the MMU use interrupts to swap stack banks when necessary.

#### Solution 3

A bit of a combination of 1 and 2.

Have banks 0 and 1 be code banks, bank 2 be a data bank, and 3 be the stack.

However, this one also ignores an assumption the other two make: that sound,
graphics, etc need all be on the data bank.

Instead, solution 3 proposes that the CPU tells the GPU what data banks to use,
and the GPU accesses the memory directly. Same for sound and anything else that
might pop up. The CPU's data bank will then be for CPU data alone.

This is not, of course, perfect. Only two code banks can be mapped at once in
this solution. This means that if code on user-provided bank 0 calls code on
user-provided bank 1 which calls a libc-bank function, bank 1 is in charge of
calling setcode0() before returning.

This is the solution that will be implemented for now, but it all raises up a
*new* problem.

### The new problem

Mapping a code bank is done via the provided setcode`x` function, where x ranges
from 0-7 (8 user code banks can be used for a total of 8 * 16 = 128kB). There's
just one tiny problem with this.

Namely, how does the function know where to map the bank to? Should it map it to
virtual bank 0 or 1?

#### Solution the only solution

There are only two real solutions to this. One, each banks specifies where to be
mapped to, which is feasible but useless. Two, even banks are mapped to 0 and
odd banks to 1 (or some such system). I'm going with two. That means that e.g.
bank 0 can only call functions from banks 0, 1, 3, 5, 7, and the BIOS.

*This is actually where I realized that the libc was no longer a libc, but a
full-fledged BIOS. It is inaccessible to all other code, can only be called
indirectly (it has its own physical bank inside the console), and it's used the
same way x86 PC BIOS functions are called (except instead of a SW INT, it uses a
transparent I/O call).*

Each bank must also be built separately, though Zany will have tools to make
inter-bank calls easier.

This still has one problem though. What if 0 calls 3 which calls 4? How does the
program return properly?

That's where it gets a bit tricky.

Option 1: automatically have the setcode`x` functions auto-restore the correct
bank after the function returns. Tricky, might be doable. Going to try this, if
it fails I'll look for a second option.

*This project has been created as part of the 42 curriculum by bkhilo.*

# Codexion

## Description

Codexion is a multithreaded simulation written in C, a variant of the
dining-philosophers problem. `N` coders sit around a table with `N` USB
dongles between them. A coder cycles through three phases: **compiling**
(which needs the two dongles next to them), **debugging** and
**refactoring**. A coder who does not start compiling within
`time_to_burnout` ms of their last compile start **burns out** and the
simulation ends. It also ends once every coder has compiled
`number_of_compiles_required` times.

Compared to the classic problem, three rules make scheduling harder:

- **Cooldown** — a released dongle is unusable for `dongle_cooldown` ms.
- **Arbitration** — when several coders want the same dongle it is granted
  by policy: `fifo` (request order) or `edf` (earliest burnout deadline
  first), with a deterministic tie-break.
- **A separate monitor thread** must report a burnout within 10 ms.

The goal is a program that never deadlocks, never lets a coder starve when
the parameters allow it, prints clean serialized logs, and frees everything.

## Instructions

```sh
make            # builds ./codexion with cc -Wall -Wextra -Werror -pthread
make clean      # removes object files
make fclean     # also removes the binary
make re         # full rebuild
```

```
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

| Argument | Meaning |
|---|---|
| `number_of_coders` | coders and dongles (≥ 1) |
| `time_to_burnout` | ms since last compile start before a coder burns out |
| `time_to_compile` | ms spent compiling (holding two dongles) |
| `time_to_debug` | ms spent debugging |
| `time_to_refactor` | ms spent refactoring |
| `number_of_compiles_required` | stop once every coder compiled this many times (`0` stops immediately) |
| `dongle_cooldown` | ms a released dongle stays unavailable |
| `scheduler` | exactly `fifo` or `edf` |

All numeric arguments must be non-negative integers that fit in an `int`;
anything else (negative, empty, `12a`, `1.5`, an unknown scheduler) is
rejected with an error on `stderr` and exit status 1.

Examples:

```sh
./codexion 5 800 200 200 200 5 0 edf     # 5 coders, everyone compiles 5 times, no burnout
./codexion 1 800 200 200 200 5 0 fifo    # one coder, one dongle: burns out at 800 ms
./codexion 4 310 200 100 100 5 0 edf     # infeasible: a burnout is reported at 310 ms
./codexion 5 1000 200 200 200 5 100 fifo # with a 100 ms cooldown on every dongle
```

Log format (one line per state change, timestamps in ms since start):

```
0 1 has taken a dongle
0 1 has taken a dongle
0 1 is compiling
200 1 is debugging
400 1 is refactoring
...
800 4 burned out
```

## Project layout

```
includes/codexion.h   structs, constants, prototypes
src/main.c            entry point
src/parse_args.c      strict argument validation
src/init.c            allocation and initialization of coders, dongles, sync primitives
src/threads.c         thread creation, start gate, joining, error path
src/cleanup.c         destruction of every mutex/cond and free of every allocation
src/time.c            microsecond clock, precise sleep, timespec helper
src/log.c             serialized logging and the shared stop flag
src/heap.c, heap_utils.c   hand-written binary min-heap (the priority queue)
src/dongle.c          per-dongle state operations (all under the dongle mutex)
src/acquire.c         arbitration: FIFO/EDF keys, priority, take-both-or-none
src/event.c           per-coder event (mutex + cond + flag) and notifications
src/coder.c           the coder thread routine
src/monitor.c         the monitor thread routine
```

## Technical choices

- **Time base**: `gettimeofday()` converted to microseconds. Logs print
  milliseconds relative to a common start. All coder threads block on a
  *start gate* (mutex + condition variable) so they share the same `t = 0`
  and the same initial deadline.
- **Precise sleeping**: `smart_sleep()` sleeps in 500 µs steps and re-checks
  the clock, so a 200 ms phase never overshoots by more than a fraction of
  a millisecond; it also returns early when the simulation stops.
- **Priority queue**: each dongle owns a binary min-heap of requests
  (`t_request { coder_id, key }`). For `fifo` the key is the request's
  arrival time; for `edf` it is `last_compile_start + time_to_burnout`.
  Equal keys are broken by the lowest coder id, which makes both policies
  fully deterministic. No library queue is used.
- **Burnout timestamp**: the monitor polls every 500 µs, and prints the
  time at which it *detected* the burnout (≤ 1 ms after the real deadline).
  Every log line therefore uses the same "now" clock and output stays
  monotonic.
- **Single coder**: with one coder there is one dongle. The coder takes it,
  logs it once, can never compile and burns out at `time_to_burnout` —
  the monitor ends the simulation and the thread exits cleanly.

## Blocking cases handled

**Deadlock (Coffman's conditions).** A deadlock needs all four of mutual
exclusion, hold-and-wait, no preemption and circular wait. This program
removes **hold-and-wait**: a coder takes its two dongles *atomically or not
at all* (`acquire.c: try_take`). A coder never sits on one dongle while
waiting for the other, so no chain of partially served coders can form and
a circular wait is impossible. The decision is made under a single
`table_lock`, and state is only ever locked one dongle mutex at a time, so
the lock graph itself is acyclic (see the lock order below).

**Convoy / starvation behind a blocked head.** With strict per-dongle
queues, a queue head that is itself blocked on its *other* dongle would
block everybody behind it, and a ring of equal deadlines degenerates into
one coder compiling at a time (5 coders × 200 ms > 800 ms → burnout even
though the parameters are feasible). Here a head that cannot proceed
right now **yields**: the dongle is granted to the earliest-deadline /
earliest-arrived requester *that can actually use it*. Whenever the
higher-priority coder can go, it still goes first, so EDF keeps its
liveness property: the coder with the earliest deadline is served as soon
as both of its dongles are simultaneously free, and no neighbour may jump
ahead of it at that moment.

**Fair arbitration.** `fifo` orders requests by arrival time, `edf` by
`last_compile_start + time_to_burnout`. Both are total orders thanks to the
coder-id tie-break, so the heap head is always well defined.

**Cooldown.** `dongle_release()` records `available_at = now + cooldown`.
A coder that is head of both queues but blocked only by a cooldown waits on
its event with `pthread_cond_timedwait()` until the later of the two
cooldowns, then retries. Nobody, including the previous holder, can take a
dongle before its cooldown has elapsed.

**Precise burnout detection.** The monitor thread reads every coder's
`last_start` under that coder's mutex and compares it with the current
time every 500 µs. Detection lag is ≤ 1 ms, far inside the 10 ms budget.
`last_start` is updated *before* the "is compiling" line is printed, so
the monitor can never see a stale deadline for a coder that has already
started compiling.

**Log serialization and clean stop.** One mutex (`print_lock`) protects
both `stdout` and the `stopped` flag. The monitor prints "burned out" and
sets `stopped` in the same critical section; every other log call checks
the flag under the same mutex, so nothing is printed after the burnout line
and lines never interleave.

**Lost wake-ups.** Coders wait on a per-coder *event* (mutex + condition
variable + `notified` flag). A notification that arrives while the coder is
between its check and its wait is remembered by the flag, so the coder
never sleeps through a state change. Every take and release notifies the
coders within two positions of the dongle — exactly those whose decision
can change.

**Simulation end.** When the monitor stops the simulation it notifies every
coder; blocked coders wake, see `stopped`, return, and the main thread joins
all threads before destroying primitives and freeing memory. Partial
initialization failures (`malloc`, `pthread_*_init`, `pthread_create`) are
unwound with the same cleanup path.

## Thread synchronization mechanisms

| Primitive | Where | Protects |
|---|---|---|
| `pthread_mutex_t dongle.mutex` (one per dongle) | `dongle.c` | `holder`, `available_at`, the request heap |
| `pthread_mutex_t sim.table_lock` | `acquire.c` | the *decision* to take a pair: two coders can never both conclude that the same free dongle is theirs |
| `pthread_mutex_t coder.lock` + `pthread_cond_t coder.cond` + `int notified` | `event.c` | the coder's event, plus `last_start` and `count` read by the monitor |
| `pthread_mutex_t sim.print_lock` | `log.c` | `stdout` and the `stopped` flag |
| `pthread_mutex_t sim.start_lock` + `pthread_cond_t sim.start_cond` | `threads.c`, `coder.c` | the start gate (`started`, `start_us`) |

**Lock order.** `table_lock` → one `dongle.mutex` at a time. `coder.lock`,
`print_lock` and `start_lock` are always taken alone (never while holding
another lock). Since no thread ever holds two locks except
`table_lock → dongle.mutex`, the wait-for graph between mutexes cannot
contain a cycle.

**Custom event (`event.c`).** `coder_notify()` sets `notified = 1` and
signals; `coder_wait()` loops on `while (!notified)` around
`pthread_cond_wait` / `pthread_cond_timedwait`, then clears the flag. This
turns the condition variable into a level-triggered event: spurious
wake-ups are harmless (the loop re-checks), and early notifications are not
lost (the flag stays set).

**Race conditions prevented — examples.**

- *Two coders taking the same dongle*: `try_take()` reads both dongles'
  state and marks them held while holding `table_lock`; the second coder's
  decision runs strictly after the first one's writes.
- *Monitor reading a half-updated coder*: `last_start` and `count` are
  written and read only under `coder.lock`.
- *A "burned out" line followed by another message*: impossible, because
  the flag that silences coders is set in the same `print_lock` section
  that prints the burnout.
- *Release racing with a waiter*: the releaser updates the dongle under its
  mutex, then notifies; if the waiter had already checked and was about to
  sleep, the `notified` flag makes its wait return immediately.
- *Cooldown granularity*: all times are kept in microseconds; comparisons
  never lose a partial millisecond.

**Coder ↔ monitor communication.** Coders publish their state through
`coder.lock`-protected fields; the monitor publishes the end of the
simulation through the `print_lock`-protected `stopped` flag and wakes
every coder's event. No global variables are used: everything hangs off
one `t_sim` structure passed by pointer.

## Resources

- *The Dining Philosophers Problem* — E. W. Dijkstra, and the summary on
  [Wikipedia](https://en.wikipedia.org/wiki/Dining_philosophers_problem).
- E. G. Coffman, M. Elphick, A. Shoshani — *System Deadlocks* (1971): the
  four necessary conditions for deadlock.
- POSIX threads: `man pthread_mutex_lock`, `man pthread_cond_timedwait`,
  `man pthread_create`; the [Open Group specification](https://pubs.opengroup.org/onlinepubs/9699919799/).
- *Earliest Deadline First* scheduling — Liu & Layland (1973), and
  [Wikipedia](https://en.wikipedia.org/wiki/Earliest_deadline_first_scheduling).
- Binary heaps: CLRS, *Introduction to Algorithms*, chapter 6.
- The 42 Norm and `norminette`: <https://github.com/42school/norminette>.

**Use of AI.** AI was used to help write
this README and the project's unit/integration tests. All generated content
was reviewed, run, and understood before being kept.

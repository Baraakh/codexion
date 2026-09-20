# Codexion — Implementation Plan

Plan for solving the Codexion subject (`docs/codexion.md`): a dining-philosophers
variant in C with pthreads, per-dongle FIFO/EDF arbitration backed by a
hand-written binary heap, dongle cooldowns, and a monitor thread that detects
burnout within 10 ms. Everything must pass `norminette` (v3.3.60 installed
locally) and compile with `cc -Wall -Wextra -Werror -pthread`.

---

## 1. Requirements checklist (extracted from the subject)

| # | Requirement | Where it is handled |
|---|---|---|
| R1 | 8 mandatory args, reject negatives / non-integers / bad scheduler | `parse_args.c` |
| R2 | No global variables | everything hangs off one `t_sim` passed by pointer |
| R3 | One thread per coder (`pthread_create`) | `init.c`, `coder.c` |
| R4 | N dongles for N coders; 1 coder → 1 dongle (left == right) | `init.c` |
| R5 | Each dongle protected by its own `pthread_mutex_t` (+ `pthread_cond_t`) | `dongle.c` |
| R6 | Cooldown: dongle unavailable `dongle_cooldown` ms after release | `dongle.c` (`available_at`) |
| R7 | Arbitration per `scheduler`: `fifo` (arrival order) / `edf` (earliest `last_compile_start + time_to_burnout`, deterministic tie-break) | `heap.c`, `dongle.c` |
| R8 | Custom priority queue (heap) — no library PQ | `heap.c`, `heap_utils.c` |
| R9 | Deadlock-free, starvation-resistant (liveness under `edf` when feasible) | resource ordering + queued arbitration, §3.4 |
| R10 | Separate monitor thread; burnout logged ≤ 10 ms after it happens | `monitor.c` (1 ms polling) |
| R11 | Serialized logging, no interleaving, nothing printed after "burned out" | `log.c` (one mutex guards stdout **and** the `stopped` flag) |
| R12 | Stop on burnout **or** when every coder compiled ≥ `number_of_compiles_required` times | `monitor.c` |
| R13 | No leaks, no crashes, all mutexes/conds destroyed | `cleanup.c` |
| R14 | Makefile: `NAME all clean fclean re`, no relink, `cc` + flags | `Makefile` |
| R15 | Norm-compliant (25 lines/function, 5 functions/file, 80 cols, no `for`/ternary/`switch`, declarations at top, 42 header…) | every file |
| R16 | `README.md` with the required sections (incl. *Blocking cases handled*, *Thread synchronization mechanisms*, AI usage) | Phase 6 |

Allowed external functions only: `pthread_create/join`, `pthread_mutex_*`,
`pthread_cond_*` (incl. `timedwait`, `broadcast`), `gettimeofday`,
`clock_gettime`, `usleep`, `write`, `malloc`, `free`, `printf`, `fprintf`,
`strcmp`, `strlen`, `atoi`, `memset`.

---

## 2. Project layout

```
Codexion/
├── Makefile
├── README.md                     (Phase 6)
├── includes/
│   └── codexion.h                structs, enums, prototypes, constants
├── src/
│   ├── main.c                    main(), run_simulation()
│   ├── parse_args.c              strict integer parsing + validation
│   ├── init.c                    allocate/initialize sim, dongles, coders
│   ├── cleanup.c                 destroy mutexes/conds, free heaps & arrays
│   ├── time.c                    get_time_ms(), elapsed(), smart_sleep(), ms_to_timespec()
│   ├── log.c                     log_state(), log_burnout(), sim_is_stopped()
│   ├── heap.c                    heap_init/push/pop/peek/free
│   ├── heap_utils.c              request_less() comparator, sift_up/down, swap
│   ├── threads.c                 start gate, pthread_create/join, error unwinding
│   ├── dongle.c                  per-dongle state ops, each under the dongle mutex
│   ├── acquire.c                 FIFO/EDF keys, priority + yield rule, take-both-or-none
│   ├── event.c                   per-coder event (mutex + cond + flag), notifications
│   ├── coder.c                   coder_routine() and the compile/debug/refactor steps
│   └── monitor.c                 monitor_routine(), burnout & completion checks
├── tests/                        (not submitted, not graded — for my own use)
│   ├── run_tests.sh
│   └── check_log.py              validates ordering / timing of a captured log
└── docs/
    ├── codexion.md               subject
    └── implementation-plan.md    this file
```

`.gitignore`: `obj/`, `codexion`, `*.o`, `*.dSYM`.

---

## 3. Design

### 3.1 Data structures (`includes/codexion.h`)

```c
typedef enum e_sched { SCHED_FIFO, SCHED_EDF }  t_sched;

typedef struct s_args
{
    int     n_coders;
    long    t_burnout;      /* ms */
    long    t_compile;
    long    t_debug;
    long    t_refactor;
    int     compiles_req;
    long    cooldown;
    t_sched sched;
}   t_args;

typedef struct s_request        /* one entry in a dongle's wait queue */
{
    int     coder_id;           /* 1-based */
    long    key;                /* FIFO: arrival sequence no. | EDF: deadline (ms) */
}   t_request;

typedef struct s_heap           /* binary min-heap of t_request */
{
    t_request   *data;
    int         size;
    int         cap;
    t_sched     mode;           /* only affects the comparator */
}   t_heap;

typedef struct s_dongle
{
    int             id;
    pthread_mutex_t mutex;      /* protects every field below */
    int             holder;     /* coder id, 0 = free */
    long            available_at; /* absolute us; cooldown end */
    t_heap          queue;      /* capacity 2: at most one request per neighbour */
}   t_dongle;

typedef struct s_coder
{
    int             id;
    pthread_t       thread;
    pthread_mutex_t lock;       /* protects the event + last_start + count */
    pthread_cond_t  cond;       /* per-coder event */
    int             notified;
    long            last_start; /* absolute us */
    int             count;
    t_dongle        *first;     /* lower-numbered dongle (taken first) */
    t_dongle        *second;    /* higher-numbered dongle */
    struct s_sim    *sim;
}   t_coder;

typedef struct s_sim
{
    t_args          args;
    long            start_ms;   /* absolute ms of simulation start */
    t_coder         *coders;
    t_dongle        *dongles;
    pthread_t       monitor;
    pthread_mutex_t print_lock; /* guards stdout AND `stopped` */
    int             stopped;
    pthread_mutex_t table_lock; /* serializes take-pair decisions */
    pthread_mutex_t start_lock; /* start gate */
    pthread_cond_t  start_cond;
    int             started;
}   t_sim;
```

Why one mutex for stdout *and* `stopped`: the monitor can atomically
"print burned out, then set stopped" and no coder can slip a message in
between. Every coder log does `lock; if (!stopped) printf; unlock`. That
gives R11 with a single lock and no lock-ordering hazard.

### 3.2 Time (`time.c`)

- `get_time_ms()` → `gettimeofday` converted to ms (`long`).
- `elapsed_ms(sim)` → `get_time_ms() - sim->start_ms` for log timestamps.
- `smart_sleep(sim, ms)`: loop `usleep(200..500)` until target reached **and**
  break early if `sim_is_stopped()`. Avoids `usleep` oversleeping by
  several ms and lets threads exit promptly when the sim ends.
- `ms_to_timespec(abs_ms, &ts)`: absolute `struct timespec` for
  `pthread_cond_timedwait` (default cond clock is `CLOCK_REALTIME`, same
  base as `gettimeofday`, so no `pthread_condattr_setclock` is needed).

### 3.3 Priority queue (`heap.c`, `heap_utils.c`)

Array-based binary min-heap, capacity 2 (at most one pending request per
neighbour of a dongle, so it never grows; no realloc needed).

Comparator `request_less(a, b)`:
- `key` smaller wins (FIFO: earlier arrival time in µs; EDF: earlier
  deadline).
- tie → lower `coder_id` wins: the required deterministic tie-break, and
  it makes both orders total.

Ops: `heap_init`, `heap_push`, `heap_remove(coder_id)`, `heap_peek`,
`heap_free`, `sift_up`, `sift_down`, `swap_requests`. All O(log n).

### 3.4 Dongle acquisition / release (`dongle.c`, `acquire.c`, `event.c`)

> **Revised after testing** — see §8 for why the first design was dropped.

Final protocol: **take-both-or-none with a yielding head**.

```
dongle_acquire_pair(coder):
    key = FIFO ? now_us : coder.last_start + t_burnout
    push {coder.id, key} on first.queue and second.queue      (each under its dongle mutex)
    loop:
        if stopped → return 0
        res = try_take(coder, &ready_at)
        res == 1 → return 1                                   (both taken)
        res == 2 → coder_wait(until ready_at)                 (only a cooldown is pending)
        res == 0 → coder_wait(forever)                        (until some dongle changes)

try_take(coder):                       (under sim.table_lock, dongles locked one at a time)
    both dongles free?                                        (holder == 0; ready_at = max(available_at))
    has_priority(first) && has_priority(second)?
        has_priority(d): head = heap_peek(d.queue)
                         head is me, or head is X and X's *other* dongle is held  → X yields
    now < ready_at → 2
    else remove my request from both heaps, set holder = me on both → 1
    (then notify_window on both dongles)

dongle_release(d):  holder = 0; available_at = now + cooldown; notify_window(d)

notify_window(d):   coder_notify() on coders d-2, d-1, d, d+1 (mod n)
                    — every coder whose decision can change when d changes
coder_notify(c):    lock c; notified = 1; signal; unlock
coder_wait(c, t):   lock c; while (!notified) cond_(timed)wait; notified = 0; unlock
```

Properties:
- **Mutual exclusion** per dongle: `holder` is only touched under `d->mutex`.
- **Cooldown**: `available_at` is enforced by the winner via `cond_timedwait`.
- **Fair arbitration**: the heap head has priority; it only yields when it
  physically cannot proceed (its other dongle is held). If the head *can*
  go, nobody passes it.
- **No lost wake-ups**: the `notified` flag turns the cond into a
  level-triggered event; a notification between check and wait is kept.
- **Shutdown**: monitor sets `stopped` and notifies every coder.

### 3.5 Deadlock prevention

Coffman's **hold-and-wait** is removed: a coder takes its two dongles
atomically or not at all, so no coder ever holds a dongle while waiting.
The mutex graph is also acyclic: `table_lock → one dongle.mutex`, and
`coder.lock` / `print_lock` / `start_lock` are always taken alone.

Single coder: `first == second`. The coder takes the one dongle, logs it
once, then waits for `stopped`; the monitor reports the burnout at
`t_burnout`.

### 3.6 Coder routine (`coder.c`)

```
coder_routine(coder):
    wait_start_gate(sim)                          (all threads start together)
    while !stopped:
        if !dongle_acquire_pair(coder) → break
        log "has taken a dongle" (twice; once for a lone coder, who then waits for stop)
        lock(coder->lock); last_start = now; count++; unlock
        log "is compiling";  smart_sleep(t_compile)
        log "is debugging"   (before the release: no neighbour can log
                              "is compiling" with these dongles while this
                              coder still reads as compiling in the log)
        release(first); release(second)
        smart_sleep(t_debug)
        log "is refactoring"; smart_sleep(t_refactor)
    return NULL
```

`last_start` is updated **before** the "is compiling" log so the monitor
never sees a stale deadline after the log timestamp.

### 3.7 Monitor (`monitor.c`)

```
monitor_routine(sim):
    while 1:
        all_done = 1
        for each coder c:
            lock(c->lock)
            if now - c->last_start > t_burnout:
                unlock; log_burnout(c); set stopped; wake_all_dongles; return
            if c->compile_count < compiles_req: all_done = 0
            unlock
        if all_done: set stopped; wake_all_dongles; return
        usleep(500)                                  (0.5 ms → well under the 10 ms budget)
```

`log_burnout` prints `"<ts> <id> burned out"` under `print_lock` and sets
`stopped = 1` in the same critical section. The burnout timestamp is the
detection time; with 1 ms polling the lag is 1–2 ms.

### 3.8 Startup gate & initial deadlines

`init.c` creates every coder thread, then sets `sim->start_ms = now`,
initializes every `last_compile_start = start_ms`, sets `started = 1` and
broadcasts `start_cond`; only then is the monitor started. All coders thus
share the same t=0 and the first log lines have timestamp ≈ 0.

### 3.9 Argument parsing (`parse_args.c`)

- Exactly 8 args (`argc == 9`), else usage on stderr, exit 1.
- Args 1–7: custom strict parser — only ASCII digits (optional leading `+`
  rejected to stay strict), non-empty, no overflow past `INT_MAX`.
  Negative, empty, `"12a"`, `" 12"`, floats → error.
- `number_of_coders ≥ 1`; times and cooldown ≥ 0; `compiles_required ≥ 0`
  (0 → the "all compiled ≥ 0 times" condition is instantly true, the sim
  stops immediately; documented in README).
- Arg 8: `strcmp(s, "fifo") == 0` or `strcmp(s, "edf") == 0`, else error.
- Errors: `fprintf(stderr, "Error: ...\n")`, return 1 from `main`.

### 3.10 Cleanup / error paths (`cleanup.c`)

- Join all coder threads, then the monitor.
- Destroy each dongle's mutex + cond and free its heap; destroy each
  coder's lock; destroy `print_lock`, `start_lock`, `start_cond`; free
  the two arrays.
- If `pthread_create` fails mid-way: set `stopped`, open the start gate,
  join the threads already created, clean up, exit 1.
- `malloc` failure → partial cleanup helper, exit 1.

---

## 4. Norm constraints to design around

- ≤ 25 lines per function, ≤ 5 functions per `.c` file, ≤ 80 columns,
  ≤ 4 parameters, ≤ 5 local variables, declarations at the top,
  **no declaration+assignment on the same line**.
- No `for`, `do…while`, `switch`, `goto`, ternary; only `while`/`if`.
- Tabs for indentation; `t_`/`s_`/`e_` prefixes; `CODEXION_H` include guard;
  no function bodies in the header; 42 header on every `.c`/`.h`.
- Because of the 25-line rule, `dongle_acquire` is split into
  `push_request()`, `wait_turn()`, `take_dongle()`; the monitor loop into
  `check_burnout()` and `all_compiled()`.
- Run `norminette src includes` after every file — it is part of the
  definition of done for each phase.

---

## 5. Implementation phases

Each phase ends with: compiles with `-Wall -Wextra -Werror -pthread`,
`norminette` clean, and the listed check passing.

| Phase | Work | Check |
|---|---|---|
| **0 Scaffold** | Makefile (`NAME=codexion`, `obj/` dir, `-MMD` deps, no relink), `.gitignore`, header with all structs/prototypes, 42 headers | `make && make re` relinks only when needed |
| **1 Parsing + time + log** | `parse_args.c`, `time.c`, `log.c`, `main.c` skeleton that parses and prints the config | invalid inputs rejected with exit 1; valid ones accepted |
| **2 Heap** | `heap.c`, `heap_utils.c` | throwaway test in `tests/` pushes random requests and pops in sorted order for both modes, incl. EDF ties |
| **3 Init + cleanup** | `init.c`, `cleanup.c`, start gate | run with coder routine = "log once and exit"; ASan/leaks clean |
| **4 Dongles + coders** | `dongle.c`, `coder.c` | `5 800 200 200 200 5 0 fifo` runs, each compiling preceded by 2 takes, exits when done |
| **5 Monitor** | `monitor.c`, shutdown path, cooldown timedwait | `1 800 200 200 200 5 0 fifo` burns out at ≈800 ms; nothing printed after |
| **6 README + polish** | `README.md` per Chapter VII; final norm/leak/TSan pass; usage message | full test matrix (§6) green |

---

## 6. Testing plan (`tests/`, not submitted)

Build variants (local only): `make` (release), plus
`cc ... -fsanitize=thread` and `-fsanitize=address` builds via a
`tests/build_san.sh` helper. On macOS use `leaks --atExit -- ./codexion …`
(no valgrind).

Argument tests (expect exit 1 + message on stderr):
`./codexion`, 7 args, 9 args, `0 800 …`, `-5 800 …`, `5 abc …`,
`5 800.5 …`, `5 800 200 200 200 5 0 lifo`, `… FIFO`, `… 99999999999 …`.

Behavior tests (`tests/run_tests.sh` captures stdout and feeds
`check_log.py`, which asserts: monotonic timestamps; every `is compiling`
preceded by exactly two `has taken a dongle` for that coder; `burned out`
is the last line and its timestamp is within `t_burnout + 10 ms` of that
coder's last `is compiling`; no message after burnout; each coder's
compile count ≥ required when no burnout):

| Command | Expected |
|---|---|
| `1 800 200 200 200 5 0 fifo` | 1 takes a dongle, burns out ≈ 800 |
| `5 800 200 200 200 5 0 fifo` / `edf` | no burnout, exits after 5 compiles each |
| `4 610 200 200 200 5 0 fifo` / `edf` | no burnout (2 pairs alternate; a cycle is 600 ms) |
| `4 310 200 100 100 5 0 fifo` | burnout, printed ≤ 10 ms late |
| `2 2000 100 0 0 5 300 fifo` / `edf` | cooldown honored: no take < 300 ms after any release |
| `5 1000 200 200 200 5 100 fifo` / `edf` | feasible with cooldown, no burnout |
| `200 800 200 200 200 3 0 edf` | scales, no crash, no leak |
| `3 800 200 200 200 0 0 fifo` | stops immediately (compiles_req = 0) |

Concurrency checks: run the 5-coder case 20× under TSan; run
`edf` vs `fifo` with tight parameters and compare burnout behavior.

---

## 7. Decisions (confirmed)

1. **42 header**: login `bkhilo`, email `bkhilo@student.42abudhabi.ae` on
   every `.c`/`.h` file.
2. **`number_of_compiles_required = 0`**: accepted — the simulation stops
   immediately (documented in the README). Times of `0` ms are accepted too.
3. **Burnout timestamp**: print the *detection time* (the moment the monitor
   notices it, ≤ 2 ms after the real deadline with 1 ms polling), not the
   theoretical `last_compile_start + time_to_burnout`. Keeps every log line
   on the same "now" clock and preserves monotonic output.
4. **README author line**: *"This project has been created as part of the
   42 curriculum by bkhilo."*

---

## 8. What changed during implementation (and why)

**Design 1 — resource ordering (lower dongle first), hold-and-wait.**
Deadlock-free, but at 200 coders a coder held its first dongle for 600 ms
while waiting for the second, which was stuck in a chain of the same kind.
Its neighbour never compiled and burned out. Hold-and-wait kills liveness.

**Design 2 — take-both-or-none, strict queue head.** No hold-and-wait, but
with equal deadlines at `t = 0` the id tie-break made coder 1 the head of
dongles 0 *and* 1 while coder 5 already held dongle 0. Coder 2 could not
take the free dongle 1, coder 3 waited on 2, … a convoy: one coder
compiling at a time, `5 × 200 ms > 800 ms`, burnout on the canonical
`5 800 200 200 200` run.

**Design 3 (final) — take-both-or-none, yielding head, table lock,
per-coder events.** A head that cannot proceed yields; priority is still
enforced whenever the higher-priority coder can go. Decisions are
serialized by `table_lock`; wake-ups reach every coder within two hops of a
changed dongle. Result: 0/30 burnouts on the 5-coder run, 200 coders under
both policies, and all timing tests inside the 10 ms budget.

**Test-parameter mistakes caught along the way.** `4 410 200 200 200` can
never work (one cycle is 600 ms); `5 800 200 200 200 5 100` is infeasible
under any protocol (five 300 ms occupancies on two parallel slots need
three 300 ms rounds = 900 ms > 800 ms).

**Tooling note.** ASan/TSan runtimes are broken on this macOS 27 toolchain
(a hello-world under ASan spins forever); leak checking was done with
`leaks --atExit`, which reports 0 leaks on every scenario.

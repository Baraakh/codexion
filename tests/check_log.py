#!/usr/bin/env python3
"""Validate a codexion log.  usage: check_log.py N BURNOUT COMPILES_REQ COOLDOWN < log
Checks: line format, monotonic timestamps, 2 takes before each compile, the
compile/debug/refactor cycle, burnout timing (<= 10 ms late), completion
counts, and (for N == 2, where both coders share both dongles) the cooldown.
"""
import re
import sys

LINE = re.compile(r"^(\d+) (\d+) (has taken a dongle|is compiling|is debugging"
                  r"|is refactoring|burned out)$")


def fail(msg):
    print("FAIL:", msg)
    sys.exit(1)


def main():
    n, burnout, req, cooldown = (int(x) for x in sys.argv[1:5])
    lines = [l for l in sys.stdin.read().splitlines() if l]
    last_ts = 0
    takes = {i: 0 for i in range(1, n + 1)}
    phase = {i: "refactoring" for i in range(1, n + 1)}
    count = {i: 0 for i in range(1, n + 1)}
    last_start = {i: 0 for i in range(1, n + 1)}
    last_release = None
    burned = None
    for idx, line in enumerate(lines):
        m = LINE.match(line)
        if not m:
            fail(f"bad line {idx + 1}: {line!r}")
        ts, cid, state = int(m.group(1)), int(m.group(2)), m.group(3)
        if burned is not None:
            fail(f"line after burnout: {line!r}")
        if ts < last_ts:
            fail(f"timestamp went backwards at line {idx + 1}: {line!r}")
        last_ts = ts
        if not 1 <= cid <= n:
            fail(f"unknown coder {cid}")
        if state == "has taken a dongle":
            if phase[cid] != "refactoring":
                fail(f"coder {cid} took a dongle while {phase[cid]}")
            takes[cid] += 1
            if takes[cid] > 2:
                fail(f"coder {cid} took a 3rd dongle")
            if n == 2 and last_release is not None \
                    and ts < last_release + cooldown - 1:
                fail(f"cooldown violated at {ts} (release {last_release})")
        elif state == "is compiling":
            if takes[cid] != 2:
                fail(f"coder {cid} compiles with {takes[cid]} dongle(s)")
            takes[cid] = 0
            phase[cid] = "compiling"
            count[cid] += 1
            last_start[cid] = ts
        elif state == "is debugging":
            if phase[cid] != "compiling":
                fail(f"coder {cid} debugging while {phase[cid]}")
            phase[cid] = "debugging"
            last_release = ts
        elif state == "is refactoring":
            if phase[cid] != "debugging":
                fail(f"coder {cid} refactoring while {phase[cid]}")
            phase[cid] = "refactoring"
        else:
            burned = (cid, ts)
            late = ts - last_start[cid] - burnout
            if late < 0 or late > 10:
                fail(f"burnout of {cid} at {ts} is {late} ms off the deadline")
    if burned is None:
        short = [c for c in count if count[c] < req]
        if short:
            fail(f"no burnout but coders {short} compiled < {req} times")
        print(f"OK: no burnout, {len(lines)} lines, counts {min(count.values())}"
              f"..{max(count.values())}")
    else:
        print(f"OK: coder {burned[0]} burned out at {burned[1]} ms "
              f"(deadline {last_start[burned[0]] + burnout}), {len(lines)} lines")


main()

#!/bin/sh
# Usage: tests/run_tests.sh [binary]   (default ./codexion)
BIN=${1:-./codexion}
DIR=$(dirname "$0")
status=0

expect_fail() {
	if "$BIN" "$@" >/dev/null 2>&1; then
		echo "FAIL: accepted invalid args: $*"; status=1
	else
		echo "ok  : rejected: $*"
	fi
}

run() {
	expect=$1; shift
	out=$("$BIN" "$@")
	res=$(printf '%s\n' "$out" | python3 "$DIR/check_log.py" "$1" "$2" "$6" "$7")
	case "$res" in
		OK*) ;;
		*) echo "FAIL [$*]: $res"; status=1; return ;;
	esac
	case "$expect" in
		burn) printf '%s\n' "$out" | grep -q "burned out" \
			|| { echo "FAIL [$*]: expected a burnout"; status=1; return; } ;;
		live) printf '%s\n' "$out" | grep -q "burned out" \
			&& { echo "FAIL [$*]: unexpected burnout"; status=1; return; } ;;
	esac
	echo "ok  : [$*] $res"
}

echo "== argument validation =="
expect_fail
expect_fail 5 800 200 200 200 5 0
expect_fail 5 800 200 200 200 5 0 fifo extra
expect_fail 0 800 200 200 200 5 0 fifo
expect_fail -5 800 200 200 200 5 0 fifo
expect_fail 5 abc 200 200 200 5 0 fifo
expect_fail 5 800.5 200 200 200 5 0 fifo
expect_fail 5 800 200 200 200 5 0 lifo
expect_fail 5 800 200 200 200 5 0 FIFO
expect_fail 5 800 200 200 200 5 0 ""
expect_fail 5 99999999999 200 200 200 5 0 fifo
expect_fail 5 "" 200 200 200 5 0 fifo

echo "== behaviour =="
run burn 1 800 200 200 200 5 0 fifo
run burn 1 800 200 200 200 5 0 edf
run live 5 800 200 200 200 5 0 fifo
run live 5 800 200 200 200 5 0 edf
run live 4 610 200 200 200 5 0 fifo
run live 4 610 200 200 200 5 0 edf
run burn 4 310 200 100 100 5 0 fifo
run burn 4 310 200 100 100 5 0 edf
run burn 4 590 200 200 200 5 0 edf
run live 2 2000 100 0 0 5 300 fifo
run live 2 2000 100 0 0 5 300 edf
run live 5 1000 200 200 200 5 100 fifo
run live 5 1000 200 200 200 5 100 edf
run live 5 800 200 200 200 5 30 edf
run live 3 800 200 200 200 0 0 fifo
run live 200 800 200 200 200 3 0 edf
run live 200 800 200 200 200 3 0 fifo
run burn 3 1000 400 100 100 10 200 fifo

[ $status -eq 0 ] && echo "ALL TESTS PASSED" || echo "SOME TESTS FAILED"
exit $status

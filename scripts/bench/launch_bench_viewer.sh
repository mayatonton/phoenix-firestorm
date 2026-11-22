#!/bin/zsh
# Log a development AYAstorm build into the local bench grid, at the bench
# view of the busy store region (128, 92), and wait until it is in-world.
#
#   scripts/bench/launch_bench_viewer.sh <AYAstorm.app> [viewer args...]
#
# e.g. AYASTORM_PERF_LOG=5 scripts/bench/launch_bench_viewer.sh \
#        build-darwin-dev/newview/Release/AYAstorm.app --set RenderMirrors 1
#
# AYASTORM_PERF_LOG and AYASTORM_VKC, when set, are passed to the viewer
# (the #VkPerf# summary of the validation guide).
#
# The OpenSim build of the viewer selects the grid with --grid; --loginuri is
# not read, and without --grid it logs into Second Life. The bench account
# comes from $OPENSIM_BENCH_DIR/bench-credentials.txt (first=, last=,
# password=). The app opens in the background (-g) and a build-tree app uses
# the AYAstorm-dev profile, apart from installed releases.
#
# A login right after an unclean exit is refused once with reason
# "presence" (the grid still lists the old session and clears it on that
# refusal); the script then quits the viewer and logs in again.

set -u
B=${OPENSIM_BENCH_DIR:-$HOME/OpenSimBench}
GRID=${BENCH_GRID:-http://127.0.0.1:9000/}
START=${BENCH_START:-secondlife://BenchRegion/128/92/23}
LOG="$HOME/Library/Application Support/AYAstorm-dev/logs/AYAstorm.log"

APP=${1:-}
[[ -d $APP ]] || { echo "usage: $0 <AYAstorm.app> [viewer args...]" >&2; exit 2; }
shift
APP=${APP:A}
EXE="$APP/Contents/MacOS/AYAstorm"
[[ $APP == */build-darwin-* ]] || echo "note: not a build-tree app, it will use the release profile" >&2

overrides=$(env | grep -E '^(VK_ICD_FILENAMES|VK_LAYER_PATH|DYLD_LIBRARY_PATH|DYLD_INSERT_LIBRARIES|AYASTORM_VKCMD_MEMO)=' || true)
[[ -n $overrides ]] && echo "warning: diagnostic overrides are set; this run is not product-equivalent:\n$overrides" >&2

creds=$B/bench-credentials.txt
first=$(sed -n 's/^first=//p' $creds); last=$(sed -n 's/^last=//p' $creds)
password=$(sed -n 's/^password=//p' $creds)
[[ -n $first && -n $last && -n $password ]] || { echo "incomplete $creds" >&2; exit 1; }

quit_viewer() {
  local pid=$(pgrep -f "^$EXE")
  [[ -z $pid ]] && return 0
  kill -TERM $pid
  for i in {1..30}; do kill -0 $pid 2>/dev/null || return 0; sleep 1; done
  echo "viewer did not quit" >&2
  return 1
}

login() {
  # apps started by open(1) do not inherit this shell's environment: pass the
  # measurement switches (AYASTORM_PERF_LOG, AYASTORM_VKC) explicitly
  local -a envs=()
  local v
  for v in AYASTORM_PERF_LOG AYASTORM_VKC; do
    [[ -n ${(P)v:-} ]] && envs+=(--env "$v=${(P)v}")
  done
  # --url is a last_option in cmd_line.xml: extra viewer args must come first
  open -n -g "${envs[@]}" "$APP" --args --grid "$GRID" --login "$first" "$last" "$password" "$@" --url "$START"
  sleep 5  # the viewer starts a fresh log
  for i in {1..180}; do
    if grep -q "STATE_STARTED" "$LOG" 2>/dev/null; then echo started; return; fi
    if grep -q "'reason':'presence'" "$LOG" 2>/dev/null; then echo presence; return; fi
    if grep -q "ERROR_MESSAGE" "$LOG" 2>/dev/null; then echo failed; return; fi
    pgrep -f "^$EXE" >/dev/null || { echo exited; return; }
    sleep 1
  done
  echo timeout
}

pgrep -f "^$EXE" >/dev/null && { echo "this app is already running; quit it first" >&2; exit 1; }
result=$(login "$@")
if [[ $result == presence ]]; then
  echo "login refused (stale session), retrying once"
  quit_viewer && sleep 3 && result=$(login "$@")
fi
case $result in
  started) echo "in-world: pid $(pgrep -f "^$EXE"), log $LOG" ;;
  *) echo "login did not complete ($result), see $LOG" >&2; exit 1 ;;
esac

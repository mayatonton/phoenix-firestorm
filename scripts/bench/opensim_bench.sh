#!/bin/zsh
# Local OpenSim standalone for the AYAstorm busy store bench (macOS).
#
#   scripts/bench/opensim_bench.sh start          start OpenSim in the background
#   scripts/bench/opensim_bench.sh stop           shut it down from its console
#   scripts/bench/opensim_bench.sh cmd <command>  send one console command
#   scripts/bench/opensim_bench.sh load <oar>     load an OAR and wait for it
#
# OPENSIM_BENCH_DIR (default ~/OpenSimBench) holds dotnet/ (the .NET 8
# runtime), opensim/ (the OpenSim 0.9.3 release) and the console files. See
# docs/guides/ayastorm-r42-opensim-bench.ja.md for the setup.
#
# OpenSim runs with the basic console (-console=basic): plain lines from
# stdin, no TTY. The default local console needs a terminal; without one it
# spins at full CPU, and under screen/script it was seen to stop logging.
# Commands go through a FIFO that a sleeping writer keeps open.

set -u
B=${OPENSIM_BENCH_DIR:-$HOME/OpenSimBench}
LOG=$B/opensim-console.log
FIFO=$B/console.fifo

running() { pgrep -f "$B/dotnet/dotnet OpenSim.dll" >/dev/null; }

start() {
  if running; then echo "OpenSim is already running"; return 0; fi
  [[ -x $B/dotnet/dotnet && -f $B/opensim/bin/OpenSim.dll ]] || {
    echo "missing $B/dotnet or $B/opensim (see the guide)" >&2; return 1; }
  [[ -p $FIFO ]] || mkfifo -m 600 $FIFO
  [[ -f $LOG ]] && mv $LOG $B/opensim-console.prev.log
  nohup sleep 2147483647 > $FIFO 2>/dev/null &
  echo $! > $B/console-holder.pid
  (cd $B/opensim/bin && nohup /bin/sh -c \
      "DOTNET_ROOT=$B/dotnet $B/dotnet/dotnet OpenSim.dll -console=basic; echo OPENSIM_EXIT=\$?" \
      < $FIFO >> $LOG 2>&1 &)
  for i in {1..120}; do
    grep -aq "LOGINS ENABLED" $LOG 2>/dev/null && { echo "OpenSim is up (log: $LOG)"; return 0; }
    grep -aq "OPENSIM_EXIT=" $LOG 2>/dev/null && { echo "OpenSim exited, see $LOG" >&2; return 1; }
    sleep 1
  done
  echo "OpenSim did not finish starting in 120 s, see $LOG" >&2
  return 1
}

stop() {
  if running; then
    print -r -- "shutdown" > $FIFO
    for i in {1..60}; do running || break; sleep 1; done
    running && { echo "shutdown timed out, terminating"; pkill -f "$B/dotnet/dotnet OpenSim.dll"; sleep 2; }
  fi
  [[ -f $B/console-holder.pid ]] && kill $(cat $B/console-holder.pid) 2>/dev/null
  rm -f $B/console-holder.pid
  echo "OpenSim stopped"
}

cmd() {
  running || { echo "OpenSim is not running" >&2; return 1; }
  print -r -- "$*" > $FIFO
}

load() {
  local oar=${1:A}
  [[ -f $oar ]] || { echo "no such OAR: $1" >&2; return 1; }
  local from=$(( $(grep -ac "" $LOG) + 1 ))
  cmd "load oar $oar" || return 1
  for i in {1..120}; do
    tail -n +$from $LOG | grep -aq "Successfully loaded archive" && {
      tail -n +$from $LOG | grep -aE "Loaded [0-9]+ scene objects" | sed -E 's/.*\]: //'; return 0; }
    tail -n +$from $LOG | grep -aqiE "Aborting load|Error loading" && {
      echo "load failed, see $LOG" >&2; return 1; }
    sleep 1
  done
  echo "load did not finish in 120 s, see $LOG" >&2
  return 1
}

case ${1:-} in
  start) start ;;
  stop) stop ;;
  cmd) shift; cmd "$@" ;;
  load) shift; load "$@" ;;
  *) echo "usage: $0 start|stop|cmd <command>|load <oar>" >&2; exit 2 ;;
esac

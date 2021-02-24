#!/usr/bin/env python3
from ptrace.debugger import (PtraceDebugger, ProcessEvent,
                             ProcessExit, ProcessSignal, NewProcessEvent)
from ptrace.debugger.child import createChild
import os
import argparse
import signal

def runCommand(cmd, timeout):
    dir = os.path.dirname(os.path.realpath(__file__))
    cmd = [os.path.join(dir, 'cpulimit_exec.sh'), str(timeout)] + cmd
    return createChild(cmd, False)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=('Launch a program with CPU limit, and send SIGINT ' +
                     'to specified process once the limit is reached.'))
    parser.add_argument('cmd', metavar='CMD', type=str, nargs='+', help='Command to execute')
    parser.add_argument('-p', '--pid', type=int, help='PID of process to send SIGINT to')
    parser.add_argument('-t', '--timeout', type=int, help='CPU time limit (in seconds)')

    args = parser.parse_args()
    sigxcpu_handled = False

    debugger = PtraceDebugger()
    debugger.traceFork()
    debugger.traceClone()
    debugger.options |= (1 << 20) # PTRACE_O_EXITKILL
    process = debugger.addProcess(runCommand(args.cmd, args.timeout), True)
    processes = set([process.pid])
    process.cont()

    while processes:
        try:
            # Wait for a SIGXCPU signal.
            ev = debugger.waitSignals(signal.SIGXCPU.value)
            if not sigxcpu_handled:
                sigxcpu_handled = True
                os.kill(args.pid, signal.SIGINT)
            ev.process.cont()
        except NewProcessEvent as ev:
            processes.add(ev.process.pid)
            ev.process.cont()
            ev.process.parent.cont()
        except ProcessExit as ev:
            processes.remove(ev.process.pid)
        except ProcessEvent as ev:
            ev.process.cont()
        except KeyboardInterrupt:
            break

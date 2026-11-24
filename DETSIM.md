# DetSim adaptation (single-threaded default)

This branch makes Rococo runnable as a system under test for DetSim, a
deterministic simulator that checkpoints every process at every syscall
boundary.  The deptran protocol and the workload are unchanged; only the
runtime plumbing differs:

* **one thread per process** - the event loop, the RPC reactor, the accept
  loop, the worker pool and the coordinator all run on the process main
  thread;
* every blocking wait is expressed as something the simulator can observe
  (epoll readiness, a modelled timer, a futex wake), never as a thread the
  simulator cannot schedule;
* time and `rdtsc` are deterministic, and `max_txn` bounds the workload.

This branch defaults to the single-threaded model.  Set `RRR_MULTI_THREAD=1`
in the environment to restore the upstream threading (the original code paths
are kept; see the `detsim-multithread` branch for the opposite default).

## Layout

`rrr/` is vendored instead of being a submodule.  The upstream `.gitmodules`
points at `git://github.com/msmummy/rrr.git`, a protocol GitHub no longer
serves, and there is no public fork that carries these changes.

## Changes

### rrr

* `pylib/yapps`, `bin/rpcgen` - python3 port of the code generator; upstream
  waf is python2-only.
* `base/basetypes` - `Time::sleep()` uses `nanosleep` (DetSim rejects
  `select`); `rdtsc` becomes a deterministic counter.
* `base/threading` - small thread stacks (below); in single-thread mode
  `ThreadPool` has no workers and `run_async()` enqueues into a process-wide
  FIFO drained by `ThreadPool::drain()`.
* `rpc/polling` - `PollThread` can run threadless (`poll_once`), epoll is
  level-triggered, `PollMgr::poll_all()` drives every reactor from the main
  loop.
* `rpc/server` - the listening socket is a `ServerListener` pollable instead
  of an accept thread.
* `rpc/client` - `connect()` retries with a bounded backoff so a client can
  be started before its server finishes binding.
* `base/misc`, `misc/cpuinfo` - deterministic time and CPU count.

### deptran

* `c_main` - the coordinator runs on the main thread and drives the event
  loop instead of waiting on a condition variable another thread would have
  to signal.
* `s_main` - the event loop runs on the main thread.
* `timer` - virtual clock, no `SIGALRM`.
* `config` - the `max_txn` attribute bounds the workload.

## Thread stacks

DetSim snapshots every writable mapping of every thread on every transition,
so the glibc default of 8 MiB per thread dominates the capture cost.  Every
pthread here is created with `RRR_THREAD_STACK_KB` KiB of stack:

```c
#define RRR_THREAD_STACK_KB 32
```

32 KiB is the smallest value verified to work (native run and DetSim run):
the poll reactor, accept loop, worker pool and coordinator all fit, and the
stacks show up as 32 KiB mappings instead of 8 MiB.  16 KiB is
`PTHREAD_STACK_MIN` on x86-64 but leaves no room for TLS and the deeper
marshalling frames.  Override at build time with
`-DRRR_THREAD_STACK_KB=<n>` if a handler needs more.

## Build

waf is python2-only.  The DetSim example builds the two binaries directly
(static, with DWARF):

```sh
scripts/build.sh            # -> build/deptran_{server,client}
```

Static linking is deliberate: DetSim's startup prelude is sensitive to
dynamically linked C++ programs.

## Run

```sh
./build/deptran_server -s 0 -f rococo.xml
./build/deptran_client -c 0 -d 60 -f rococo.xml
```

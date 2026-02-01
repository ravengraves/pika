# libpk

Minimal Linux syscall runtime for Pika and Hare with file I/O, shared memory,
TCP sockets, and process control. This is freestanding (no libc dependency)
and intended for modern Linux.

## Build

From the repo root, ensure the Pika transpiler exists:

```
make pika2c
```

Then build libpk tests:

```
cd lib/libpk
make
make check
```

## Usage (Pika)

Concatenate the module files with your program and transpile:

```
cat lib/libpk/file.pk \
    lib/libpk/net.pk \
    lib/libpk/shm.pk \
    lib/libpk/proc.pk \
    lib/libpk/clock.pk \
    your.pk | build/pika2c > out.c
cc -std=c11 -O2 -fno-builtin out.c lib/pika/rt.c lib/libpk/libpk.c -o yourprog
```

Path strings are null-terminated byte arrays cast to `file::cstr`,
`shm::cstr`, or `proc::cstr`.

## Modules

- `file.pk`: file descriptors, read/write/open helpers.
- `net.pk`: TCP sockets and sockaddr helpers.
- `shm.pk`: mmap/munmap, shared mappings, and ring buffer helpers.
- `proc.pk`: spawn/wait, priorities, and sleep.
- `clock.pk`: rdtsc/counter access and elapsed-time helpers.

## Ergonomic helpers

- `file`: `file::open_ro`, `file::open_rw_trunc`, `file::write_all`,
  `file::read_exact`.
- `shm`: `shm::map_shared_file`, `shm::map_shared_existing`,
  `shm::ring_init`, `shm::ring_write`, `shm::ring_read`.
- `net`: `net::tcp_listen_local`, `net::tcp_connect_local`.
- `proc`: `proc::spawn_simple`, `proc::setpriority_process`,
  `proc::wait_nohang`, `proc::sleep_ms`, `proc::pdeathsig`,
  `proc::kill`, `proc::getppid`.
- `clock`: `clock::now`, `clock::since`, `clock::dt`, `clock::calibrate`.

## Ring pipeline demo

Four-process pipeline built from `lib/libpk/test/ring_*.pk`:

1) `ring_writer` creates and maps the ring buffer file and writes data.
2) `ring_server` reads the ring buffer and serves it over TCP.
3) `ring_client` connects to TCP and writes data to stdout.
4) `ring_supervisor` spawns and monitors the above three processes,
   setting priorities 10, 5, and 0 (lower nice => higher priority).

Build the pipeline:

```
make ring
```

Run from `lib/libpk` (the supervisor uses relative paths):

```
./build/bin/ring_supervisor
```

Use `timeout 5 ./build/bin/ring_supervisor` to stop it. The data flows via:

- ring file: `/tmp/libpk_ring.bin`
- port file: `/tmp/libpk_ring.port`

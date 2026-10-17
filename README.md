# Pika

[![Pika CI](https://github.com/ravengraves/pika/actions/workflows/ci.yml/badge.svg)](https://github.com/ravengraves/pika/actions/workflows/ci.yml)

## Install on Linux

On Ubuntu 24.04, install the prerequisites:

```sh
sudo apt install curl git build-essential python3
```

Install or upgrade the Hare2X Pika transpiler and the Pika compiler driver:

```sh
curl -fsSL https://raw.githubusercontent.com/ravengraves/pika/master/install.sh -o /tmp/pika-install.sh && sh /tmp/pika-install.sh
```

The installer reuses a sibling `hare2x` and `pika` checkout, or creates them under `~/pika`. It builds `pika2c` from the `master` branch of `ravengraves/hare2x` (which provides `-H` module headers), bootstraps `pikacc`, and installs both commands into `/usr/local/bin`. Private repositories require GitHub credentials for both curl and Git.

The installer also copies `lib/` and `runtime/` to `/usr/local/share/pika/`. Both compiler drivers prefer those installed directories when both are present, and otherwise fall back to `lib/` and `runtime/` in the current directory. Installed `pikacc` can compile from any working directory. For example:

```sh
cd /tmp
pikacc ~/pika/pika/test/basic.pka -o /tmp/pika-basic
/tmp/pika-basic
```

## Usage

```sh
pikacc main.pka [options] -o app [-- C compiler arguments...]
```

| Argument | Purpose |
| --- | --- |
| `file.pka ...` | Pika source files |
| `file.c ...` | Additional C source |
| `file.o`, `libfoo.a` | Object files or static libraries to link |
| `-o output` | Output executable (required) |
| `--cc compiler` | Select the C compiler |
| `--platform target` | `x86_64_raw` (default), `aarch64_raw`, `libc`, or `win64` |
| `-R` | Release build: `-O3`, loop unrolling, vectorization, fast math, native CPU tuning, LTO, and stripping |
| `-M directory` | Add a Pika module search root; repeat as needed |
| `-T +tag`, `-T -tag` | Include or exclude tagged module files |
| `-- args...` | Forward all remaining arguments to the C compiler, preserving order and quoting |

Without `-R`, the driver adds no optimization or debug flags. Choose them explicitly, for example `-- -O2` or `-- -O0 -g`. Put Pika inputs and driver options before `--`; C compiler options such as `-march=...`, `-I`, `-D`, `-L`, and `-l` go after it. Forwarded arguments follow the driver's defaults and generated source.

On all supported targets, `-R` disables compiler identification emission (`-fno-ident`) and strips symbols. Non-Windows targets also disable build IDs. Linux Clang release builds use LLD (install `lld`) and add `-fno-PIC` for freestanding raw builds. Raw release builds disable unwind tables with either compiler.

Pass `.a` files directly, or use `-- -L directory -lfoo`. Use `--platform libc` for C libraries that depend on libc:

```sh
pikacc main.pka libfoo.a -R --platform libc -o app
pikacc main.pka --platform libc -o app -- -O2 -L ./libs -lfoo
```

## Build and tests

### Dependencies

| Task | Dependencies |
| --- | --- |
| Build Hare2X's `pika2c` | GNU Make, GCC, Python 3 |
| Bootstrap Pika's `pikacc` | `pika2c` from Hare2X `master` on PATH, GCC with libc development headers, Bash, standard Unix utilities |
| Run `ninja check` | Bootstrap dependencies and Ninja |
| Run `ninja bench` | Ninja, built `bin/pikacc`, `pika2c`, GCC, Clang with LTO support, LLVM/LLD; x86-64 Linux also needs glibc's `libmvec` |
| Run `ninja bench_c` | Ninja and GCC with OpenMP support (`libgomp`) |
| Run `ninja bench_rs` | Ninja, `rustc`, and a working system C linker |
| Download and install | Git, curl, sudo, and the Hare2X/Pika bootstrap dependencies above |

The bootstrap driver uses utilities such as `find`, `sort`, and `mktemp`. The installer builds `pika2c` for you; manual builds need it on PATH first. Tests use GCC explicitly, so installing Clang alone is insufficient.

### Build and check

On Ubuntu 24.04, with `pika2c` on PATH:

```sh
sudo apt install build-essential bash python3 ninja-build
ninja bootstrap_pikacc
ninja check
```

### Optional benchmarks

Install the additional tools, then run the desired benchmark target:

```sh
sudo apt install clang llvm lld rustc
ninja bench       # Pika benchmarks and GCC/Clang math comparisons
ninja bench_c     # C reference benchmarks
ninja bench_rs    # Rust reference benchmarks
# ninja bench_all # All three suites
```

On Ubuntu, `build-essential` supplies the libc development files and GCC's OpenMP support. Rust is only needed for `bench_rs` or `bench_all`.

The native `pikacc` driver selects whole library modules by matching `use <module>` or `use <alias> = <module>` text in the input files, including comments and strings. It also scans selected libraries for their own imports and includes their C backing files. Runtime support and shared platform declarations remain available. Nested forms such as `use foo::path;` and `use b = foo::path;` also match the final module name (`path`); `-M` modules can match their full nested name. This selects libraries without renaming their symbols. Whitespace around aliases and `::` can include spaces, tabs, and newlines.

The bootstrap driver `bin/pikacc.sh` always loads all standard-library modules and their platform C backing files.

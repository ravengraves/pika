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

Run `pikacc` from the pika checkout: it currently reads `lib/` and `runtime/` relative to the working directory. For example:

```sh
cd ~/pika/pika
pikacc test/basic.pka -o /tmp/pika-basic
/tmp/pika-basic
```

## Build and tests

With `pika2c` on PATH:

```sh
sudo apt install build-essential clang llvm lld python3 ninja-build
ninja bootstrap_pikacc
ninja check
```

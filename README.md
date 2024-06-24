# Otter SAPI

This is an experimental SAPI based on [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/).

It should **not** be considered stable by any means.

## Requirements

* PHP 8.1+
* libmicrohttpd

## Dependencies

### Arch Linux

```shell
sudo pacman -S libmicrohttpd
```

### Ubuntu/Debian

```shell
sudo apt-get update --assume-yes && \
  sudo apt-get install libmicrohttpd-dev --assume-yes --no-install-recommends
```

## Build and Test

A list of all available commands is available running `make help` (or simply `make`).

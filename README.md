Yet Another Useless [Saturn] Library
===
<div align="center">
  <img src=".images/logo.png" width="66%" height="66%"/>
</div>
<div align="center">
  <a href="https://travis-ci.org/ijacquez/libyaul">
    <img src="https://img.shields.io/travis/ijacquez/libyaul.svg" alt="Build status"/>
  </a>
  <a href="https://github.com/ijacquez/libyaul/issues">
    <img src="https://img.shields.io/github/issues/ijacquez/libyaul.svg" alt="Issues"/>
  </a>
  <a href="https://github.com/ijacquez/libyaul/commits/develop">
    <img src="https://img.shields.io/github/last-commit/ijacquez/libyaul.svg" alt="Last commit"/>
  </a>
  <a href="https://discord.gg/S434dWA">
    <img src="https://img.shields.io/discord/531844227655532554.svg" alt="Join us on Discord"/>
  </a>
</div>

## About

Yaul is an open source development kit for the SEGA Saturn. The SDK as a whole
aims to minimize the _painful_ experience that is developing for the Saturn by
providing lightweight abstractions between your program and the hardware.

## Documentation

Visit [yaul.org][1].

## Pre-built environment

### Windows

#### Installer

If you don't already have MSYS2 installed, download the _Yaul MSYS2 64-bit_
installer from the release [page][2].

#### Existing MSYS2

If you already have MSYS2 installed, follow the directions below to setup access
to the package repository.

1. Open `/etc/pacman.conf` and at the end of the file, add the following:

       [yaul-mingw-w64]
       SigLevel = Optional TrustAll
       Server = http://packages.yaul.org/mingw-w64/x86_64

2. Sync and refresh the databases.

       pacman -Syy

3. To list the packages for the `yaul-mingw-w64` repository, use:

       pacman -Sl yaul-mingw-w64

4. Install everything.

       pacman -S \
         yaul-tool-chain-git \
         yaul-git \
         yaul-emulator-yabause \
         yaul-emulator-mednafen \
         yaul-examples-git

5. Be sure to copy `/opt/tool-chains/sh2eb-elf/yaul.env.in`. This is your
   environment file.

6. Once copied, follow the steps in setting up your [environment
   file](#setting-up-environment-file).

7. Test your environment by building an
   [example](#building-and-running-an-example).

### Linux

#### Arch

Follow the directions below to setup access to the Arch Linux package
repository, or build the [packages][6] yourself.

1. As `root`, open `/etc/pacman.conf` and at the end of the file, add the
   following:

       [yaul-linux]
       SigLevel = Optional TrustAll
       Server = http://packages.yaul.org/linux/x86_64

2. Sync and refresh the databases.

       pacman -Syy

3. To list the packages for the `yaul-linux` repository, use:

       pacman -Sl yaul-linux

4. Install everything.

       pacman -S \
         yaul-tool-chain-git \
         yaul-git \
         yaul-emulator-mednafen \
         yaul-emulator-kronos \
         yaul-examples-git

5. Be sure to copy `/opt/tool-chains/sh2eb-elf/yaul.env.in`. This is your
   environment file.

6. Once copied, follow the steps in setting up your [environment
   file](#setting-up-environment-file).

7. Test your environment by building an
   [example](#building-and-running-an-example).

#### Debian based

There are currently no `.deb` packages available.

### MacOS X

There are currently no packages available.

## Building tool-chain from source

Follow the instructions found in the [`build-scripts/`][5] submodule directory.
Please note, you still need to [build Yaul](#building-yaul-manually).

## Building Yaul manually

### Cloning the repository

1. Clone the repository.

       git clone --recursive "https://github.com/ijacquez/libyaul.git"

### Building

1. Follow the steps in setting up your [environment
   file](#setting-up-environment-file).

2. Build and install the supported libraries.

       SILENT=1 make install-release

   If any given library in Yaul is being debugged, use the `install-debug`
   target instead. Either _release_ or _debug_ can currently be installed at one
   time. It's possible to switch between the two in the same installation.

   To find more about other targets, call `make list-targets`.

3. Build and install the tools.

       SILENT=1 make install-tools

4. Test your environment by building an
   [example](#building-and-running-an-example).

## Setting up environment file

1. Copy the template `yaul.env.in` to your home directory as `.yaul.env`. This
   is your environment file.

2. Open `.yaul.env` in a text editor and change the following to define your
   environment:

   1. Set the absolute path to the tool-chain in `YAUL_INSTALL_ROOT`.
   2. If necessary, set `YAUL_PROG_SH_PREFIX` and `YAUL_ARCH_SH_PREFIX`.
   3. Set the absolute path to where the `libyaul` source tree is located in
      `YAUL_BUILD_ROOT`.
   4. Set the type of development cart you own in `YAUL_OPTION_DEV_CARTRIDGE`.
      If you don't own a development cart, or you will only test on emulators,
      set to 0 (zero).
   5. Enable RTags/Irony/ccls support by setting `YAUL_CDB` to 1. To disable,
      set to 0 (zero).

   Setting the wrong values may result in compilation errors.

3. Read the environment file `.yaul.env` into your current shell.

       source ~/.yaul.env

4. Reading the environment file needs to be done every time a new shell is
   opened. To avoid having to do this every time, add the line below to your
   shell's startup file.

       echo 'source ~/.yaul.env' >> ~/.bash_profile

   If `.bash_profile` is not used, use `.profile` instead. This is dependent on
   your set up.

## Building and running an example

1. If you've built Yaul manually, check out any example in the [`examples`][4]
   submodule. Otherwise, go to `/opt/yaul-examples/`.

2. Copy the `vdp1-balls` example to your home directory

       cp -r vdp1-balls ~

3. Build `vdp1-balls`

       cd ~/vdp1-balls
       SILENT=1 make clean
       SILENT=1 make

4. If you have Mednafen or Yabause correctly configured, you can test the
   example.

       mednafen vdp1-balls.cue

5. Success! :tada:

<p align="center">
  <img src=".images/results.png" alt="Balls!">
</p>

## Contact

You can find me (*@mrkotfw*) on [Discord]( https://discord.gg/S434dWA).

[1]: https://yaul.org/
[2]: https://github.com/ijacquez/libyaul-installer/releases/latest
[3]: https://github.com/ijacquez/libyaul-packages/releases/latest
[4]: https://github.com/ijacquez/libyaul-examples
[5]: https://github.com/ijacquez/libyaul-build-scripts
[6]: https://github.com/ijacquez/libyaul-packages

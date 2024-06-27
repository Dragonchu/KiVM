KiVM
=============
[![Build Status](https://travis-ci.org/imkiva/KiVM.svg?branch=master)](https://travis-ci.org/imkiva/KiVM)
[![GitHub top language](https://img.shields.io/github/languages/top/imkiva/KiVM.svg)](https://github.com/imkiva/KiVM)
[![license](https://img.shields.io/github/license/imkiva/KiVM.svg?colorB=000000)](https://github.com/imkiva/KiVM)

Kiva's Java Virtual Machine.

### Features

- JNI Support
- JAR Class Loading Support (libzip needed)
- Full OracleJDK/OpenJDK compatibility
- Copying Garbage Collector

### Build

1. Requirements
    * Linux, macOS, or Windows(untested) .
    * JDK (OpenJDK or OracleJDK) (>= 8)
    * CMake (>= 3.2)
    * libzip (>= 1.5.1) (to support JAR Class Loading)

2. Build instructions
    * Clone this repo.
    * `cd` into your directory that contains KiVM source code.
    * Type `cmake . && make` in your terminal app.
    * Enjoy it!

### Usage

```
Usage:
        java [-?] [-v] [-cp <path>] [-classpath <path>] <class-name> [<args>]...

Options:
        -?, -help           show help
        -v, -version        show version
        -cp <path>          class search path
        -classpath <path>   same as -cp
        <class-name>        name of the class to run

```

### Credit

* Inspired by [wind_jvm](https://github.com/wind2412/wind_jvm)
* Modified version of [libzippp](https://github.com/ctabin/libzippp)
* Command line options parsing using [clipp](https://github.com/muellan/clipp)

## Acknowledge

We would like to thank [JetBrains](https://www.jetbrains.com/?from=mozart++) for sharing free
open-source licences of amazing tools such as [CLion](https://www.jetbrains.com/idea/?from=mozart++).

[<img src="logo/jetbrains.png" width="200"/>](https://www.jetbrains.com/?from=mozart++)

### See Also

* [HiVM](https://github.com/imkiva/HiVM)
* [The Java Virtual Machine Specification Java SE 8 Edition](https://docs.oracle.com/javase/specs/jvms/se8/html/)
* [HotSpot Virtual Machine Garbage Collection Tuning Guide](https://docs.oracle.com/en/java/javase/11/gctuning/preface.html#GUID-5650179B-DC2A-4F25-B2C6-F3961C93FD07)


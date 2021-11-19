# NVM Unsafe Java Library

## Description
This library implements an Unsafe API Library about managing unsafely off-heap
memory.

## Prerequisites
To use the Persistent Memory Java Library you have first install the
`PMDK: Persistent Memory Development Kit`. We use the libvmem library. 
For more information see:
https://github.com/pmem/pmdk

```
$ wget https://github.com/jackkolokasis/pmdk/archive/refs/heads/fastmap.zip
$ unzip fastmap.zip
$ cd fastmap
$ make
$ sudo make install prefix=/usr/local
```

## Build
To build all shared libraries and classes for NVM Unsafe library:

```
$ make all  
$ make NVMUnsafe.jar
$ sudo make install 
```
Or run:
```
./build.sh
```

Set the dynamic library path
```
$ export LD_LIBRARY_PATH = /usr/local/lib:$LD_LIBRARY_PATH
$ export LD_LIBRARY_PATH = /usr/local/lib64:$LD_LIBRARY_PATH
```

## Example usage
Inside the nvmUnsafe directory there is a test file to check if the
building of the library succeed.
```
$ make test
```

## Authors
Iacovos G. Kolokasis,
Polyvios Pratikakis,
Angelos Bilas

if test "x${je_cv_madvise}" = "xyes" ; then
  ;
fi

#AC_DEFINE([JEMALLOC_HAVE_MADVISE], [ ])

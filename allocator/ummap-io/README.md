# User-level Memory-mapped I/O

## How to compile

1. Clone https://github.com/manosanaggh/parallax/tree/manos/cs647_project/12.2022

2. cd parallax

3. Change the destination path of "cp -r" in "compile.sh" to /path/to/teraheap/allocator/ummap-io/ so the libparallax.so (parallax implementation) is copied to your ummap-io folder in order to build it.

4. Run "compile.sh -c".

5. Add the following to your ~/.bashrc and source it:

export LIBRARY_PATH=/path/to/teraheap/allocator/ummap-io/build/lib/:$LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/teraheap/allocator/ummap-io/build/lib/:$LD_LIBRARY_PATH
export PATH=/path/to/teraheap/allocator/ummap-io/include/:$PATH
export C_INCLUDE_PATH=/path/to/teraheap/allocator/ummap-io/include/:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=/path/to/teraheap/allocator/ummap-io/include/:$CPLUS_INCLUDE_PATH
export LIBRARY_PATH=/path/to/teraheap/allocator/lib/:$LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/teraheap/allocator/lib/:$LD_LIBRARY_PATH
export PATH=/path/to/teraheap/allocator/include/:$PATH
export C_INCLUDE_PATH=/path/to/teraheap/allocator/include/:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=/path/to/teraheap/allocator/include/:$CPLUS_INCLUDE_PATH
export LIBRARY_PATH=/path/to/teraheap/allocator/ummap-io/:$LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/teraheap/allocator/ummap-io/:$LD_LIBRARY_PATH


6. The `src` folder contains the source code and all the necessary
elements. Type the following to compile both the library and a simple
example application:

```
make rebuild
```


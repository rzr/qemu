### 1 Build

Clone and cd into qemu directory, run:

```
./1-conf.sh && make
```

If you get this link error:

```
/usr/bin/ld: hw/vigs/vigs_gl_backend_glx.o: undefined reference to symbol 'dlclose@@GLIBC_2.2.5'
/usr/lib/libdl.so.2: error adding symbols: DSO missing from command line
collect2: error: ld returned 1 exit status
```

Add `LIBS=-ldl` before `make`

Like this:

```
LIBS=-ldl make
```

### 2 Get Tizen:Common image

Download [Image](https://drive.google.com/file/d/0B_GT1uhG7YaDQ1RFQ1JhblNFZDQ/edit?usp=sharing) and
put it to ./data

### 3 Run

```
./1-run.sh
```

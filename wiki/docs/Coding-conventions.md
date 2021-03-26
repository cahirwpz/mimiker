Before you start implementing anything please carefully read following rules.

**Coding style.**

> All source code in the repository should be formatted according to [LLVM coding standards](http://llvm.org/docs/CodingStandards.html#source-code-formatting). You don't have to read this document, there's a tool that'll do the work for you - [clang-format](http://clang.llvm.org/docs/ClangFormat.html). Please use it by running `make format`.

**Explicit size for basic data types.**

> Please use data types from [stdint.h](http://www.cplusplus.com/reference/cstdint/), namely:
> * `uint8_t` instead of `unsigned char`
> * `uint16_t` instead of `unsigned short int`
> * `int16_t` instead of `short int`
> * `int32_t` instead of `int`
> * `uint32_t` instead of `unsigned int`
> * `int64_t` instead of `long`
> * `uint64_t` instead of `unsigned long`

**Put all header files into `include` directory.**

> Additionaly each header file `${name}.h` should be created from following template:

    #ifndef __NAME_H__
    #define __NAME_H__
    ...
    #endif /* __NAME_H__ */

**Name functions according to following rules:**

> 1. __Always__ give a function reasonably self-explaining name.
> 2. If a function belongs to `${phys_mem}.h` file please choose a reasonable abbreviation for `phys_mem`, for instance `pm`, and prefix all functions that belong to this header with `pm_`.
> 3. If a function performs an `action` on an `object` please name it `object_action` (e.g. `pm_frames_alloc`).
> 4. Otherwise just name it as you wish (e.g. `pm_get_frame_size`).
<p align="center">
    <img src="assets/lime.svg" width=400>
</p>

# 👋 Introduction
Lime is a *cross-platform* framework that is focused on game modding and tries to provide you with useful features for your journey.


# 🗒️ Features
- Detours
  - x86/x86-64
  - Supports lambdas as detours, similar to [rcmp](https://github.com/Smertig/rcmp)
- Instruction
  - Allows to work with instructions (i.e. follow jumps/calls, ...)
- Memory Pages
  - Easy Allocation
- Module
  - Iterate Loaded Modules
  - Iterate Symbols
- Address
  - Read / Write Data
- Signature Scanner
  - Supports Traditional & IDA Signatures
- Cross-Platform Entrypoint

> Lime follows the `RAII` paradigm, so you won't have to care about manually cleaning anything up (i.e. when allocating a page).

# ⚙️ Configuration

### Entrypoint
```cmake
set(lime_static_entrypoint ON)
```
> Uses a platform-independent method for the entrypoint implementation.  
> You do not need to enable this to make use of the cross-platform entrypoint!

### Tests
```cmake
set(lime_tests ON)
```
> If `ON`, tests will be built.  

# 📦 Installation
- FetchContent
  ```cmake
  include(FetchContent)

  FetchContent_Declare(
        lime
        GIT_REPOSITORY "https://github.com/Curve/lime"
  )

  FetchContent_MakeAvailable(lime)

  target_link_libraries(<YourLibrary> cr::lime)
  ```

- Git Submodule
  ```bash
  git submodule add "https://github.com/Curve/lime"
  ```
  ```cmake
  # CMakeLists.txt
  add_subdirectory("<path_to_lime>")

  target_link_libraries(<YourLibrary> cr::lime)
  ```

# 📖 Examples

https://github.com/Curve/lime/blob/9ca0e477e6d5493f3b82ea79b51a2e2eb9f8b85f/tests/hook.test.cpp#L25-L29
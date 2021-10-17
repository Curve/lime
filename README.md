<p align="center">
    <img src="assets/lime.svg" width=400>
</p>

# 👋 Introduction
Lime is a *cross-platform* framework that is focused on game modding and tries to provide you with useful features for your journey.

> This library is meant to be used in mods that get loaded into the target process. Remote usage is not yet available but may be in the future, albeit with low priority.

# 🗒️ Features
- Detours <sub><sup>(x86 & x64)</sub></sup>
- Memory utility
  - Read
  - Write
  - Allocate
  - Free
  - Protect
- Console utility
  - Allocate console¹
  - Redirect `cout` to file
- Address utility
  - Get current mnemonic
  - Read until specific mnemonic
  - Follow calls/jumps
  - Read as type
- Keyboard utility²
  - Press / Release key
  - Check if key is pressed
  - Check if key is pressed and was previously not pressed
- Signature scanner
  - Find single / all occurrences
  - IDA / Code Style signatures supported
  - Search whole process / specific module / specific page
- Page utility
  - Get Page protection, start & end
- Module utility
  - Get loaded modules
  - Get module by name
  - Get module size / name
  - Retrieve specific symbol 
- Cross-platform entry-point 

> ¹ On linux no console will be opened because it is not portable, instead a bash script (`tools/listen.sh`) is used to follow the output

> ² The keyboard utility is meant to be a cross-platform `GetAsyncKeyState`, on windows it will only serve as a small abstraction to `GetAsyncKeyState`.

# 🧰 Tools <sub><sup>(Linux only)</sub></sup>
### `inject_so.sh`
Allows you to inject a shared object file into a process.  
Usage: `./inject_so.sh <process_name/process_id> <library_path> (--unload)`

### `listen.sh`
Allows you to follow the output of an "allocated console" on linux.  
Usage: `./listen.sh (name)`
> If you don't supply a name in the `alloc_console` call you should also not supply a name here.

# 📑 Documentation
The documentation can be found [here](https://github.com/Curve/lime/wiki/Documentation).  
If you want code examples you can take a look at the [tests](tests/).

# ⚙️ Installation
- FetchContent
  ```cmake
  include(FetchContent)

  FetchContent_Declare(
        lime
        GIT_REPOSITORY "https://github.com/Curve/lime"
  )

  FetchContent_MakeAvailable(lime)

  target_link_libraries(<YourLibrary> lime)
  ```

- Git Submodule
  ```bash
  git submodule add "https://github.com/Curve/lime"
  ```
  ```cmake
  # CMakeLists.txt
  add_subdirectory("<path_to_lime>")
  ```

# ✅ Requirements

| Operating System | Requirements |
| ---------------- | ------------ |
| Windows          | Windows >10  |
| Linux            | X11          |
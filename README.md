# sometil — A file dumper/searcher  

**Ugly, and doesn’t ask permission.**  

A lightweight CLI utility for **hex/viewing** and **pattern searching** in files.  
Written in C because **I am bored**. 

## Features  
- **Search modes**:  
  - Text (`-s "cool pattern"`)  
  - Hex (`-x "DEADBEEF"`)  
- **View modes**:  
  - Raw bytes (`-v raw`)  
  - Hex dump (`-v hex`)  
  - ASCII+hex (`-v ascii`)  
  - Text-only (UTF-8 aware, `-v textonly`)  
- **Metrics**: Count matches (`-nc` to disable), measure time (`-t`).  
- **Tunable**: Bytes per line (`-w 32`).  
- **Style**: Grep mode (`-g`)

## Usage  
```sh
sometil file.txt -s "error"  # Text search  
sometil file.bin -x "C0FFEE" -t  # Hex search with timing  
sometil file.log -v ascii -w 64  # Custom hex/ASCII view 
```

Here's a concise **README.md** section for your GitHub project explaining how to build it:

---

## 🛠 Building the Project

### Prerequisites
- [Clang](https://clang.llvm.org/) or [GCC](https://gcc.gnu.org/) installed
- Windows/Linux/macOS terminal

### Quick Build
```bash
# Clone the repository
git clone https://github.com/yourusername/sometil.git
cd sometil

# Build with default settings (using clang)
./build.sh  # Linux/macOS
build.bat   # Windows
```

### Custom Build Options
Edit these variables in `build.sh` or `build.bat`:
```bash
COMPILER="gcc"       # Change to gcc if preferred
STANDARD="c17"       # C standard (c11/c17/c2x)
OPTIMIZATION="-O2"   # Optimization level (-O0 to -O3)
```
The binary `sometil` (or `sometil.exe`) will be generated in the project root.

---

### Key Notes:
- ✅ **Tested** with Clang 11+ and GCC 99+
- 🔧 Adjust `build.sh`/`build.bat` for custom source files
- ⚠️ Requires C11 standard or newer

For development, consider using [CMake](https://cmake.org/) for cross-platform builds (let me know if you'd like a CMake example!).
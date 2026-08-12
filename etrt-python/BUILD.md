# Building etrt-python

## Prerequisites

- Python 3.7+
- CMake 3.15+
- C++17 compatible compiler
- ET runtime library installed (findable via `find_package(runtime)`)
- nanobind (installed automatically via pip)

## Installation

### Option 1: Using pip (Recommended)

This will automatically build the extension using CMake:

```bash
pip install .
```

For development (editable install):

```bash
pip install -e .
```

### Option 2: Manual CMake build

If you want more control over the build process:

```bash
# Install nanobind first
pip install nanobind

# Create build directory
mkdir build
cd build

# Set ET platform if not at /opt/et (default)
export ET_PLATFORM=/path/to/et

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# The extension will be in etrt/_etrt_native.*.so
```

## Build Configuration

### Finding the runtime library and dependencies

The build system uses `find_package(runtime REQUIRED)` to locate the ET runtime library and its dependencies.

**ET Platform Path**: The build system looks for the ET platform installation, which defaults to `/opt/et`.

You can override this by setting the `ET_PLATFORM` environment variable:

```bash
# Using pip with custom ET platform path
ET_PLATFORM=/path/to/et pip install .

# Using manual CMake
cmake .. -DET_PLATFORM=/path/to/et
# Or via environment variable
export ET_PLATFORM=/path/to/et
cmake ..
```

**Directory Structure**: The ET platform should be installed with all CMake config files in `<ET_PLATFORM>/lib/cmake/`:
- `runtime/runtimeConfig.cmake`
- `deviceLayer/deviceLayerConfig.cmake`
- `linuxDriver/linuxDriverConfig.cmake`
- `hostUtils/hostUtilsConfig.cmake`
- And other dependencies...

### Debug builds

For debug builds with symbols:

```bash
# Using pip
pip install . --config-settings=cmake.build-type=Debug

# Using manual CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

## Testing

After installation, run tests:

```bash
# Install pytest
pip install pytest

# Run tests
pytest tests/

# Or run specific test files
python tests/test_runtime.py
python tests/test_types.py
```

## Running Examples

```bash
# Basic usage (requires PCIe hardware or emulator)
python examples/basic_usage.py

# System emulator example
python examples/sysemu_example.py

# Async operations
python examples/async_operations.py
```

## Troubleshooting

### CMake can't find runtime or dependencies

```
CMake Error at CMakeLists.txt:23 (find_package):
  Could not find a package configuration file provided by "runtime"
```

Or:

```
CMake Error: Could not find a package configuration file provided by "linuxDriver"
```

**Solution**: Set the `ET_PLATFORM` environment variable to the ET platform installation directory:

```bash
export ET_PLATFORM=/opt/et
pip install .
```

Or pass it directly:

```bash
ET_PLATFORM=/path/to/et pip install .
```

**Verify installation**: Check that the ET platform is properly installed:

```bash
# Check for runtime config
ls $ET_PLATFORM/lib/cmake/runtime/runtimeConfig.cmake

# Check for dependencies
ls $ET_PLATFORM/lib/cmake/deviceLayer/
ls $ET_PLATFORM/lib/cmake/linuxDriver/
```

### nanobind not found

```
CMake Error: nanobind not found. Install with: pip install nanobind
```

Solution:

```bash
pip install nanobind
```

### Import error: cannot import name '_etrt_native'

This means the native extension wasn't built or installed correctly. Try:

```bash
# Reinstall
pip install --force-reinstall .

# Check if extension exists
ls etrt/_etrt_native*.so
```

### Symbol not found / Undefined symbol errors

This usually means:
1. Runtime library version mismatch
2. ABI incompatibility
3. Missing dependencies (deviceLayer, hostUtils, etc.)

Solution:
- Rebuild the runtime library and dependencies
- Make sure C++ compilers match
- Check that the runtime library exports all required symbols
- Verify all dependencies are installed (cereal, fmt, hostUtils, deviceApi, deviceLayer, sw-sysemu, libcap, easy_profiler)

## Development Workflow

For active development:

```bash
# Set ET platform if needed
export ET_PLATFORM=/opt/et

# Install in editable mode
pip install -e .

# Make changes to C++ code
vim src/runtime_bindings.cpp

# Rebuild
cd build
make

# Test changes
python -c "import etrt; print(etrt.__version__)"
```

## Cross-compilation

For cross-compilation, set appropriate CMake toolchain file:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake
```

## Packaging

To create a wheel distribution:

```bash
pip install build
python -m build
```

This will create a wheel in `dist/` directory.

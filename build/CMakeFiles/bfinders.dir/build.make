# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.31

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "C:\Program Files\CMake\bin\cmake.exe"

# The command to remove a file.
RM = "C:\Program Files\CMake\bin\cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\build

# Include any dependencies generated for this target.
include CMakeFiles/bfinders.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/bfinders.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/bfinders.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/bfinders.dir/flags.make

CMakeFiles/bfinders.dir/codegen:
.PHONY : CMakeFiles/bfinders.dir/codegen

CMakeFiles/bfinders.dir/Bfinders.c.obj: CMakeFiles/bfinders.dir/flags.make
CMakeFiles/bfinders.dir/Bfinders.c.obj: CMakeFiles/bfinders.dir/includes_C.rsp
CMakeFiles/bfinders.dir/Bfinders.c.obj: C:/Users/kalok/Downloads/Chunkbiomes-main/Chunkbiomes-main/Bfinders.c
CMakeFiles/bfinders.dir/Bfinders.c.obj: CMakeFiles/bfinders.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/bfinders.dir/Bfinders.c.obj"
	C:\msys64\ucrt64\bin\cc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/bfinders.dir/Bfinders.c.obj -MF CMakeFiles\bfinders.dir\Bfinders.c.obj.d -o CMakeFiles\bfinders.dir\Bfinders.c.obj -c C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\Bfinders.c

CMakeFiles/bfinders.dir/Bfinders.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/bfinders.dir/Bfinders.c.i"
	C:\msys64\ucrt64\bin\cc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\Bfinders.c > CMakeFiles\bfinders.dir\Bfinders.c.i

CMakeFiles/bfinders.dir/Bfinders.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/bfinders.dir/Bfinders.c.s"
	C:\msys64\ucrt64\bin\cc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\Bfinders.c -o CMakeFiles\bfinders.dir\Bfinders.c.s

# Object files for target bfinders
bfinders_OBJECTS = \
"CMakeFiles/bfinders.dir/Bfinders.c.obj"

# External object files for target bfinders
bfinders_EXTERNAL_OBJECTS =

libbfinders.a: CMakeFiles/bfinders.dir/Bfinders.c.obj
libbfinders.a: CMakeFiles/bfinders.dir/build.make
libbfinders.a: CMakeFiles/bfinders.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library libbfinders.a"
	$(CMAKE_COMMAND) -P CMakeFiles\bfinders.dir\cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\bfinders.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/bfinders.dir/build: libbfinders.a
.PHONY : CMakeFiles/bfinders.dir/build

CMakeFiles/bfinders.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\bfinders.dir\cmake_clean.cmake
.PHONY : CMakeFiles/bfinders.dir/clean

CMakeFiles/bfinders.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\build C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\build C:\Users\kalok\Downloads\Chunkbiomes-main\Chunkbiomes-main\build\CMakeFiles\bfinders.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/bfinders.dir/depend

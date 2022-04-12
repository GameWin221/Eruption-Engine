# Dependencies
### Linux:
- CMake 3.1+
- Any C++ compiler  with **C++20** support
### Windows:
- Visual Studio 2022

### Both OS:
- Vulkan SDK 1.3+
- glfw 3.3+
- GL

# How to compile?
## Windows
Just open the `EruptionEngine.sln` with Visual Studio 2022 file and compile it by pressing `F11`.

## Linux
If you already have the dependencies properly installed, you can skip to step 4.

Also, you can delete the `External/GLFW` directory when compiling on Linux because it contains (unnecessary) Windows-only files.
1. Download GL:
`sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev`
2. Download GLFW:
`sudo apt install libglfw3-dev`
3. Download the Vulkan SDK [as shown on the official LunarG site](https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html).
Although it's simpler **on Ubuntu 20.04**:

```
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-focal.list http://packages.lunarg.com/vulkan/lunarg-vulkan-focal.list
sudo apt update
sudo apt install vulkan-sdk
```
4. Now `cd` into the repo folder, then:
```
cd Build
cmake ..
make
```

5. Just run it:
```
./EruptionEngine
```

# How to compile the shaders?
## Windows
Enter the `Shaders` directory and run the `compile.bat` file.

## Linux
Enter the `Shaders` directory and run the following command:
```
bash compile.sh
```
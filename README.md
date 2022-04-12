# Eruption-Engine

![](https://forthebadge.com/images/badges/works-on-my-machine.svg)
![](https://forthebadge.com/images/badges/made-with-c-plus-plus.svg)

My own rendering engine made with Vulkan.

The engine is in a really early stage of development.

# Dependencies
### Windows:
- Visual Studio 2022
### Linux:
- CMake 3.1+
- Any C++ compiler  with **C++20** support
- GLFW 3.3+
- GL
### Both OS:
- Vulkan SDK 1.3+

# How to compile?
## Windows
1. Download [Vulkan SDK]([https://vulkan.lunarg.com/sdk/home#windows](https://vulkan.lunarg.com/sdk/home#windows)) and install it wherever you want.
2. Open the `EruptionEngine.sln` file  with Visual Studio 2022 and compile it by pressing `F11`.


## Linux
If you already have the dependencies properly installed, you can skip to step 4.

Also, you can delete the `External/GLFW` directory when compiling on Linux because it contains (unnecessary) Windows-only files.
1. Download GL:

```sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev```

2. Download GLFW:

```sudo apt install libglfw3-dev```

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

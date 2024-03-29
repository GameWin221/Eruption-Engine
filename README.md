## This is the old version of Eruption Engine the new remake is in [this repository](https://github.com/GameWin221/Gemino)

# Eruption Rendering Engine

![](https://forthebadge.com/images/badges/made-with-c-plus-plus.svg)

This is my own rendering engine using Vulkan as its graphical API. I started this project in late March of 2022 because I wanted to learn more about modern computer graphics and I'm still working on it to this day.

### [Eruption Engine Milanote Board](https://app.milanote.com/1NmtuU1jo13IbQ?p=vUC5d3l4PKq)

## Implemented major features:
- Clustered Forward Renderer
- Cascaded Shadows
- PBR Materials.
- Point, Spot and Directional lights.
- Soft PCF shadows.
- Partly Bindless Rendering
- Depth pre-pass.
- FXAA antialiasing.
- HDR tonemapping.
- Simple asset manager.
- Custom GLTF model loader.
- Runtime modifiable and assignable materials.
- Simple scene editor.
- ImGui UI.
- And more coming in the future... :)

# Dependencies
- Visual Studio 2022
- Vulkan SDK 1.2+

# How to compile?
1. Download [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows) and install it wherever you want.
2. Open the `EruptionEngine.sln` file  with Visual Studio 2022 and compile it by pressing 'Ctrl + Shift + B' or `Build > Build Solution`.

# How to compile the shaders?
Enter the `Shaders` directory and run the `compile.bat` file.

## Known issue:
The app doesn't launch on Intel GPUs due to their low max per-stage image count.

# In engine screenshots:
![](https://user-images.githubusercontent.com/72656547/182171704-8ca8ca6f-d27d-4aaa-a66c-b266cf18b7fc.jpg)
![](https://user-images.githubusercontent.com/72656547/182172048-73e7951d-739f-47a2-ac4a-44e7d64802fc.jpg)
![](https://user-images.githubusercontent.com/72656547/182173442-0d863f3c-ef96-4ec9-ab1b-0ae1a62b3b4e.jpg)

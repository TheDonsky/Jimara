# Jimara

Jimara will be a simplistic game engine initially built for my own projects, as well as educational purposes.

Initiall incarnation of the project will come with Windows and Linux support, MAC will be comming later; Consoles may or may not come and if they do, due to likely NDA-s, they'll likely be tucked away somewhere on a private branch or something anyway...

README will be updated as we go further and the project evolves beyond the starter code :).


## Project configuration:

### Windows:
0. Install the latest version of ```python3```.
1. Install the latest version of ```Microsoft VisualStudio 2019```.
2. Install the latest version of ```LunarG Vulkan SDK```.
3. Add ```JIMARA_VULKAN_SDK``` environment variable to your system (for example, ```{name:"JIMARA_VULKAN_SDK"; value:"C:\VulkanSDK\1.2.154.1"}```).
4. Make full recursive clone of Jimara repository (additional dependencies are included as submodules):
    ```
    git clone --recursive https://github.com/TheDonsky/Jimara.git
    ```
5. ```Run jimara_initialize.py``` from the repository to create symbolic links that make the source visible to MSVS projects.
6. Open ```Project/Windows/MSVS2019/Jimara.sln``` with MSVS2019.
7. Build the solution to generate ```Jimara.lib``` files for each configuration, as well as corresponding Google Test runner executables (all stored in ```__BUILD__``` directory).

### Linux:
0. Make sure to have ```python3``` installed
1. Make sure to gave ```gcc``` installed
2. Make full recursive clone of Jimara repository (additional dependencies are included as submodules):
    ```
    git clone --recursive https://github.com/TheDonsky/Jimara.git
    ```
3. Install Packege dependencies:
    - On debian based systems run: 
       ```
       sudo apt install libgtest-dev cmake vulkan-tools libvulkan-dev vulkan-validationlayers-dev \
         spirv-tools libglfw3-dev libglm-dev libx11-dev libx11-xcb-dev libxxf86vm-dev libxi-dev
       ```
    - On rpm based systems run: 
        ```
        sudo dnf install gtest-devel vulkan-tools mesa-vulkan-devel vulkan-validation-layers-devel \ 
          spirv-tools glslc glfw-devel glm-devel libXxf86vm-devel
        ``` 
    - If you are **ME** or, for some strange reason trust my administrator-privileged calls, run ```jimara_initialize.py``` from the repository to do the same as above and create a bounch of symbolic links that Linux does not yet need.
4. Build and run Google Test executable with ```Makefile``` inside ```Project/Linux``` directory (output stored in ```__BUILD__``` directory):
    ```
    make clean test
    ```

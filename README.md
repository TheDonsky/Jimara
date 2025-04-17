# Jimara Engine
<p align="center">
    <img width="200" src=https://github.com/TheDonsky/Jimara/blob/main/__Source__/Jimara-EditorExecutable/Jimara.ico>
</p>

##
Jimara is an experimental 3d game engine for desktop platforms (Windows/Linux). 
Core update loop, custom behaviour API, as well as all the basic features requred for development are already there and evolving with time. 
Engine also comes with an editor application for building scenes and manipulating Component hierarchies.
Having said that, the project is not yet fully production-ready, since it lacks some of the proper deployment options that would hide assets from outside applications.

## License
Jimara is published under [MIT license](https://github.com/TheDonsky/Jimara/blob/main/LICENSE). Code comes "as is", you can modify/extend/publish as you wish for any purpose.

## Disclaimer
Since the project is in an experiental stage, external API is still evolving and some changes are expected with future versions.
However, I will try to communicate those changes the best I can.

## Setting up the Engine:

### Windows:
0. Install the latest version of ```python3```;
1. Install the latest version of ```Microsoft VisualStudio 2019/2022```;
2. Install the latest version of ```LunarG Vulkan SDK```;
3. Make full recursive clone of Jimara repository (additional dependencies are included as submodules):
    ```
    git clone --recursive https://github.com/TheDonsky/Jimara.git
    ```
4. ```Run jimara_initialize.py``` from the repository to create symbolic links that make the source visible to MSVS projects;
5. Open ```Project/Windows/MSVS2019/Jimara.sln``` with MSVS2019/2022;
6. Build the solution to generate ```Jimara.lib``` and ```Jimara.dll``` files for each configuration, as well as the optional extensions and ```Editor``` executable (all stored in ```__BUILD__``` directory);
7. Add an environment variable ```JIMARA_REPO``` linking to the path of the engine repository on your machine.


### Linux:
0. Make full recursive clone of Jimara repository (additional dependencies are included as submodules):
    ```
    git clone --recursive https://github.com/TheDonsky/Jimara.git
    ```
1. Install build dependencies for your distribution:
    - On debian based systems run: 
       ```
       sudo apt install libgtest-dev cmake vulkan-tools libvulkan-dev vulkan-validationlayers-dev \
         spirv-tools libglfw3-dev libglm-dev libx11-dev libx11-xcb-dev libxxf86vm-dev libxi-dev \
         python clang freeglut3-dev libopenal-dev libfreetype-dev
       ```
    - On rpm based systems run: 
        ```
        sudo dnf install gtest-devel vulkan-tools mesa-vulkan-devel vulkan-validation-layers-devel \ 
          spirv-tools glslc glfw-devel glm-devel libXxf86vm-devel
        ``` 
    - To perform the same steps in an automated manner, you can choose to run ```jimara_initialize.py``` from the repository.
2. Build ```Jimara.so```, ```Editor``` and extensions by running ```Makefile``` inside ```Project/Linux``` directory (output stored in ```__BUILD__``` directory):
    ```
    make
    ```

## Setting up a project

### Windows:
0. Create empty C++ visual studio project;
1. Configure Windows SDK version to match that of the Engine;
2. Set ```General/Output directory``` to ```$(SolutionDir)\__BUILD__\$(Platform)\$(Configuration)\Game\```;
3. Set ```General/Intermediate directory``` to ```$(SolutionDir)\__BUILD__\Intermediate\$(Platform)\$(Configuration)\Game\```;
4. Set ```General/Configuration type``` to ```Dynamic Library (.dll)```;
5. Set ```General/C++ Language Standard``` to ```/std:C++17```;
6. Set ```General/C Language Standard``` to ```/std:C17```;
7. Set ```Debugging/Command``` to ```$(JIMARA_REPO)\__BUILD__\Windows\Jimara\$(Platform)\$(Configuration)\Jimara-Editor.exe```;
8. Set ```Debugging/Working Directory``` to ```$(OutputPath)\..\```;
9. Set ```'C/C++'/General/Additional Include Directories``` to ```%JIMARA_REPO%/__Source__;%JIMARA_REPO%/Jimara-ThirdParty/glm;%(AdditionalIncludeDirectories)```;
10. Under ```'C/C++'/Code Generation``` enable ```Parallel Code generation```, ```AVX``` and ```Fast floating point model```;
11. Set ```Linker/General/Additional Library Directories``` to ```%JIMARA_REPO%\__BUILD__\Windows\Jimara\$(Platform)\$(Configuration)\;%(AdditionalLibraryDirectories)```;
12. Set ```Linker/Input/Additional Dependencies``` to ```Jimara.lib;Jimara-EditorTools.lib;Jimara-StateMachines.lib;Jimara-StateMachines-Editor.lib;Jimara-GenericInputs.lib;%(AdditionalDependencies)```;
13. Set ```Build Events/Pre-Build Event/Command Line``` to:
    ```
    set jimara_src_dir="%JIMARA_REPO%\__Source__\Jimara"
    set game_src_dir="$(ProjectDir)\"
    
    set shader_intermediate_dir="$(SolutionDir)\__BUILD__\Intermediate\GLSL\$(Configuration)\$(Platform)\LitShaders"
    set shader_output_dir="$(OutDir)Shaders"
    
    python "%JIMARA_REPO%\__Scripts__\jimara_build_shaders.py"  %jimara_src_dir% %game_src_dir% -id %shader_intermediate_dir% -o %shader_output_dir%
    
    set game_type_registry="GAME_NAMESPACE::GAME_PROJECT_NAME_TypeRegistry"
    set game_type_registry_impl=%game_src_dir%\__Generated__\TypeRegistry.impl.h
    python "%JIMARA_REPO%\__Scripts__\jimara_implement_type_registrator.py" %game_src_dir% %game_type_registry% %game_type_registry_impl%
    ```
    (Replace GAME_NAMESPACE::GAME_PROJECT_NAME_TypeRegistry with the correct registry typename)
15. Create 'external' folder for your assets;
16. Create a symbolic link to the assets folder inside ```$(SolutionDir)\__BUILD__\$(Platform)\$(Configuration)``` directory (symlink name HAS TO BE ```Assets```);
    Alternatively, you can set ```Debugging/Command Arguments``` to ```-asset_directory=Path/To/Asset/Directory```;
17. When you build and run, the ```Editor Application``` will open with your game code loaded-in and will have access to your Assets folder.

### Linux:
0. Create a separate directory/repository for your game's code;
1. Copy ```Makefile``` from ```Project/Presets/Linux``` into your project's root directory;
2. Alter ```Makefile``` parameters:
    - Set ```JIMARA_REPO``` to the path of the engine repository on your machine;
    - Set ```GAME_PROJECT_NAME``` to the name of your project;
    - Set ```GAME_NAMESPACE``` to the main namespace you'll be using for your game code (Including ```::``` at the end; can be left empty if there's no namespace);
    - Optionally change ```GAME_SOURCE_DIR``` to relative path to the source folder (Defaults to ```./src```);
    - Optionally change ```GAME_ASSETS_DIR``` to relative path to the assets folder (Defaults to ```./Assets```);
    - Optionally change ```GAME_BUILD_DIR``` to the directory you want to store built shaders and static objects in (Defaults to ```./build-editor```);
    - Optionally change ```GAME_INTERMEDIATE_DIR``` to a directory for intermediate build files (Defaults to ```./build-intermediate```).
3. Use make to build, run the editor or clear:
   ```
   make build
   make build-and-run
   make clean
   ```

### For all operating systems:
0. Within the main source directory, add two files:
    - ```TypeRegistry.h```:
      ```cpp
      #pragma once
      #include <Jimara/Core/TypeRegistration/TypeRegistration.h>
      namespace GAME_NAMESPACE { // Note, that GAME_NAMESPACE has to be replaced with the correct value from the makefile
          // Note, that GAME_PROJECT_NAME has to be replaced with the correct value from the makefile
          JIMARA_REGISTER_TYPE(GAME_NAMESPACE::GAME_PROJECT_NAME_TypeRegistry);
          #define TypeRegistry_TMP_DLL_EXPORT_MACRO
          /// <summary> Type registry for our game </summary>
          JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(GAME_PROJECT_NAME_TypeRegistry, TypeRegistry_TMP_DLL_EXPORT_MACRO);
          #undef TypeRegistry_TMP_DLL_EXPORT_MACRO
      }
      ```
    - ```TypeRegistry.cpp```:
      ```cpp
      #include "__Generated__/TypeRegistry.impl.h"
      namespace GAME_NAMESPACE {
          static Jimara::Reference<GAME_PROJECT_NAME_TypeRegistry> registryInstance = nullptr;
    
          inline static void GAME_PROJECT_NAME_OnLibraryLoad() {
              registryInstance = GAME_PROJECT_NAME_TypeRegistry::Instance();
          }
    
          inline static void GAME_PROJECT_NAME_OnLibraryUnload() {
              registryInstance = nullptr;
          }
      }
    
      extern "C" {
      #ifdef _WIN32
          #include <windows.h>
          BOOL WINAPI DllMain(_In_ HINSTANCE, _In_ DWORD fdwReason, _In_ LPVOID) {
              if (fdwReason == DLL_PROCESS_ATTACH) GAME_NAMESPACE::GAME_PROJECT_NAME_OnLibraryLoad();
              else if (fdwReason == DLL_PROCESS_DETACH) GAME_NAMESPACE::GAME_PROJECT_NAME_OnLibraryUnload();
              return TRUE;
          }
      #else
          __attribute__((constructor)) static void DllMain() {
              GAME_NAMESPACE::GAME_PROJECT_NAME_OnLibraryLoad();
          }
          __attribute__((destructor)) static void OnStaticObjectUnload() {
              GAME_NAMESPACE::GAME_PROJECT_NAME_OnLibraryUnload();
          }
      #endif
      }
      ```
   These will enable the engine to see custom classes, specific to your project.
1. Once the boilerplate above is done, you will be able to add stuff like custom Components like this:
    ```cpp
    #include <Jimara/Components/Transform.h>

    namespace SomeNamespace {
        JIMARA_REGISTER_TYPE(AnyNamespace::CustomComponent);

        class CustomComponent : public virtual Jimara::Component {
        public:
            CustomComponent(Jimara::Component* parent, const std::string_view& name = "")
                : Jimara::Component(parent, name) {}
            virtual ~CustomComponent() {}

            // Cusom logic would go here. For example:
            // 0. You can override GetFields() method to expose custom variables to the Editor UI, save/load and undo actions;
            // 1. If you want to do something on each update cycle,
            //    you can inherit Jimara::SceneContext::UpdatingComponent and implement it's Update() method;
            // 2. For handling Component lifecycle events you can override OnComponentInitialized(), OnComponentStart(),
            //    OnComponentEnabled(), OnComponentDisabled() and OnComponentDestroyed() methods.
            // 3. Feel free to read through inline doumentation for Component and SceneContext for further details.
            //    There are more plces you can hook into if you want custom renderers and alike,
            //    but you can start with the simple stuff like this and explore the rest when you need to :D.
        };
    }

    namespace Jimara {
        template<> inline void TypeIdDetails::GetParentTypesOf<AnyNamespace::CustomComponent>(const Callback<TypeId>& report) {
            // Current version recommends reporting parent types through this function; Down the line, need for this function will likely be eleminated.
        	report(TypeId::Of<Component>());
        }
        template<> void TypeIdDetails::GetTypeAttributesOf<AnyNamespace::CustomComponent>(const Callback<const Object*>& report) {
            // This factory object adds your component type to AddComponent menu and enables saving/loading the instances.
            static const Reference<ComponentFactory> factory = ComponentFactory::Create<AnyNamespace::CustomComponent>(
            	"Name", "Context menu path", "Hint");
            report(factory);
        }
    }
    ```
    Note that you can have separated headers and cpp files for your custom classes, but in order for the type registry to access it, header HAS TO exist.
 
  

# Setting Up Environment (no admin needed)
## Installing Vscode (and extensions)
- 1. install VsCode latest version: https://code.visualstudio.com/download

- 2. follow method to install C/C++ Extension via this link: https://code.visualstudio.com/docs/languages/cpp#_example-install-mingwx64-on-windows
OR 
- Use the follwoing:
    - 1. Open VS Code.
    - 2. Select the Extensions view icon on the Activity Bar or use the keyboard shortcut (`Ctrl+Shift+X`).
    - 3. Search for 'C++'.
    - 4. Select Install.
- 3. Check if a compiler is already available:
    - 1. Open a new VS Code terminal window using (`Ctrl+Shift+`).

    - 2. Use the following command to check for the GCC compiler g++:
```bash
g++ --version
```
    OR this command for the Clang compiler clang:
```bash
clang --version
```

   - 3. The output should show you the compiler version and details. If neither are found, make sure your compiler executable is in your platform path (%PATH on Windows, $PATH on Linux and macOS) so that the C/C++ extension can find it. Otherwise, use the instructions in the section below to install a compiler.

## Installing Compiler -> MinGW64
- 4. If non -> Install a compiler
For windows:
   - 1. Download using this direct link to the MinGW installer: https://github.com/msys2/msys2-installer/releases/download/2024-12-08/msys2-x86_64-20241208.exe

   - 2. Run the installer and follow the steps of the installation wizard. Note, MSYS2 requires 64 bit Windows 8.1 or newer.

   - 3. In the wizard, choose your desired Installation Folder. Record this directory for later. In most cases, the recommended directory is acceptable. The same applies when you get to setting the start menu shortcuts step. When complete, ensure the **Run MSYS2 now** box is checked and select **Finish**. A MSYS2 terminal window will then automatically open. NOTE: An **MSYS** terminal is opened _NOT_ **MSYS2 MinGW64**
   - Later, you are going to install the same toolchain in the **MSYS2 MinGW64** terminal when installing the `opengl` libs.

   - 4. In this terminal, install the MinGW-w64 toolchain by running the following command:
```shell
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
```
   - 5. A list of available packages will be displayed

   - 6. Accept the default number of packages in the toolchain group by pressing `Enter`.

   - 7. Enter Y when prompted whether to proceed with the installation.

   - 8. Add the path of your MinGW-w64 bin folder to the Windows PATH environment variable by using the following steps:
     - 1. In the Windows search bar, type **Settings** to open your Windows Settings.

     - 2. Search for **Edit environment variables for your account**.

     - 3. In your **User variables**, select the Path variable and then select **Edit**.

     - 4. Select **New** and add the MinGW-w64 destination folder you recorded during the installation process to the list. If you selected the default installation steps, the path is: C:\msys64\ucrt64\bin.

     - 5. Select **OK**, and then select **OK** again in the **Environment Variables** window to update the PATH environment variable. You have to reopen any console windows for the updated PATH environment variable to be available.

   - 9. Check that your MinGW-w64 tools are correctly installed and available, open a new Command Prompt and type:
```bash
gcc --version
g++ --version
gdb --version
```
   You should see output that states which versions of GCC, g++ and GDB you have installed. If this is not the case, make sure your PATH entry matches the Mingw-w64 binary location where the compiler tools are located or reference the troubleshooting section(https://code.visualstudio.com/docs/cpp/config-mingw#__check-your-mingw-installation).

# 1. Install OpenGL + freeGLUT in MSYS2 (no admin needed)

## Prerequisities
Before installing the OpenGL libraries, ensure that you are running the correct MSYS2 terminal and have the MinGW-w64 toolchain installed and updated.

1. **Open the MinGW64 terminal**
- Launch the "MSYS2 MinGW 64-bit" application from your Start menu. The prompt should indicate `MINGW64`. Always use **MSYS2 MinGW64** _NOT_ **MSYS** terminal

2. **Update your package databases and core packages**
- By running the following commands in the terminal, restarting the terminal as instructed if prompted to do so:
```bash
pacman -Syu
pacman -Su
```

3. **Install the MinGW-w64 GCC toolchain** (if you haven't already)
```bash
pacman -S mingw-w64-x86_64-toolchain
```
Accept the default packages by pressing Enter and then type `Y` to proceed with the installation.

## Installing OpenGL Libraries
The core `opengl32.dll` library is typically provided by your graphics card drivers on Windows, so you only need to install the development libraries for additional functionality like creating windows and managing extensions.

Run the following commands in your MinGW64 terminal to install `freeglut`, `glew`, and optionally `glfw`:
  - 1. **FreeGLUT**
    - An open-source alternative to the GLUT library for window management.
```bash
pacman -S mingw-w64-x86_64-freeglut
```

  - 2. **GLEW** (OpenGL Extension Wrangler Library)
    - Helps in managing and loading OpenGL extensions at runtime.
```bash
pacman -S mingw-w64-x86_64-glew
```

  - 3. **GLFW** (Optional)
    - Another popular library for window and input management, often preferred over GLUT.
```bash
pacman -S mingw-w64-x86_64-glfw
```

## Compiling and Linking Your Code
When compiling your OpenGL program with `g++`, you need to link against the installed libraries. For a program using FreeGLUT, GLEW, and basic OpenGL, you would use a command similar to this:
```bash
g++ your_file.cpp -o your_program.exe -lfreeglut -lglew32 -lopengl32 -lglu32
```

If using  GLFW and GLEW do
```bash
gcc -Wall -DUSEGLEW -o foo foo.c -lglfw3 -lglew32 -lglu32 -lopengl32 -lm
```
using GLEW is required on some systems and not on others. So I use the compiler flag `-DUSEGLEW` to conditionally compile in GLEW when needed.

NOTE: Make sure the Path for the **User variables** of the **Edit environment variables for your account**, is _C:/.../msys64/mingw64/bin_ NOT _C:/.../msys64/ucrt64/bin_. This is because we are using the mingw64 environment when building and since that is where you installed 

# When Running an App
Make sure when you click the run icon, you Choose **C/C++: g++.exe build and debug active file** from the list of detected compilers on your system.

You are only prompted to choose a compiler the first time you run `your_app.cpp`. This compiler becomes "default" compiler set in your tasks.json file.


# 2. Using CMake (no admin needed)
**NOTE: IF you are a pro**, and you are used to cmake then use it, otherwise stick to the *1.*
## Installing CMake and Mingw64
1. Follow steps above for installing `MinGW-w64`
2. > Install CMake: https://cmake.org/download/ (Download binaries and extract to folder of choice; usually install in C:/user/your_account_name/CMake)
3. Add the path of your `CMake` bin folder to the Windows PATH environment variable just as you did for `MinGW-w64`.
4. Open Command Prompt and execute `cmake --version` to see the version.

## Installing FreeGLUT and GLEW
### Installing FreeGLUT
5. > Download the latest Freeglut from here: https://freeglut.sourceforge.net/ under **Stable Releases**. It must be a tar.gz file so you might have to extract it twice using 7-Zip.
6. Go to the freeglut folder (it should contain CMakeLists.txt file) and open the Command Prompt at this location to execute the following command:
```bash
cmake -G "MinGW Makefiles" -S . -B . -DCMAKE_INSTALL_PREFIX=C:\mingw64\x86_64-w64-mingw32
```
- Use `cmake --help` to learn about the different options (-G, -S, -B, -D)
- **CMAKE_INSTALL_PREFIX** is set to the location where we want to install the Freeglut library files so that our future OpenGL code can use its headers.
7. > Next execute: `mingw32-make all`
- **mingw32-make** is part of the _C:\mingw64\bin_ suite
Note: You might get a lot of warnings but don’t panic.
8. > Finally execute: `mingw32-make install` so that the include headers, lib and bin files are copied to the corresponding folders of **CMAKE_INSTALL_PREFIX** (which is *C:\mingw64\x86_64-w64-mingw32*)

### Installing GLEW
- The steps to install GLEW are similar to Step 1 (Installing FreeGLUT)
9. Download the latest GLEW from here: http://glew.sourceforge.net/index.html (use binaries)
10. Go the glew folder and search for the location of **CMakeLists.txt**
- It’s here: _glew-2.1.0\build\cmake\CMakeLists.txt_
- Open the Command Prompt at this location and execute the following command:
```bash
cmake -G "MinGW Makefiles" -S . -B . -DCMAKE_INSTALL_PREFIX=C:\mingw64\x86_64-w64-mingw32
```
11. > Next execute: `mingw32-make all`
12. > Finally execute: `mingw32-make install`

NOW you can write opengl code
Here is a quick example:
- Run the following code based on the file in `src\triangle.cpp`
```bash
g++ triangle.cpp -o triangle -lopengl32 -lglew32 -lfreeglut -lglu32
```
- It will create the executable **triangle.exe** which you can run from the Command Prompt or open it from its folder as any application exe.

## NOTE:
- If you get errors such as “No such file or directory” for an imported Header file, it means that header file is not in the search paths of g++/gcc. You can check where the g++ tool is searching for header files by running with `-v` option as follows:
```bash
g++ -v triangle.cpp -o triangle -lopengl32 -lglew32 -lfreeglut -lglu32
```

Make sure the headers are available in those search paths by copying them or reinstalling by changing the install location (*CMAKE_INSTALL_PREFIX* flag)

Similarly, if you face *.dll* not found errors then make sure the dll’s location is added to the Path of the System Environment Variables.

## Installing GLFW (Optional)
1. > Download the latest GLFW source package from here: https://www.glfw.org/download.html (binaries)
2. Extract and Open Command Prompt, go to its folder and execute the following:
```bash
cmake -G "MinGW Makefiles" -S . -B . -DCMAKE_INSTALL_PREFIX=C:\mingw64\x86_64-w64-mingw32
```
3. Next execute: `mingw32-make all`
4. Finally execute: `mingw32-make install`

For testing:
```bash
g++ triangle.cpp -o triangle -lopengl32 -lglew32 -lfreeglut -lglu32
```


# 3. Common Problems (and fixes)

### “freeglut.dll missing”
- Fix: `export PATH=$PATH:/c/msys64/mingw64/bin` '(that is; path in user var in environ)
- Or copy: `C:\msys64\mingw64\bin\freeglut.dll` into your project folder.
- or Make sure your `include` path and `compiler` path have been defined in VSCode C/C++ extension(access it via `ctrl + shift + P`).
- Or look at the below solution for *Wrong Terminal*, as sometimes that could be the error.

### Wrong terminal
- Always use **MSYS2 MinGW64** _NOT_ **MSYS**
- If you did not install `mingw-w64-x86_64-toolchain` in **MSYS2 MinGW64** terminal and you are using the one you installed when first installing mingw64 in **UCRT64** terminal then you are going to have to run your code like this:
```bash
g++ "src/main.cpp" -o app.exe -I"C:/Users/your_account_name/AppData/Local/msys64/mingw64/include" -L"C:/Users/your_account_name/AppData/Local/msys64/mingw64/lib" -lfreeglut -lglew32 -lopengl32 -lglu32
```
- Make sure to adjust the location of include and bin folders of *msys64* based on where you put it.
- But this can cause errors in future due to cross compatibility. So, please(if you have internet data) install the *toolchain* in the correct terminal.
- **TOTAL INTERNET DATA REQUIRED**: ~ 450-700MB
- **TOTAL INSTALL SPACE REQUIRED**: ~ 2.7-3.0GB

### IntelliSense errors in VS Code

Add to `.vscode/c_cpp_properties.json`:

```json
"includePath": [
    "C:/.../msys64/mingw64/include"
]
```

# Final
Stick to **1.** as it is less prone to errors!!!
Thanks for reading my guide and hope it helped you set up the OpenGL environment.
(Tutorial Link(CMake): https://medium.com/@bhargav.chippada/how-to-setup-opengl-on-mingw-w64-in-windows-10-64-bits-b77f350cea7e)
@credit -> Bhargav Chippada
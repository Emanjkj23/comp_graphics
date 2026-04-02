2. Generate GLAD Files
GLAD is a loader-generator, not a traditional pre-compiled library. You must generate the source files for your specific OpenGL version: 
Go to the GLAD Web Service.
Language: Select C/C++.
API: Select gl and your desired version (e.g., 3.3 or higher).
Profile: Select Core.
Options: Ensure "Generate a loader" is checked.
Click Generate and download the resulting glad.zip. 

3. Integrate GLAD into your MSYS2 Project 
Extract the glad.zip file. You will see two main directories: include/ and src/. 
Include Headers: Move the contents of include/glad and include/KHR to your project's include directory (e.g., C:/msys64/mingw64/include/ for a global install, or keep them local to your project).
Source File: Copy src/glad.c into your project's source folder. 

4. Compilation and Linking
When compiling with g++, you must include the glad.c file as a source and link against the Windows OpenGL library (-lopengl32).
Basic Compilation Command:
bash
g++ main.cpp glad.c -o my_opengl_app -I./include -lopengl32 -lglfw3 -lgdi32
glad.c: Must be compiled along with your main code.
-I./include: Tells the compiler where to find the glad/glad.h header.
-lopengl32: Links the system OpenGL library.
g++ "./vortex/src/main.cpp" "./vortex/src/glad.c" -o vortex.exe -lopengl32 -lglfw3 -lgdi32
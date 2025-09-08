
// HW 0 - Moving Square - Updated for SDL3
//Starter code for the first homework assignment.
//This code assumes SDL3 and OpenGL are both properly installed on your system

// clang++ -o square solution.cpp -xc glad/glad.c -F ~/Library/Frameworks -framework SDL3 -Wl,-rpath,$HOME/Library/Frameworks

#include "glad/glad.h"  //Include order can matter here
#if defined(__APPLE__) || defined(__linux__)
 #include <SDL3/SDL.h>
 #include <SDL3/SDL_opengl.h>
#else
 #include <SDL.h>
 #include <SDL_opengl.h>
#endif
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include "pga.h"
using namespace std;


// Name of image texture
string textureName = "test.ppm";

// Screen size
int screen_width = 800;
int screen_height = 800;

// Globals to store the state of the square (position, width, and angle)
Point2D rect_pos = Point2D(0,0);
float rect_scale = 1;
float rect_angle = 0;

float vertices[] = {  // The function updateVertices() changes these values to match p1,p2,p3,p4
//  X     Y     R     G     B     U    V
  0.3f,  0.3f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top right
  0.3f, -0.3f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
  -0.3f,  0.3f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top left
  -0.3f, -0.3f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom left
};

Point2D screen_origin = Point2D(0,0);

// Be careful with the winding order of the points (e.g clockwise vs counterclockwise)
Point2D init_p1 = Point2D(vertices[21],vertices[22]);
Point2D init_p2 = Point2D(vertices[7],vertices[8]);
Point2D init_p3 = Point2D(vertices[0],vertices[1]);
Point2D init_p4 = Point2D(vertices[14],vertices[15]);

Point2D p1 = init_p1, p2 = init_p2, p3 = init_p3, p4 = init_p4;

// Build the lines that make up the edges of the square/rectangle
// The lines are used for later geometric calculations such as intersection tests
Line2D l1 = vee(p1,p2).normalized();
Line2D l2 = vee(p2,p3).normalized();
Line2D l3 = vee(p3,p4).normalized();
Line2D l4 = vee(p4,p1).normalized();

// Helper variables for mouse interaction
Point2D clicked_pos;
Point2D clicked_mouse;
float clicked_angle, clicked_size;

void mouseClicked(float mx, float my); // Called whenever mouse is pressed down
void mouseDragged(float mx, float my); // Called every time mouse moves while button is down
void updateVertices();

bool do_translate = false;
bool do_rotate = false;
bool do_scale = false;

//TODO: Read from ASCII (P3) PPM files properly
//Note: Reference/output variables img_w and img_h used to return image width and height
unsigned char* loadImage(int& img_w, int& img_h){

   // Open the texture image file
   ifstream ppmFile;
   ppmFile.open(textureName.c_str());
   if (!ppmFile){
      printf("ERROR: Texture file '%s' not found.\n",textureName.c_str());
      exit(1);
   }

   // Check that this is an ASCII (P3) PPM file (i.e the first line is "P3")
   string PPM_style;
   ppmFile >> PPM_style; // Read the first line of the file/header
   if (PPM_style != "P3") {
      printf("ERROR: PPM Type number is %s. Not an ASCII (P3) PPM file!\n",PPM_style.c_str());
      exit(1);
   }

   // Read in the image dimensions (width, height)
   ppmFile >> img_w >> img_h;
   unsigned char* img_data = new unsigned char[4*img_w*img_h];

   // Check that the 3rd line of the header is 255 (ie., this is an 8-bit per pixel image)
   int maximum;
   ppmFile >> maximum;
   if (maximum != 255) {
      printf("ERROR: Maximum size is (%d) not 255.\n",maximum);
      exit(1);
   }

   // TODO: This loop reads fake data. Replace it with code to read in the actual pixel values from the file
   // TODO:    ... see the project description for a hint on how to do this
   for (int i = 0; i < img_h; i++){
      float fi = i/(float)img_h;
      for (int j = 0; j < img_w; j++){
         float fj = j/(float)img_w;
         img_data[i*img_w*4 + j*4] = 50;  // Red 
         img_data[i*img_w*4 + j*4 + 1] = fj*150; // Green
         img_data[i*img_w*4 + j*4 + 2] = fi*250; // Blue
         img_data[i*img_w*4 + j*4 + 3] = 255; // Alpha
      }
   }
   return img_data;
}

//TODO: Choose between translation, rotation, and scaling based on where the mouse was when the user clicked
// In this temporary example code, I always assume a translation operation. Fix this to switch between the 3 operations
// of translate, rotate, and scale based on where the mouse was when the user clicked.

void mouseClicked(float m_x, float m_y){
   printf("Clicked at %f, %f\n",m_x,m_y);

   // We may need to know where both the mouse and the rectangle were at the moment the mouse was clicked
   // so we store these values in global variables for later use
   clicked_mouse = Point2D(m_x,m_y);
   clicked_pos = rect_pos;
   clicked_angle = rect_angle;
   clicked_size = rect_scale;

   // Helper global variables to determine if we are translating, rotating, or scaling
   // You will need to change this logic to choose between the 3 operations based on where the mouse was clicked
   do_translate = true; // FIX ME - Should only be true if mouse clicked on the rectangle in the right region
   do_rotate = false;
   do_scale = false;
}

// TODO 1: Update the position, rotation angle, or scale of the rectangle based on how far the mouse has moved
//        I've implemented translation for you as an example. You need to implement rotation and scaling

// TODO 2: Notice how smooth the rectangle moves when you drag it (e.,g there are no "jumps" or stutters when you click)
//       Try to make your implementation of rotation and scaling also be as smooth

void mouseDragged(float m_x, float m_y){
   Point2D cur_mouse = Point2D(m_x,m_y);

   if (do_translate){
      // Compute the position, rect_pos, By first finding the displacement vector of the mouse since the initial click, then move the rectangle by the same amount
      Dir2D disp = cur_mouse-clicked_mouse;
      rect_pos = clicked_pos+disp;
   }

   if (do_scale){
      // Compute the new size, rect_scale, based on how far the mouse has moved around the rectangle since the initial click
      rect_scale = clicked_size; // FIX ME - Replace this line with your code to compute the new scale
   }

   if (do_rotate){
      // Compute the new angle, rect_angle, based on how far the mouse has moved around the rectangle since the initial click
      rect_angle = clicked_angle; // FIX ME - Replace this line with your code to compute the new angle
   }

   // Assuming the angle (rect_angle), position (rect_pos), and scale (rect_scale) of the rectangle have been updated correctly above
   // we now need to update the actual positions of the 4 corners of the rectangle (p1,p2,p3,p4) in order to rotate, translate, and scale the shape correctly
   // I've wrote all the code for you here. You shouldn't need to change the rest of this function, but
   // you should read through it to understand how it works since it uses the geometric algebra concepts that we will be learning about in class!
   
   Motor2D translate, rotate;

   Dir2D disp = rect_pos - screen_origin;
   translate = Translator2D(disp);
   rotate = Rotator2D(rect_angle, rect_pos);

   Motor2D movement = rotate*translate;

   // Scale the rectangle points (this is done relative to the screen_origin)
   p1 = init_p1.scale(rect_scale);
   p2 = init_p2.scale(rect_scale);
   p3 = init_p3.scale(rect_scale);
   p4 = init_p4.scale(rect_scale);

   // Use the motor "movement" to rotate and translate the rectangle to its new position
   p1 = transform(p1,movement);
   p2 = transform(p2,movement);
   p3 = transform(p3,movement);
   p4 = transform(p4,movement);

   // Update the lines that make up the edges of the rectangle based on new corner points (lines are used for geometric calculations above)
   l1 = vee(p1,p2).normalized();
   l2 = vee(p2,p3).normalized();
   l3 = vee(p3,p4).normalized();
   l4 = vee(p4,p1).normalized();

   updateVertices();
}

// You shouldn't need to change this function.
// It updates the displayed vertices to match the current positions of p1, p2, p3, p4
void updateVertices(){
   vertices[0] = p3.x; vertices[1] = p3.y;     // Top right x & y
   vertices[7] = p2.x; vertices[8] = p2.y;     // Bottom right x & y
   vertices[14] = p4.x; vertices[15] = p4.y;   // Top left x & y
   vertices[21] = p1.x; vertices[22] = p1.y; // Bottom left x & y
}

// TODO: Implement this function to reset the rectangle to its initial position, size, and angle
void r_keyPressed(){
   cout << "The 'r' key was pressed" <<endl;
   // FIX ME - You should reset the rectangle position, size, and angle to its initial values, and update the vertices
}

// ----------------------- //
// Below this line is OpenGL specific code
// We'll cover OpenGL in detail around Week 9
// But you should poke around a bit right now to start to see how it works
// I've annotated some part with "TODO: Test ..." check those out!
// ----------------------- //

// Shader sources
const GLchar* vertexSource =
   "#version 150 core\n" // Shader Version (150 --> OpenGL 3.2)
   "in vec2 position; in vec3 inColor; in vec2 inTexcoord;" // Input vertex data (position, color, and texture coordinates)
   "out vec3 Color; out vec2 texcoord;" // Output data to fragment shader (color and texture coordinates)
   "void main() { Color = inColor; gl_Position = vec4(position, 0.0, 1.0); texcoord = inTexcoord; }"; // Position is a vec4 (x,y,z,w) so z=0 and w=1
const GLchar* fragmentSource =
   "#version 150 core\n"
   "uniform sampler2D tex0; in vec2 texcoord; out vec3 outColor;" // Input texture and texture coordinates from vertex shader, output is pixel color
   "void main() { outColor = texture(tex0, texcoord).rgb; }"; // Set pixel color based on texture color at the texture coordinates

int main(int argc, char *argv[]){

   //Initialize Graphics (for OpenGL)
   if (!SDL_Init(SDL_INIT_VIDEO)) {
      printf("SDL_Init Error: %s\n", SDL_GetError());
      return 1;
   }

   //Ask SDL to get a fairly recent version of OpenGL (3.2 or greater)
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

   //Create a window (title, width, height, flags)
   //TODO: TEST your understanding: Try changing the title of the window to something more personalized.
   SDL_Window* window = SDL_CreateWindow("My OpenGL Program (SDL3 + OpenGL 3.2)", screen_width, screen_height, SDL_WINDOW_OPENGL);
   if (!window) {
      printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
   }

   // The above window cannot be resized which makes some code slightly easier.
   // If you want to make it resizable, you can add the flag: | SDL_WINDOW_RESIZABLE ... but be sure to update your to support the size changing!
   // SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screen_width, screen_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
   

   // If we allow non-square screens, we need to take the aspect ratio into account when we draw things in updateVertices()
   // float aspect = screen_width/(float)screen_height; //aspect ratio (needs to be updated if the window is resized) //I make this a global variable for simplicity

   updateVertices();

   // Create an OpenGL context associated with the window (the context is like a "state" for OpenGL)
   SDL_GLContext context = SDL_GL_CreateContext(window);
   if (!context) {
      printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
   }

   // OpenGL functions are loaded from the driver using glad library
   if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)){
      printf("ERROR: Failed to initialize OpenGL context.\n");
      return -1;
   }
   printf("OpenGL loaded\n");
   printf("Vendor:   %s\n", glGetString(GL_VENDOR));
   printf("Renderer: %s\n", glGetString(GL_RENDERER));
   printf("Version:  %s\n", glGetString(GL_VERSION));

   // Allocate and assign a texture name (tex0) to an OpenGL texture object
   GLuint tex0; glGenTextures(1, &tex0);
   glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex0);

   // What happens when you read a texture pixel outside the range [0,1]
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, GL_REPEAT, etc.
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // GL_NEAREST, GL_LINEAR, etc.
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // GL_NEAREST, GL_LINEAR, etc.
   // TODO: TEST your understanding: Try changing GL_LINEAR to GL_NEAREST on the 4x4 checkerboard texture. What is happening? Also change the wrap modes to GL_CLAMP_TO_EDGE and see what happens.

   // Load texture data from a file
   int img_w, img_h;
   unsigned char* img_data = loadImage(img_w,img_h);
   printf("Loaded image %s (width=%d, height=%d)\n",textureName.c_str(),img_w,img_h);
   // memset(img_data,0,4*img_w*img_h); // Loads all zeroes (black) for testing

   // Assign the image to the OpenGL texture object on the GPU's memory
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_w, img_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
   glGenerateMipmap(GL_TEXTURE_2D);

   // Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
   GLuint vao; 
   glGenVertexArrays(1, &vao); 
   glBindVertexArray(vao);

   // Allocate a Vertex Buffer Objects (VBOs) to store geometry (vertex data) on the GPU such as vertex position and color.
   // VBOs are memory on the GPU that can store information about each vertex.
   GLuint vbo; glGenBuffers(1, &vbo); // Create 1 buffer called 'vbo'
   glBindBuffer(GL_ARRAY_BUFFER, vbo); // Set the buffer to be the active buffer (Only 1 buffer can be active at a time, here we put all the data in one buffer)
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW); // glBufferData uploads the data to the GPU
   // GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW means we may change the store data repeatedly (e.g., for animation) like we do in the main loop below
   //GL_STATIC_DRAW vs GL_DYNAMIC_DRAW vs GL_STREAM_DRAW change the type of GPU memory that is allocated and can affect performance

   // Load the vertex shader to the GPU (Vertex shaders process each vertex to compute the position and color of each vertex)
   GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); 
   glShaderSource(vertexShader, 1, &vertexSource, NULL); 
   glCompileShader(vertexShader);

   // Debugging: Check if the shader compiled properly
   GLint status;
   glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
   if (status != GL_TRUE) {
      char buffer[512];
      glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
      printf("Vertex Shader Compile Error: %s\n",buffer);
   }

   // Load the fragment shader to the GPU (Fragment shaders process each pixel of a triangle to compute the color of each pixel) 
   //    ... The GPU interpolates values across the triangle vertices to compute pixel values
   GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); 
   glShaderSource(fragmentShader, 1, &fragmentSource, NULL); 
   glCompileShader(fragmentShader);

   // Debugging: Check if the shader compiled properly
   glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
   if (status != GL_TRUE) {
      char buffer[512];
      glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
      printf("Fragment Shader Compile Error: %s\n",buffer);
   }

   // Link the vertex and fragment shaders together into a shader program
   GLuint shaderProgram = glCreateProgram();
   glAttachShader(shaderProgram, vertexShader); 
   glAttachShader(shaderProgram, fragmentShader);
   glBindFragDataLocation(shaderProgram, 0, "outColor"); // Set output variable that gets displayed on screen/framebuffer
   glLinkProgram(shaderProgram); // Run the linker (after the compile step)

   glUseProgram(shaderProgram); // Set the active shader (only one can be used at a time)

   // Tell OpenGL how to set fragment shader input variables (in vec3 Color) from the vertex data array.
   // Our vertex data is stored in "vbo" and contains (x,y,r,g,b,u,v) values at each vertex.
   GLint posAttrib = glGetAttribLocation(shaderProgram, "position"); // x,y = position -- a vec of 2 floats
   //                    attribute, # of values, type, normalized?, stride, offset (in bytes)
   glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), 0);
   glEnableVertexAttribArray(posAttrib); // Enable the attribute and bind to the current VBO

   GLint colAttrib = glGetAttribLocation(shaderProgram, "inColor"); // r,g,b = color -- a vec of 3 floats
   glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(2*sizeof(float)));
   glEnableVertexAttribArray(colAttrib);

   GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord"); // u,v = texture coordinates -- a vec of 2 floats
   glEnableVertexAttribArray(texAttrib);
   glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(5*sizeof(float)));

   // Status booleans that update based on user input
   bool done = false;
   bool mouse_dragging = false;
   bool fullscreen = false;

   // Event Loop (Loop forever processing each event as it arrives)
   while (!done){
      SDL_Event event;
      while (SDL_PollEvent(&event)){ // Process events (keyboard, mouse, window, etc) that have happened since the last time we checked for events
         switch (event.type) {
            case SDL_EVENT_QUIT:
               done = true;
               break;

            case SDL_EVENT_KEY_UP:
               // List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can use these in the if statement below to add more keyboard commands
               // Unlike keycodes, Scancodes are like keycodes but represent the physical location of the key on a keyboard, so they are the same across different keyboard layouts/languages (e.g., European vs US keyboards)
               // if (event.key.scancode == SDL_SCANCODE_Q) ... // Do something when the key in the "Q" location is pressed (e.g., Q on US keyboards, A on French keyboards, etc., but always the same physical key location of the top left)
               // if (event.key.key == SDLK_q) ... // Do something when the "q" key is pressed wherever it may be in the current keyboard layout (different for French vs. US keyboards, etc.)
               if (event.key.key == SDLK_ESCAPE) done = true;
               if (event.key.scancode == SDL_SCANCODE_F) done = true; 
               if (event.key.key == SDLK_F) {
                  fullscreen = !fullscreen;
                  SDL_SetWindowFullscreen(window, fullscreen);
               }
               if (event.key.key == SDLK_R) r_keyPressed();
               break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
               if (event.button.button == SDL_BUTTON_LEFT) {
                  // Convert the mouse coordinates from window pixels to normalized coordinates (i.e. x and y values between -1 and +1)
                  float mouse_x = 2*event.button.x/(float)screen_width - 1;
                  float mouse_y = 1-2*event.button.y/(float)screen_height;
                  mouseClicked(mouse_x, mouse_y);
                  mouse_dragging = true;
               }
               break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
               if (event.button.button == SDL_BUTTON_LEFT) {
                  mouse_dragging = false;
               }
               break;

            case SDL_EVENT_MOUSE_MOTION:
               float mouse_x = 2*event.motion.x/(float)screen_width - 1;
               float mouse_y = 1-2*event.motion.y/(float)screen_height;
               if (mouse_dragging) {
                  mouseDragged(mouse_x, mouse_y);
               }
               break;
         }
      }

      // Add the vertices to the vertex buffer (VBO) each frame since they may change every frame
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

      // Clear the screen to grey
      glClearColor(0.6f, 0.6, 0.6f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Draw 4 vertices (2 triangles) to make up the rectangle
      //TODO: TEST your understanding: What shape do you expect to see if you change the above 4 to a 3? Guess, then try it!
      // TODO: TEST   ... What if you call glDrawArrays(GL_TRIANGLE_STRIP, 1, 4) -- what do you expect to change?

      SDL_GL_SwapWindow(window); // Double buffering (draw to back buffer while front buffer is being displayed, then swap)
   }

   delete [] img_data;
   glDeleteProgram(shaderProgram);
   glDeleteShader(fragmentShader);
   glDeleteShader(vertexShader);
   glDeleteBuffers(1, &vbo);
   glDeleteVertexArrays(1, &vao);

   SDL_GL_DestroyContext(context);
   SDL_DestroyWindow(window);
   SDL_Quit();
   return 0;
}

//Set the global scene parameter variables
//TODO: Set the scene parameters based on the values in the scene file

#ifndef PARSE_VEC3_H
#define PARSE_VEC3_H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <string>

//Camera & Scene Parameters (Global Variables)
//Here we set default values, override them in parseSceneFile()

//Image Parameters
int img_width = 800, img_height = 600;
std::string imgName = "raytraced.png";

//Camera Parameters
vec3 eye = vec3(0,0,0); 
vec3 forward = vec3(0,0,-1).normalized();
vec3 up = vec3(0,1,0).normalized();
vec3 right;
float halfAngleVFOV = 35; 

//Scene (Sphere) Parameters
vec3 spherePos = vec3(0,0,2);
float sphereRadius = 1; 

void parseSceneFile(std::string fileName){
  //TODO: Override the default values with new data from the file "fileName"
  std::ifstream file(fileName);
  if(!file.is_open()){
    std::cerr << "Error: Could not open file " << fileName << std::endl;
    return;
  }

  std::string line;
  while(std::getline(file, line)){
    if(line.empty() || line[0] == '#') continue; // skip comments/empty lines

    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;

    if(cmd == "sphere:"){
      ss >> spherePos.x >> spherePos.y >> spherePos.z >> sphereRadius;
    }
    else if(cmd == "image_resolution:"){
      ss >> img_width >> img_height;
    }
    else if(cmd == "output_image:"){
      ss >> imgName;
    }
    else if(cmd == "camera_pos:"){
      ss >> eye.x >> eye.y >> eye.z;
    }
    else if(cmd == "camera_fwd:"){
      ss >> forward.x >> forward.y >> forward.z;
    }
    else if(cmd == "camera_up:"){
      ss >> up.x >> up.y >> up.z;
    }
    else if(cmd == "camera_fov_ha:"){
      ss >> halfAngleVFOV;
    }
    // ignore unrecognized commands
  }
  file.close();

  //TODO: Create an orthogonal camera basis, based on the provided up and right vectors
  forward = forward.normalized();
  up = up.normalized();
  right = cross(forward, up).normalized();
  up = cross(right, forward).normalized();

  printf("Orthogonal Camera Basis:\n");
  printf("forward: %f,%f,%f\n",forward.x,forward.y,forward.z);
  printf("right: %f,%f,%f\n",right.x,right.y,right.z);
  printf("up: %f,%f,%f\n",up.x,up.y,up.z);
}

#endif
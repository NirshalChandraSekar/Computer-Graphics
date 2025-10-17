#ifndef VEC3_H
#define VEC3_H

//Small vector library
// Represents a vector as 3 floats

struct vec3{
  float x,y,z;

  vec3(float x, float y, float z) : x(x), y(y), z(z) {}
  vec3() : x(0), y(0), z(0) {}

  //Clamp each component (used to clamp pixel colors)
  vec3 clampTo1(){
    return vec3(fmin(x,1),fmin(y,1),fmin(z,1));
  }

  //Compute vector length (you may also want length squared)
  float length(){
    return sqrt(x*x+y*y+z*z);
  }

  //Create a unit-length vector
  vec3 normalized(){
    float len = sqrt(x*x+y*y+z*z);
    return vec3(x/len,y/len,z/len);
  }

};

//Multiply float and vector
//TODO - Implement: you probably also want to implement multiply vector and float ... inline vec3 operator*(float f, vec3 a)
inline vec3 operator*(float f, vec3 a){
  return vec3(a.x*f,a.y*f,a.z*f);
}

//Vector-vector dot product
inline float dot(vec3 a, vec3 b){
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

//Vector-vector cross product
inline vec3 cross(vec3 a, vec3 b){
  return vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y-a.y*b.x);
}

//Vector addition
inline vec3 operator+(vec3 a, vec3 b){
  return vec3(a.x+b.x, a.y+b.y, a.z+b.z);
}

//Vector subtraction
inline vec3 operator-(vec3 a, vec3 b){
  return vec3(a.x-b.x, a.y-b.y, a.z-b.z);
}

#endif
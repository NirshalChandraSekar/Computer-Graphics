//UMN CSCI 5607 2D Geometry Library Homework Test Harness [HW0]
// For this assignment you will implement several basic operations of
// 2D geometry. A list of functions to complete is in geom_lib_2d.h.
// This file shows example usage of those functions.

//To compile: g++ -fsanitize=address -g -std=c++11 main.cpp -o geom_test

#define _CRT_SECURE_NO_WARNINGS //Needed for MSVC to allow printf()

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

#include "PGA/pga.h"
#include "geom_lib_2d.h"

using namespace std;

int main(int argc, char *argv[]){
  printf("\nCSCI 5607 HW 1 - 2D Geometry Library Testing Examples\n");
  printf("\nTesting Distances:\n");
  Point2D p1 = Point2D(3,2);
  Point2D p2 = Point2D(1,1);
  printf("Distance from p1 to p2 is %f (should be: ~2.236)\n",dist(p1,p2));

  Dir2D d1 = Dir2D(5,0);
  printf("Displacing from p1 to d1 is %s (should be: (8.0,2.0))\n",string(move(p1,d1)).c_str());

  Line2D l1 = Line2D(1,-2);
  Point2D p3 = Point2D(2,2);
  printf("Distance p1 to l1 is %f (should be: ~0.447 or -0.447)\n",dist(l1,p1));
  printf("Distance p3 to l1 is %f (should be: ~0.894 or -0.894)\n",dist(p3,l1));


  printf("\nTesting Meet and Join:\n");
  Line2D l2 = Line2D(0,1,-3);
  Point2D p4 = intersect(l1,l2);
  printf("Intersection of l1 and l2 is %s (should be (6.0,3.0))\n",string(p4).c_str());

  Line2D l3 = join(p2,p1);
  printf("Line through p1 and p2 is %s (should be -x+2y-1=0)\n",string(l3).c_str());
  printf("Distance from l3 to p1 is %f (should be 0)\n",dist(l3,p1));
  printf("Distance from l3 to p2 is %f (should be 0)\n",dist(l3,p2));


  printf("\nTesting Projections:\n");
  Point2D p5 = project(p3,l3);
  printf("Projection of p3 onto Line l3 is %s (should be (2.2,1.6))\n",string(p5).c_str());

  Line2D l4 = project(l3,p3);
  printf("Project Line l3 onto Point p3 is %s (should be: x-2y+2=0)\n",string(l4).c_str());

  printf("Angle between lines l1 and l2 is %f (should be ~2.678 or ~0.464)\n",angle(l1,l2));


  printf("\nTesting Intersections:\n");

  Point2D p6 = Point2D(0,0);
  Point2D p7 = Point2D(3,3);
  Point2D p8 = Point2D(1,2);
  Point2D p9a = Point2D(1,4);
  Point2D p9b = Point2D(1,-4);
  printf("Does p6->p7 intersect p8->p9a? %d (should be 0/false)\n",segmentSegmentIntersect(p6,p7,p8,p9a));
  printf("Does p6->p7 intersect p8->p9b? %d (should be 1/true)\n",segmentSegmentIntersect(p6,p7,p8,p9b));


  printf("\nTesting Point in Triangle Function:\n");
  Point2D p10 = Point2D(0,0);
  Point2D p11 = Point2D(0,3);
  Point2D p12 = Point2D(3,0);
  Point2D test1 =  Point2D(1,1);
  Point2D test2 =  Point2D(4,4);

  printf("Is the point test1 in the triangle (p10,p11,p12)? %d (should be 1/true)\n",pointInTriangle(test1,p10,p11,p12));
  printf("Is the point test1 in the triangle (p12,p11,p10)? %d (should be 1/true)\n",pointInTriangle(test1,p12,p11,p10));
  printf("Is the point test2 in the triangle (p10,p11,p12)? %d (should be 0/false)\n",pointInTriangle(test2,p10,p11,p12));
  printf("Is the point test2 in the triangle (p12,p11,p10)? %d (should be 0/false)\n",pointInTriangle(test2,p12,p11,p10));

  printf("\nTesting Triangle Area funcion:\n");
  printf("Area of Triangle (p10,p11,p12) is %f (should be ~4.5 or -4.5)\n",areaTriangle(p10,p11,p12));
  printf("Area of Triangle (p12,p11,p10) is %f (should be ~4.5 or -4.5)\n",areaTriangle(p12,p11,p10));

  printf("\nTesting Triangle Distance functions:\n");
  printf("Point to triangle edge dist is %f (should be ~3.536 or -3.536)\n",pointTriangleEdgeDist(test2,p10,p11,p12));
  printf("Point to triangle edge dist is %f (should be 0)\n",pointTriangleEdgeDist(p12,p12,p11,p10));

  printf("Point to triangle corner dist is %f (should be ~4.123)\n",pointTriangleCornerDist(test2,p10,p11,p12));
  printf("Point to triangle corner dist is %f (should be 0)\n",pointTriangleCornerDist(p12,p12,p11,p10));

  printf("\nTesting Convexity:\n");
  Point2D a = Point2D(0,0);
  Point2D b = Point2D(0,2);
  Point2D c = Point2D(2,2);
  Point2D d = Point2D(2,0);
  Point2D e = Point2D(0.9,1.0);
  printf("Is the Quad (a,b,c,d) convex? %d (should be 1/true)\n",isConvex_Quad(a,b,c,d));
  printf("Is the Quad (d,c,b,a) convex? %d (should be 1/true)\n",isConvex_Quad(d,c,b,a));
  printf("Is the Quad (a,c,b,d) convex? %d (should be 0/false)\n",isConvex_Quad(a,c,b,d));
  printf("Is the Quad (a,b,c,e) convex? %d (should be 0/false)\n",isConvex_Quad(a,b,c,e));

  printf("\nTesting Reflection:\n");
  Point2D p15 = Point2D(0,3);
  Line2D l5 = Line2D(1,-1,0);
  printf("The reflection of the point p15 about the line l5 is %s (should be (3,0))\n",string(reflect(p15,l5)).c_str());

  Line2D l7 = Line2D(1,0,0);
  printf("The reflection of the line l7 about the line l5 is %s (should be y=0)\n",string(reflect(l7,l5)).c_str());

  Point2D pA = Point2D(0,0);
  Point2D pB = Point2D(1,0);
  Point2D pC = Point2D(1,1);
  Point2D p_test = Point2D(-1,0);
  printf("Distance from p_test to triangle edge is %f (should be 1)\n",pointTriangleEdgeDist(p_test,pA,pB,pC));
}

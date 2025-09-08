//UMN CSCI 5607 2D Geometry Library Homework [HW0]
//TODO: For the 18 functions below, replace their sub function with a working version that matches the desciption.

#ifndef GEOM_LIB_H
#define GEOM_LIB_H

#include "PGA/pga.h"

//Displace a point p on the direction d
//The result is a point
Point2D move(Point2D p, Dir2D d){
  Point2D result = d + p;
  return result;
}

//Compute the displacement vector between points p1 and p2
//The result is a direction 
Dir2D displacement(Point2D p1, Point2D p2){
  Dir2D result = p2 - p1;
  return result;
}

//Compute the distance between points p1 and p2
//The result is a scalar 
float dist(Point2D p1, Point2D p2){
  Dir2D displacement = p2 - p1;
  auto distance = displacement.magnitude();
  return distance;
}

//Compute the perpendicular distance from the point p the the line l
//The result is a scalar 
float dist(Line2D l, Point2D p){
  auto distance = (l.x * p.x + l.y * p.y + l.w) / sqrt(l.x * l.x + l.y * l.y);
  if (distance < 0) distance = -distance;
  return distance;
}

//Compute the perpendicular distance from the point p the the line l
//The result is a scalar 
float dist(Point2D p, Line2D l){
  auto distance = (l.x * p.x + l.y * p.y + l.w) / sqrt(l.x * l.x + l.y * l.y);
  if (distance < 0) distance = -distance;
  return distance;
}

//Compute the intersection point between lines l1 and l2
//You may assume the lines are not parallel
//The results is a a point that lies on both lines
Point2D intersect(Line2D l1, Line2D l2){
  auto denominator = l1.x * l2.y - l2.x * l1.y;
  auto x = (l1.y * l2.w - l2.y * l1.w) / denominator;
  auto y = (l2.x * l1.w - l1.x * l2.w) / denominator;
  return Point2D(x, y);
}

//Compute the line that goes through the points p1 and p2
//The result is a line 
Line2D join(Point2D p1, Point2D p2){
  Dir2D direction = p2 - p1;
  return Line2D(direction.y, -direction.x, -direction.y*p1.x + direction.x*p1.y);
}

//Compute the projection of the point p onto line l
//The result is the closest point to p that lies on line l
Point2D project(Point2D p, Line2D l){
  float distance = (l.x * p.x + l.y * p.y + l.w) / sqrt(l.x * l.x + l.y * l.y);
  Dir2D normal_direction = Dir2D(l.x, l.y).normalized();
  Point2D projected_point = p - Dir2D(normal_direction.x * distance, normal_direction.y * distance);
  return projected_point;
}

//Compute the projection of the line l onto point p
//The result is a line that lies on point p in the same direction of l
Line2D project(Line2D l, Point2D p){
  return Line2D(0,0,0); //Wrong, fix me...
}

//Compute the angle point between lines l1 and l2 in radians
//You may assume the lines are not parallel
//The results is a scalar
float angle(Line2D l1, Line2D l2){
  float sloap1 = -l1.x / l1.y;
  float sloap2 = -l2.x / l2.y;
  float radian_angle = atan2((sloap2 - sloap1), (1 + sloap1 * sloap2));
  if (radian_angle < 0) radian_angle = -radian_angle;
  return radian_angle;
}

//Compute if the line segment p1->p2 intersects the line segment a->b
//The result is a boolean
bool segmentSegmentIntersect(Point2D p1, Point2D p2, Point2D a, Point2D b){
  Dir2D segment1 = p2 - p1;
  Dir2D segment2 = b - a;

  float cross1 = segment1.x * (a.y - p1.y) - segment1.y * (a.x - p1.x);
  float cross2 = segment1.x * (b.y - p1.y) - segment1.y * (b.x - p1.x);
  if (cross1 * cross2 > 0) return false;

  float cross3 = segment2.x * (p1.y - a.y) - segment2.y * (p1.x - a.x);
  float cross4 = segment2.x * (p2.y - a.y) - segment2.y * (p2.x - a.x);
  if (cross3 * cross4 > 0) return false;

  return true;
}

//Compute if the point p lies inside the triangle t1,t2,t3
//Your code should work for both clockwise and counterclockwise windings
//The result is a bool
bool pointInTriangle(Point2D p, Point2D t1, Point2D t2, Point2D t3){
  Dir2D segment1 = t2 - t1;
  Dir2D segment2 = t3 - t2;
  Dir2D segment3 = t1 - t3;

  float cross1 = segment1.x * (p.y - t1.y) - segment1.y * (p.x - t1.x);
  float cross2 = segment2.x * (p.y - t2.y) - segment2.y * (p.x - t2.x);
  float cross3 = segment3.x * (p.y - t3.y) - segment3.y * (p.x - t3.x);

  // Check if all cross products have the same sign. Check for both windings.
  if ((cross1 >= 0 && cross2 >= 0 && cross3 >= 0) || (cross1 <= 0 && cross2 <= 0 && cross3 <= 0)) {
    return true;
  }
  else {
    return false;
  }
}

//Compute the area of the triangle t1,t2,t3
//The result is a scalar
float areaTriangle(Point2D t1, Point2D t2, Point2D t3){
  float base_length = Dir2D(t2 - t1).magnitude();
  float height =  dist(t3, join(t1, t2));
  float area = 0.5 * base_length * height;
  return area;
}

//Compute the distance from the point p to the triangle t1,t2,t3 as defined 
//by it's distance from the edge closest to p.
//The result is a scalar
//NOTE: There are some tricky cases to consider here that do not show up in the test cases!
/*
CHECK THIS LATER TO TAKE THE SEGMENT INSTEAD OF THE FULL INFINITE LINE
*/
float pointTriangleEdgeDist(Point2D p, Point2D t1, Point2D t2, Point2D t3){
  if(pointInTriangle(p, t1, t2, t3)) return 0;
  float d1 = dist(p, join(t1, t2));
  float d2 = dist(p, join(t2, t3));
  float d3 = dist(p, join(t3, t1));
  return min(d1, min(d2, d3));
}

//Compute the distance from the point p to the closest of three corners of
// the triangle t1,t2,t3
//The result is a scalar
float pointTriangleCornerDist(Point2D p, Point2D t1, Point2D t2, Point2D t3){
  float distance1 = sqrt(pow(p.x - t1.x, 2) + pow(p.y - t1.y, 2));
  float distance2 = sqrt(pow(p.x - t2.x, 2) + pow(p.y - t2.y, 2));
  float distance3 = sqrt(pow(p.x - t3.x, 2) + pow(p.y - t3.y, 2));
  return min(distance1, min(distance2, distance3));
}

//Compute if the quad (p1,p2,p3,p4) is convex.
//Your code should work for both clockwise and counterclockwise windings
//The result is a boolean
bool isConvex_Quad(Point2D p1, Point2D p2, Point2D p3, Point2D p4){
  return false; //Wrong, fix me...
}

//Compute the reflection of the point p about the line l
//The result is a point
Point2D reflect(Point2D p, Line2D l){
  return Point2D(0,0); //Wrong, fix me...
}

//Compute the reflection of the line d about the line l
//The result is a line
Line2D reflect(Line2D d, Line2D l){
  return Line2D(0,0,0); //Wrong, fix me...
}

#endif
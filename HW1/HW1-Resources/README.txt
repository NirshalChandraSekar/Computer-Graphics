This zip file contains two files and a directory.

The first file, geom_lib_2d.h, contains subs of several functions relating to 2D geometric processing that you must implement. For example

```
Point2D move(Point2D p, Dir2D d){
  return Point2D(0,0); //Wrong, fix me...
}
```

Should return a new point that is d units away from p.
You can do that with either of the following solutions:

```
Point2D move(Point2D p, Dir2D d){
  return Point2D(p.x+d.x, p.y+d.y);
}
```

Or


```
Point2D move(Point2D p, Dir2D d){
  return p+d; //Exploits the operator overloading in the PGA library
}
```

Once updated, this file, geom_lib_2d.h, is the only file you need to turn in.

---

The second file, main.cpp, is a test harness to help you test if your implementation of in the geometric library, geom_lib_2d.h,  is correct. I only have a few tests in this file. Be sure to add your own tests to make sure your implementation is robust.
Your grade will be based on many tests, not just the few samples I provide.

To run this test you can use the following command on Linux or Mac terminal:
g++ main.cpp && ./a.out

On Windows Visual Studio, create a project consisting one the single file main.cpp, and run that project.

The output shows the test run, what result your code gives, and what the expected result was.

---
The directory /PGA/ has a library that implements a PGA multivector class. This class supports wedge, vee, and dot products, along with duals, reverse operations. The class also implements an overloaded +, -, * and / operator.

The library also provides several 2D geometric primitives representing points, lines, and directions; and has a motor primitive that can represent rigid 2D motion of rotations and translations. Each primitive can be cast to a multivector or can be used as the primitive directly.

To help you understand what is built into the PGA library, the file pga_example.cpp gives several examples of how to use the library. 
I would suggest a good starting point is to compile and run pga_example.cpp, and then read through the code to understand how it works.

g++ ./PGA/pga_example.cpp && ./a.out
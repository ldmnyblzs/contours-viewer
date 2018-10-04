#ifndef PRIMITIVES_HPP
#define PRIMITIVES_HPP

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Aff_transformation_3.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef Kernel::Line_3 Line;
typedef Kernel::Plane_3 Plane;
typedef Kernel::Triangle_3 Triangle;
typedef Kernel::Sphere_3 Sphere;
typedef Kernel::Circle_3 Circle;
typedef Kernel::Iso_cuboid_3 Cuboid;

typedef CGAL::Aff_transformation_3<Kernel> Transform;

#ifdef _WIN32
typedef std::wstring path_type;
#else
typedef std::string path_type;
#endif

#endif // PRIMITIVES_HPP

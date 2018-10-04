#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <CGAL/Random_access_adaptor.h>
#include <tbb/blocked_range.h>

#include "primitives.hpp"

template <typename Mesh>
class Vertex_base {
    typedef typename Mesh::Vertex_handle handle_type;
    const Mesh *mesh;
    handle_type vertex;
public:
    Vertex_base(const Mesh *mesh, handle_type vertex) :
        mesh(mesh),
        vertex(vertex) {
    }
    Vertex_base(const Vertex_base &) = default;
    Vertex_base(Vertex_base &&) = default;
    Vertex_base &operator=(const Vertex_base &) = default;
    Vertex_base &operator=(Vertex_base &&) = default;
    ~Vertex_base() = default;

    auto distance(const Point &center) const {
        return mesh->vertex_distance(center, vertex);
    }
    auto point() const {
        return mesh->vertex_point(vertex);
    }
    auto halfedge_to(const Vertex_base &target) const {
        return mesh->halfedge(vertex, target.vertex);
    }
    auto handle() const {
        return vertex;
    }

    bool operator==(const Vertex_base<Mesh> &other) const {
        return std::tie(mesh, vertex) == std::tie(other.mesh, other.vertex);
    }
    bool operator!=(const Vertex_base<Mesh> &other) const {
        return !(*this == other);
    }
    bool operator<(const Vertex_base<Mesh> &other) const {
        return vertex < other.vertex;
    }
};

template <typename Mesh, typename Iterator>
class VertexIterator :
        public boost::iterator_facade<VertexIterator<Mesh, Iterator>, Vertex_base<Mesh> const, boost::random_access_traversal_tag, Vertex_base<Mesh>> {
    friend class boost::iterator_core_access;

    Vertex_base<Mesh> dereference() const {
        return Vertex_base<Mesh>(mesh, *(adaptor[index]));
    }
    bool equal(const VertexIterator<Mesh, Iterator> &other) const {
        return index == other.index;
    }
    void increment() {
        ++index;
    }
    void decrement() {
        --index;
    }
    void advance(std::size_t a) {
        index += a;
    }
    std::ptrdiff_t distance_to(const VertexIterator<Mesh, Iterator> &other) const {
        return other.index - index;
    }

    const Mesh *mesh;
    CGAL::Random_access_adaptor<Iterator> adaptor;
    std::size_t index;
public:
    VertexIterator() = default;
    VertexIterator(const Mesh *mesh, const Iterator &begin, const Iterator &end, std::size_t index = 0) :
        mesh(mesh),
        adaptor(begin, end),
        index(index) {
    }
};

template <typename Mesh, typename Iterator>
using VertexRange = tbb::blocked_range<VertexIterator<Mesh, Iterator> >;

#endif // VERTEX_HPP

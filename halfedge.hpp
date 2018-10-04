#ifndef HALFEDGE_HPP
#define HALFEDGE_HPP

#include <CGAL/Random_access_adaptor.h>
#include <tbb/blocked_range.h>

#include "primitives.hpp"
//#include "vertex.hpp"

template <typename Mesh>
class Halfedge_base {
    typedef typename Mesh::Halfedge_handle handle_type;
    const Mesh *mesh = nullptr;
    handle_type halfedge;
public:
    Halfedge_base() {
    }
    Halfedge_base(const Mesh *mesh, handle_type halfedge) :
        mesh(mesh),
        halfedge(halfedge) {
    }
    Halfedge_base(const Halfedge_base &) = default;
    Halfedge_base(Halfedge_base &&) = default;
    Halfedge_base &operator=(const Halfedge_base &) = default;
    Halfedge_base &operator=(Halfedge_base &&) = default;
    ~Halfedge_base() = default;

    auto source() const {
        return mesh->source(halfedge);
    }
    auto target() const {
        return mesh->target(halfedge);
    }
    auto next() const {
        return mesh->next(halfedge);
    }
    auto opposite() const {
        return mesh->opposite(halfedge);
    }
    auto handle() const {
        return halfedge;
    }
    bool operator==(const Halfedge_base<Mesh> &other) const {
        return std::tie(mesh, halfedge) == std::tie(other.mesh, other.halfedge);
    }
    bool operator!=(const Halfedge_base<Mesh> &other) const {
        return !(*this == other);
    }
};

template <typename Mesh, typename Iterator>
class HalfedgeIterator :
public boost::iterator_facade<HalfedgeIterator<Mesh, Iterator>, Halfedge_base<Mesh> const, boost::random_access_traversal_tag, Halfedge_base<Mesh>> {
    friend class boost::iterator_core_access;

    Halfedge_base<Mesh> dereference() const {
        return Halfedge_base<Mesh>(mesh, *adaptor[index]);
    }
    bool equal(const HalfedgeIterator<Mesh, Iterator> &other) const {
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
    std::ptrdiff_t distance_to(const HalfedgeIterator<Mesh, Iterator> &other) const {
        return other.index - index;
    }

    const Mesh *mesh;
    CGAL::Random_access_adaptor<Iterator> adaptor;
    std::size_t index;
public:
    HalfedgeIterator() = default;
    HalfedgeIterator(const Mesh *mesh, const Iterator &begin, const Iterator &end, std::size_t index = 0) :
        mesh(mesh),
        adaptor(begin, end),
        index(index) {
    }
};

template <typename Mesh, typename Iterator>
using HalfedgeRange = tbb::blocked_range<HalfedgeIterator<Mesh, Iterator> >;

#endif // HALFEDGE_HPP

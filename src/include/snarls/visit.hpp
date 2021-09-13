#ifndef LIBSNARLS_VISIT_HPP_INCLUDED
#define LIBSNARLS_VISIT_HPP_INCLUDED

#include <handlegraph/handle_graph.hpp>

#include <vg/vg.pb.h>

#include <iostream>

namespace snarls {

using namespace std;
using namespace vg;

/// Make a Visit from a node ID and an orientation
inline Visit to_visit(id_t node_id, bool is_reverse);
    
/// Make a Visit from a snarl to traverse
inline Visit to_visit(const Snarl& snarl);

/// Make a Visit from a handle in a HandleGraph.
inline Visit to_visit(const handlegraph::HandleGraph& graph, const handle_t& handle);
    
/// Get the reversed version of a visit
inline Visit reverse(const Visit& visit);

/// Make an edge_t from a pair of visits
edge_t to_edge(const handlegraph::HandleGraph& graph, const Visit& v1, const Visit& v2);

// We need some Visit operators
    
/**
 * Two Visits are equal if they represent the same traversal of the same
 * Node or Snarl.
 */
bool operator==(const Visit& a, const Visit& b);
/**
 * Two Visits are unequal if they are not equal.
 */
bool operator!=(const Visit& a, const Visit& b);
/**
 * A Visit is less than another Visit if it represents a traversal of a
 * smaller node, or it represents a traversal of a smaller snarl, or it
 * represents a traversal of the same node or snarl forward instead of
 * backward.
 */
bool operator<(const Visit& a, const Visit& b);
    
/**
 * A Visit can be printed.
 */
ostream& operator<<(ostream& out, const Visit& visit);
    
/****
 * Template and Inlines:
 ****/
    
inline Visit to_visit(id_t node_id, bool is_reverse) {
    Visit to_return;
    to_return.set_node_id(node_id);
    to_return.set_backward(is_reverse);
    return to_return;
}
    
inline Visit to_visit(const Snarl& snarl) {
    Visit to_return;
    // Only copy necessary fields
    *to_return.mutable_snarl()->mutable_start() = snarl.start();
    *to_return.mutable_snarl()->mutable_end() = snarl.end();
    return to_return;
}

inline Visit to_visit(const handlegraph::HandleGraph& graph, const handle_t& handle) {
    return to_visit(graph.get_id(handle), graph.get_is_reverse(handle));
}
    
inline Visit reverse(const Visit& visit) {
    // Copy the visit
    Visit to_return = visit;
    // And flip its orientation bit
    to_return.set_backward(!visit.backward());
    return to_return;
}

}

#endif


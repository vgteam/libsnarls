#include "snarls/visit.hpp"

namespace snarls {

using namespace std;
using namespace vg;
using namespace handlegraph;


edge_t to_edge(const HandleGraph& graph, const Visit& v1, const Visit& v2) {

    id_t prev_id;
    bool prev_back;
    if (v1.node_id() != 0) {
        prev_id = v1.node_id();
        prev_back = v1.backward();
    } else {
        const Snarl& prev_snarl = v1.snarl();
        if (v1.backward()) {
            prev_id = prev_snarl.start().node_id();
            prev_back = !prev_snarl.start().backward();
        } else {
            prev_id = prev_snarl.end().node_id();
            prev_back = prev_snarl.end().backward();
        }
    }
    
    id_t cur_id;
    bool cur_back;                
    if (v2.node_id() != 0) {
        cur_id = v2.node_id();
        cur_back = v2.backward();
    } else {
        const Snarl& cur_snarl = v2.snarl();
        if (v2.backward()) {
            cur_id = cur_snarl.end().node_id();
            cur_back = !cur_snarl.end().backward();
        } else {
            cur_id = cur_snarl.start().node_id();
            cur_back = cur_snarl.start().backward();
        }
    }

    return graph.edge_handle(graph.get_handle(prev_id, prev_back),
                             graph.get_handle(cur_id, cur_back));

}

bool operator==(const Visit& a, const Visit& b) {
    // IDs and orientations have to match, and nobody has a snarl or the
    // snarls match.
    return a.node_id() == b.node_id() &&
        a.backward() == b.backward() &&
        ((!a.has_snarl() && !b.has_snarl()) ||
         a.snarl() == b.snarl());
}
    
bool operator!=(const Visit& a, const Visit& b) {
    return !(a == b);
}
    
bool operator<(const Visit& a, const Visit& b) {
    if (!a.has_snarl() && !b.has_snarl()) {
        // Compare everything but the snarl
        return make_tuple(a.node_id(), a.backward()) < make_tuple(b.node_id(), b.backward());
    } else {
        // Compare including the snarl
        return make_tuple(a.node_id(), a.snarl(), a.backward()) < make_tuple(b.node_id(), b.snarl(), b.backward());
    }        
}
    
ostream& operator<<(ostream& out, const Visit& visit) {
    if (!visit.has_snarl()) {
        // Use the node ID
        out << visit.node_id();
    } else {
        // Use the snarl
        out << visit.snarl();
    }
    out << " " << (visit.backward() ? "rev" : "fwd");
    return out;
}

}

#include "snarls/snarl_traversal.hpp"

namespace snarls {

using namespace std;
using namespace vg;
using namespace handlegraph;

bool operator==(const SnarlTraversal& a, const SnarlTraversal& b) {
    if (a.visit_size() != b.visit_size()) {
        return false;
    }
    for (size_t i = 0; i < a.visit_size(); i++) {
        if (a.visit(i) != b.visit(i)) {
            return false;
        }
    }
    // Otherwise everything we can think of matches
    return true;
}
    
bool operator!=(const SnarlTraversal& a, const SnarlTraversal& b) {
    return !(a == b);
}
    
bool operator<(const SnarlTraversal& a, const SnarlTraversal& b) {
    for (size_t i = 0; i < b.visit_size(); i++) {
        if (i >= a.visit_size()) {
            // A has run out and B is still going
            return true;
        }
            
        if (a.visit(i) < b.visit(i)) {
            return true;
        } else if (b.visit(i) < a.visit(i)) {
            return false;
        }
    }
        
    // If we get here either they're equal or A has more visits than B
    return false;
}

}


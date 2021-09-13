#include "snarls/snarl.hpp"

namespace snarls {

using namespace std;
using namespace vg;
using namespace handlegraph;

bool operator==(const Snarl& a, const Snarl& b) {
    if (a.type() != b.type()) {
        return false;
    }
    if (a.start() != b.start()) {
        return false;
    }
    if (a.end() != b.end()) {
        return false;
    }
    if (a.has_parent() || b.has_parent()) {
        // Someone has a parent so we must compare them.
        return a.parent() == b.parent();
    }
    return true;
}
    
bool operator!=(const Snarl& a, const Snarl& b) {
    return !(a == b);
}
    
bool operator<(const Snarl& a, const Snarl& b) {
    if (!a.has_parent() && !b.has_parent()) {
        // Compare without parent
        return make_tuple(a.type(), a.start(), a.end()) < make_tuple(b.type(), b.start(), b.end());
    } else {
        // Compare with parent
        return make_tuple(a.type(), a.start(), a.end(), a.parent()) < make_tuple(b.type(), b.start(), b.end(), b.parent());
    }
}
    
ostream& operator<<(ostream& out, const Snarl& snarl) {
    return out << snarl.start() << "-" << snarl.end();
}

}







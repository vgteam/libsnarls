#ifndef LIBSNARLS_SNARL_HPP_INCLUDED
#define LIBSNARLS_SNARL_HPP_INCLUDED

#include <vg/vg.pb.h>

namespace snarls {

using namespace std;
using namespace vg;
    
/// Copies the boundary Visits from one Snarl into another
inline void transfer_boundary_info(const Snarl& from, Snarl& to);

// And some operators for Snarls
    
/**
 * Two Snarls are equal if their types are equal and their bounding Visits
 * are equal and their parents are equal.
 */
bool operator==(const Snarl& a, const Snarl& b);
/**
 * Two Snarls are unequal if they are not equal.
 */
bool operator!=(const Snarl& a, const Snarl& b);
/**
 * A Snarl is less than another Snarl if its type is smaller, or its start
 * Visit is smaller, or its end Visit is smaller, or its parent is smaller.
 */
bool operator<(const Snarl& a, const Snarl& b);
    
/**
 * A Snarl can be printed.
 */
ostream& operator<<(ostream& out, const Snarl& snarl);
    
/****
 * Template and Inlines:
 ****/
    
inline void transfer_boundary_info(const Snarl& from, Snarl& to) {
    *to.mutable_start() = from.start();
    *to.mutable_end() = from.end();
}

}

// note: this hash funtion is not used internally because we want the internal indices to ignore any
// additional information so that the Snarls stored as references map to the same place
// as the original objects
namespace std {
/// hash function for Snarls
template<>
struct hash<const vg::Snarl> {
    size_t operator()(const vg::Snarl& snarl) const {
        auto hsh = hash<pair<pair<int64_t, bool>, pair<int64_t, bool> > >();
        return hsh(make_pair(make_pair(snarl.start().node_id(), snarl.start().backward()),
                             make_pair(snarl.end().node_id(), snarl.end().backward())));
    }
};
}

#endif

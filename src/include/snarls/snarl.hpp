#ifndef LIBSNARLS_SNARL_HPP_INCLUDED
#define LIBSNARLS_SNARL_HPP_INCLUDED

#include "snarls/visit.hpp"
#include "snarls/vg_types.hpp"

#include <iostream>
#include <sstream>

namespace snarls {

using namespace std;
using namespace vg;
    
/// Copies the boundary Visits from one Snarl into another
inline void transfer_boundary_info(const Snarl& from, Snarl& to);

/// Convert a Snarl to a printable string
inline string to_string(const Snarl& snarl);

}

namespace vg {

using namespace std;

// Snarl operators need to be put in the vg namespace, where the Snarl type is,
// or they will not be found.
    
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

}
    

namespace snarls {

using namespace std;
using namespace vg;

/****
 * Template and Inlines:
 ****/
    
inline void transfer_boundary_info(const Snarl& from, Snarl& to) {
    *to.mutable_start() = from.start();
    *to.mutable_end() = from.end();
}

inline string to_string(const Snarl& snarl) {
    stringstream ss;
    ss << snarl;
    return ss.str();
}

}

// Note: this hash funtion is not used internally because we want the internal indices to ignore any
// additional information so that the Snarls stored as references map to the same place
// as the original objects
namespace std {
/// Hash function for Snarls
template<>
struct hash<const vg::Snarl> {
    size_t operator()(const vg::Snarl& snarl) const;
};
}

#endif

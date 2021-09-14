#ifndef LIBSNARLS_SNARL_TRAVERSAL_HPP_INCLUDED
#define LIBSNARLS_SNARL_TRAVERSAL_HPP_INCLUDED

#include "snarls/vg_types.hpp"

namespace snarls {

using namespace std;
using namespace vg;
    
// Some operators for SnarlTraversals
    
/**
 * Two SnarlTraversals are equal if their snarls are equal and they have the
 * same number of visits and all their visits are equal.
 */
bool operator==(const SnarlTraversal& a, const SnarlTraversal& b);
/**
 * Two SnarlTraversals are unequal if they are not equal.
 */
bool operator!=(const SnarlTraversal& a, const SnarlTraversal& b);
/**
 * A SnalTraversal is less than another if it is a traversal of a smaller
 * Snarl, or if its list of Visits has a smaller Visit first, or if its list
 * of Visits is shorter.
 */
bool operator<(const SnarlTraversal& a, const SnarlTraversal& b);

}

#endif


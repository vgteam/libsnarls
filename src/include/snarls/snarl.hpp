#ifndef LIBSNARLS_SNARL_HPP_INCLUDED
#define LIBSNARLS_SNARL_HPP_INCLUDED

#include "snarls/visit.hpp"

#include <vg/vg.pb.h>

namespace snarls {

using namespace std;
using namespace vg;
    
/// Copies the boundary Visits from one Snarl into another
inline void transfer_boundary_info(const Snarl& from, Snarl& to);

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

}

// http://stackoverflow.com/questions/4870437/pairint-int-pair-as-key-of-unordered-map-issue#comment5439557_4870467
// https://github.com/Revolutionary-Games/Thrive/blob/fd8ab943dd4ced59a8e7d1e4a7b725468b7c2557/src/util/pair_hash.h
// taken from boost
#ifndef OVERLOAD_PAIR_HASH
#define OVERLOAD_PAIR_HASH
namespace std {
namespace
{
    
    // Code from boost
    // Reciprocal of the golden ratio helps spread entropy
    //     and handles duplicates.
    // See Mike Seymour in magic-numbers-in-boosthash-combine:
    //     http://stackoverflow.com/questions/4948780
    
    template <class T>
    inline void hash_combine(size_t& seed, T const& v)
    {
        seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }
    
    // Recursive template code derived from Matthieu M.
    template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
    struct HashValueImpl
    {
        static void apply(size_t& seed, Tuple const& tuple)
        {
            HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
            hash_combine(seed, std::get<Index>(tuple));
        }
    };
    
    template <class Tuple>
    struct HashValueImpl<Tuple,0>
    {
        static void apply(size_t& seed, Tuple const& tuple)
        {
            hash_combine(seed, std::get<0>(tuple));
        }
    };
}
    
template <typename A, typename B>
struct hash<pair<A,B> > {
    size_t operator()(const pair<A,B>& x) const {
        size_t hash_val = std::hash<A>()(x.first);
        hash_combine(hash_val, x.second);
        return hash_val;
    }
};

// from http://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
template <typename ... TT>
struct hash<std::tuple<TT...>>
{
    size_t
    operator()(std::tuple<TT...> const& tt) const
    {
        size_t seed = 0;
        HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
        return seed;
    }
    
};
}
#endif  // OVERLOAD_PAIR_HASH

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

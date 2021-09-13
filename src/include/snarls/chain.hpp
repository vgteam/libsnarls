#ifndef LIBSNARLS_CHAIN_HPP_INCLUDED
#define LIBSNARLS_CHAIN_HPP_INCLUDED

#include <vg/vg.pb.h>

#include <vector>
#include <utility>

namespace snarls {

using namespace std;
using namespace vg;

/**
 * Snarls are defined at the Protobuf level, but here is how we define
 * chains as real objects.
 *
 * A chain is a sequence of Snarls, in either normal (false) or reverse (true)
 * orientation.
 *
 * The SnarlManager is going to have one official copy of each chain stored,
 * and it will give you a pointer to it on demand.
 */
using Chain = vector<pair<const Snarl*, bool>>;
    
/**
 * Return true if the first snarl in the given chain is backward relative to the chain.
 */
bool start_backward(const Chain& chain);
    
/**
 * Return true if the last snarl in the given chain is backward relative to the chain.
 */
bool end_backward(const Chain& chain);
    
/**
 * Get the inward-facing start Visit for a chain.
 */
Visit get_start_of(const Chain& chain);
    
/**
 * Get the outward-facing end Visit for a chain.
 */
Visit get_end_of(const Chain& chain);
    
/**
 * We want to be able to loop over a chain and get iterators to pairs of the
 * snarl and its orientation in the chain. So we define some iterators.
 */
struct ChainIterator {
    /// Advance the iterator
    ChainIterator& operator++();
    /// Get the snarl we're at and whether it is backward 
    pair<const Snarl*, bool> operator*() const;
    /// Get a pointer to the thing we get when we dereference the iterator
    const pair<const Snarl*, bool>* operator->() const;
        
    /// We need to define comparison because C++ doesn't give it to us for free.
    bool operator==(const ChainIterator& other) const;
    bool operator!=(const ChainIterator& other) const;
        
    /// Are we a reverse iterator or not?
    bool go_left;
    
    /// What position in the underlying vector are we in?
    Chain::const_iterator pos;
        
    /// What are the bounds of that underlying vector?
    Chain::const_iterator chain_start;
    Chain::const_iterator chain_end;
        
    /// Since we're using backing random access itarators to provide reverse
    /// iterators, we need a flag to see if we are rend (i.e. before the
    /// beginning)
    bool is_rend;
        
    /// When dereferencing, should we flip snarl orientations form the
    /// orientations they appear at in the chain when read left to right?
    bool complement;
        
    /// In order to dereference to a pair with -> we need a place to put the pair so we can have a pointer to it.
    /// Gets lazily set to wherever the iterator is pointing when we do ->
    mutable pair<const Snarl*, bool> scratch;
};
    
/**
 * We define free functions for getting iterators forward and backward through chains.
 */
ChainIterator chain_begin(const Chain& chain);
ChainIterator chain_end(const Chain& chain);
ChainIterator chain_rbegin(const Chain& chain);
ChainIterator chain_rend(const Chain& chain);
    
/// We also define some reverse complement iterators, which go from right to
/// left through the chains, but give us the reverse view. For ecample, if
/// all the snarls are oriented forward in the chain, we will iterate
/// through the snarls in reverse order, with each individual snarl also
/// reversed.
ChainIterator chain_rcbegin(const Chain& chain);
ChainIterator chain_rcend(const Chain& chain);
    
/// We also define a function for getting the ChainIterator (forward or reverse
/// complement) for a chain starting with a given snarl in the given inward
/// orientation. Only works for bounding snarls of the chain.
ChainIterator chain_begin_from(const Chain& chain, const Snarl* start_snarl, bool snarl_orientation);
/// And the end iterator for the chain (forward or reverse complement) viewed
/// from a given snarl in the given inward orientation. Only works for bounding
/// snarls of the chain, and should be the *same* bounding snarl as was used
/// for chain_begin_from.
ChainIterator chain_end_from(const Chain& chain, const Snarl* start_snarl, bool snarl_orientation);

}

#endif

#include "snarls/chain.hpp"

#include "snarls/visit.hpp"

namespace snarls {

using namespace std;
using namespace vg;

bool start_backward(const Chain& chain) {
    // The start snarl is backward if it is marked backward.
    return !chain.empty() && chain.front().second;
}
    
bool end_backward(const Chain& chain) {
    // The end snarl is backward if it is marked backward.
    return !chain.empty() && chain.back().second;
}

Visit get_start_of(const Chain& chain) {
    // Get a bounding visit and return it.
    return chain.front().second ? reverse(chain.front().first->end()) : chain.front().first->start();
}
    
Visit get_end_of(const Chain& chain) {
    // Get a bounding visit and return it.
    return chain.back().second ? reverse(chain.back().first->start()) : chain.back().first->end();
}
    
bool ChainIterator::operator==(const ChainIterator& other) const {
    return (tie(go_left, pos, chain_start, chain_end, is_rend, complement) ==
            tie(other.go_left, other.pos, other.chain_start, other.chain_end, other.is_rend, other.complement));
}
    
bool ChainIterator::operator!=(const ChainIterator& other) const {
    auto unequal = !(*this == other);
    return unequal;
}
    
ChainIterator& ChainIterator::operator++() {
    if (go_left) {
        // Walk left
            
        if (pos == chain_start) {
            if (is_rend) {
                throw runtime_error("Walked off start!");
            }
                
            // We're already at the start, so next is just going to become rend
            is_rend = true;
        } else {
            // There's actually something to the left of us
            --pos;
        }
    } else {
        // Walk right
            
        if (pos == chain_end) {
            throw runtime_error("Walked off end!");
        }
            
        ++pos;
    }
        
    return *this;
}
    
pair<const Snarl*, bool> ChainIterator::operator*() const {
    return make_pair(pos->first, pos->second != complement);
}
    
const pair<const Snarl*, bool>* ChainIterator::operator->() const {
    // Make the pair we need
    scratch = *(*this);
    // Return a pointer to it.
    return &scratch;
}
    
ChainIterator chain_begin(const Chain& chain) {
    ChainIterator to_return{
        false, // Don't go left
        chain.begin(), // Be at the start of the chain
        chain.begin(), // Here's the chain's start
        chain.end(), // And its end
        false, // This is not a reverse end
        false // Do not complement snarl orientations
    };
        
    return to_return;
}
    
ChainIterator chain_end(const Chain& chain) {
    ChainIterator to_return{
        false, // Don't go left
        chain.end(), // Be at the end of the chain
        chain.begin(), // Here's the chain's start
        chain.end(), // And its end
        false, // This is not a reverse end
        false // Do not complement snarl orientations
    };
        
    return to_return;
}
    
ChainIterator chain_rbegin(const Chain& chain) {
    if (chain.empty()) {
        // If it's empty we should be the rend past-the-end reverse iterator
        return chain_rend(chain);
    }
        
    // Otherwise there's at least one element so point to the last.
    ChainIterator to_return{
        true, // Go left
        --chain.end(), // Be at the last real thing in the chain
        chain.begin(), // Here's the chain's start
        chain.end(), // And its end
        false, // This is not a reverse end
        false // Do not complement snarl orientations
    };
        
    return to_return;
}
    
ChainIterator chain_rend(const Chain& chain) {
    ChainIterator to_return{
        true, // Go left
        chain.begin(), // Be at the start of the chain
        chain.begin(), // Here's the chain's start
        chain.end(), // And its end
        true, // This is a reverse end
        false // Do not complement snarl orientations
    };
        
    return to_return;
}
    
ChainIterator chain_rcbegin(const Chain& chain) {
    ChainIterator to_return = chain_rbegin(chain);
    to_return.complement = true;
    return to_return;
}
    
ChainIterator chain_rcend(const Chain& chain) {
    ChainIterator to_return = chain_rend(chain);
    to_return.complement = true;
    return to_return;
}
    
ChainIterator chain_begin_from(const Chain& chain, const Snarl* start_snarl, bool snarl_orientation) {
    assert(!chain.empty());
    if (start_snarl == chain.front().first && snarl_orientation == start_backward(chain)) {
        // We are at the left end of the chain, in the correct orientation, so go forward
        return chain_begin(chain);
    } else if (start_snarl == chain.back().first) {
        // We are at the right end of the chain, so go reverse complement
        return chain_rcbegin(chain);
    } else {
        throw runtime_error("Tried to view a chain from a snarl not at either end!");
    }
    return ChainIterator();
}
    
ChainIterator chain_end_from(const Chain& chain, const Snarl* start_snarl, bool snarl_orientation) {
    assert(!chain.empty());
    if (start_snarl == chain.front().first && snarl_orientation == start_backward(chain)) {
        // We are at the left end of the chain, so go forward
        return chain_end(chain);
    } else if (start_snarl == chain.back().first) {
        // We are at the right end of the chain, so go reverse complement
        return chain_rcend(chain);
    } else {
        throw runtime_error("Tried to view a chain from a snarl not at either end!");
    }
    return ChainIterator();
} 

}

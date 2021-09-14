#ifndef LIBSNARLS_SNARL_MANAGER_HPP_INCLUDED
#define LIBSNARLS_SNARL_MANAGER_HPP_INCLUDED

#include "snarls/net_graph.hpp"
#include "snarls/vg_types.hpp"

#include <iostream>
#include <vector>
#include <deque>
#include <unordered_map>
#include <random>

namespace snarls {

using namespace std;
using namespace vg;

/**
 * A structure to keep track of the tree relationships between Snarls and perform utility algorithms
 * on them
 */
class SnarlManager {
public:
        
    /// Construct a SnarlManager for the snarls returned by an iterator
    /// Also covers iterators of chains of snarls.
    template <typename SnarlIterator>
    SnarlManager(SnarlIterator begin, SnarlIterator end);
        
    /// Construct a SnarlManager for the snarls contained in an input stream
    SnarlManager(istream& in);
    
    /// Construct a SnarlManager from a function that calls a callback with each Snarl in turn
    SnarlManager(const function<void(const function<void(Snarl&)>&)>& for_each_snarl);
        
    /// Default constructor for an empty SnarlManager. Must call finish() once
    /// all snarls have been added with add_snarl().
    SnarlManager() = default;
        
    /// Destructor
    ~SnarlManager() = default;
        
    /// Cannot be copied because of all the internal pointer indexes
    SnarlManager(const SnarlManager& other) = delete;
    SnarlManager& operator=(const SnarlManager& other) = delete;
        // copy the SnarlManager
    /// Can be moved
    SnarlManager(SnarlManager&& other) = default;
    SnarlManager& operator=(SnarlManager&& other) = default;

    // Can be serialized
    void serialize(ostream& out) const;
    
    ///////////////////////////////////////////////////////////////////////////
    // Write API
    ///////////////////////////////////////////////////////////////////////////
    
    /// Add the given snarl to the SnarlManager. After all snarls have been
    /// added, finish() must be called to compute chains and indexes. We don't
    /// let precomputed chains be added, because we want chain orientations
    /// relative to snarls to be deterministic given an order of snarls.
    /// Returns a pointer to the managed snarl copy.
    /// Only this function may add in new Snarls.
    const Snarl* add_snarl(const Snarl& new_snarl);
    
    /// Reverses the orientation of a managed snarl.
    void flip(const Snarl* snarl);
    
    /// Reverses the order and orientation of a managed chain, leaving all the
    /// component snarls in their original orientations.
    void flip(const Chain* snarl);
        
    /// Note that we have finished calling add_snarl. Compute the snarl
    /// parent/child indexes and chains.
    void finish();
    
    ///////////////////////////////////////////////////////////////////////////
    // Read API
    ///////////////////////////////////////////////////////////////////////////

    /// Returns a vector of pointers to the children of a Snarl.
    /// If given null, returns the top-level root snarls.
    const vector<const Snarl*>& children_of(const Snarl* snarl) const;
        
    /// Returns a pointer to the parent of a Snarl or nullptr if there is none
    const Snarl* parent_of(const Snarl* snarl) const;
        
    /// Returns the Snarl that a traversal points into at either the start
    /// or end, or nullptr if the traversal does not point into any Snarl.
    /// Note that Snarls store the end Visit pointing out of rather than
    /// into the Snarl, so they must be reversed to query it.
    const Snarl* into_which_snarl(int64_t id, bool reverse) const;
        
    /// Returns the Snarl that a Visit points into. If the Visit contains a
    /// Snarl rather than a node ID, returns a pointer the managed version
    /// of that snarl.
    const Snarl* into_which_snarl(const Visit& visit) const;
    
    /// Get the Chain that the given snarl participates in. Instead of asking
    /// this class to walk the chain for you, use ChainIterators on this chain.
    /// This is always non-null.
    const Chain* chain_of(const Snarl* snarl) const;
    
    /// If the given Snarl is backward in its chain, return true. Otherwise,
    /// return false.
    bool chain_orientation_of(const Snarl* snarl) const;
    
    /// Get the rank that the given snarl appears in in its chain. If two
    /// snarls are in forward orientation in the chain, then leaving the end of
    /// the lower rank snarl will eventually reach the start of the higher rank
    /// snarl. If either or both snarls is backward, you leave/arrive at the
    /// other bounding node instead.
    ///
    /// Sorting snarls by rank will let you visit them in chain order without
    /// walking the whole chain.
    size_t chain_rank_of(const Snarl* snarl) const;
    
    /// Return true if a Snarl is part of a nontrivial chain of more than one
    /// snarl. Note that chain_of() still works for snarls in trivial chains.
    bool in_nontrivial_chain(const Snarl* here) const;
        
    /// Get all the snarls in all the chains under the given parent snarl.
    /// If the parent snarl is null, gives the top-level chains that connect and contain the top-level root snarls.
    /// Unary snarls and snarls in trivial chains will be presented as their own chains.
    /// Snarls are not necessarily oriented appropriately given their ordering in the chain.
    /// Useful for making a net graph.
    const deque<Chain>& chains_of(const Snarl* snarl) const;
        
    /// Get the net graph of the given Snarl's contents, using the given
    /// backing HandleGraph. If use_internal_connectivity is false, each
    /// chain and unary child snarl is treated as an ordinary node which is
    /// assumed to be only traversable from one side to the other.
    /// Otherwise, traversing the graph works like it would if you actually
    /// went through the internal graphs fo child snarls.
    NetGraph net_graph_of(const Snarl* snarl, const HandleGraph* graph, bool use_internal_connectivity = true) const;
        
    /// Returns true if snarl has no children and false otherwise
    bool is_leaf(const Snarl* snarl) const;
        
    /// Returns true if snarl has no parent and false otherwise
    bool is_root(const Snarl* snarl) const;

    /// Returns true if the snarl is trivial (an ultrabubble with just the
    /// start and end nodes) and false otherwise.
    /// TODO: Implement without needing the vg graph, by adding a flag to trivial snarls.
    bool is_trivial(const Snarl* snarl, const HandleGraph& graph) const;
    
    /// Returns true if the snarl lacks any nontrivial children.
    bool all_children_trivial(const Snarl* snarl, const HandleGraph& graph) const;

    /// Returns a reference to a vector with the roots of the Snarl trees
    const vector<const Snarl*>& top_level_snarls() const;
        
    /// Returns the Nodes and Edges contained in this Snarl but not in any child Snarls (always includes the
    /// Nodes that form the boundaries of child Snarls, optionally includes this Snarl's own boundary Nodes)
    pair<unordered_set<nid_t>, unordered_set<edge_t> > shallow_contents(const Snarl* snarl, const HandleGraph& graph,
                                                                       bool include_boundary_nodes) const;
        
    /// Returns the Nodes and Edges contained in this Snarl, including those in child Snarls (optionally
    /// includes Snarl's own boundary Nodes)
    pair<unordered_set<nid_t>, unordered_set<edge_t> > deep_contents(const Snarl* snarl, const HandleGraph& graph,
                                                                    bool include_boundary_nodes) const;
        
    /// Look left from the given visit in the given graph and gets all the
    /// attached Visits to nodes or snarls.
    vector<Visit> visits_left(const Visit& visit, const HandleGraph& graph, const Snarl* in_snarl) const;
        
    /// Look left from the given visit in the given graph and gets all the
    /// attached Visits to nodes or snarls.
    vector<Visit> visits_right(const Visit& visit, const HandleGraph& graph, const Snarl* in_snarl) const;
        
    /// Returns a map from all Snarl boundaries to the Snarl they point into. Note that this means that
    /// end boundaries will be reversed.
    unordered_map<pair<int64_t, bool>, const Snarl*> snarl_boundary_index() const;
        
    /// Returns a map from all Snarl start boundaries to the Snarl they point into.
    unordered_map<pair<int64_t, bool>, const Snarl*> snarl_start_index() const;
        
    /// Returns a map from all Snarl end boundaries to the Snarl they point into. Note that this means that
    /// end boundaries will be reversed.
    unordered_map<pair<int64_t, bool>, const Snarl*> snarl_end_index() const;
        
    /// Execute a function on all top level sites
    void for_each_top_level_snarl(const function<void(const Snarl*)>& lambda) const;
        
    /// Execute a function on all sites in a preorder traversal
    void for_each_snarl_preorder(const function<void(const Snarl*)>& lambda) const;
        
    /// Execute a function on all top level sites in parallel
    void for_each_top_level_snarl_parallel(const function<void(const Snarl*)>& lambda) const;
        
    /// Execute a function on all sites in parallel
    void for_each_snarl_parallel(const function<void(const Snarl*)>& lambda) const;

    /// Execute a function on all top level chains
    void for_each_top_level_chain(const function<void(const Chain*)>& lambda) const;

    /// Execute a function on all top level chains in parallel
    void for_each_top_level_chain_parallel(const function<void(const Chain*)>& lambda) const;

    /// Ececute a function on all chains
    void for_each_chain(const function<void(const Chain*)>& lambda) const;
    
    /// Ececute a function on all chains in parallel
    void for_each_chain_parallel(const function<void(const Chain*)>& lambda) const;

    /// Iterate over snarls as they are stored in deque<SnarlRecords>
    void for_each_snarl_unindexed(const function<void(const Snarl*)>& lambda) const;
        
    /// Given a Snarl that we don't own (like from a Visit), find the
    /// pointer to the managed copy of that Snarl.
    const Snarl* manage(const Snarl& not_owned) const;

    /// Sample snarls discrete uniformly 
    /// Returns a nullptr if no snarls are found 
    const Snarl* discrete_uniform_sample(minstd_rand0& random_engine)const;

    /// Count snarls in deque<SnarlRecords>, a master list of snarls in graph
    int num_snarls()const;

    ///Get the snarl number from the SnarlRecord* member with given snarl
    inline size_t snarl_number(const Snarl* snarl) const{
        const SnarlRecord* record = SnarlManager::record(snarl);
        return record->snarl_number;
    }
    //use the snarl number to access the Snarl*
    inline const Snarl* translate_snarl_num(size_t snarl_num){
        return unrecord(&snarls.at(snarl_num));
    }

        
private:
    
    /// To support the Snarl*-driven API, we use a struct that lays out a snarl
    /// followed by indexing metadata, one after the other in memory. We can
    /// just cast a Snarl* to a pointer to one of these to get access to all
    /// the metadata.
    struct alignas(alignof(Snarl)) SnarlRecord {
        /// With recent Protobuf, we can't inherit from Protobuf generated
        /// classes, so we rely on the first member here being at offset 0.
        /// This is achieved by making sure SnarlRecord is aligned like Snarl. 
        Snarl snarl;
        
        /// This is a vector of pointers into the master snarl container at
        /// children. We know the pointers are to valid SnarlRecords. A
        /// SnarlRecord does not own its children.
        vector<const Snarl*> children;
        
        /// This holds chains over the child snarls.
        deque<Chain> child_chains;
        
        /// This points to the parent SnarlRecord (as a snarl), or null if we
        /// are a root snarl or have not been told of our parent yet.
        const Snarl* parent = nullptr;
        
        /// This points to the chain we are in, or null if we are not in a chain.
        Chain* parent_chain = nullptr;
        /// And this is what index we are at in the chain;
        size_t parent_chain_index = 0;

        /// This holds the index of the SnarlRecord* in the deque
        /// We are doing this because a deque is not contiguous and the index lookup using a SnarlRecord* isn't easily derivable 
        size_t snarl_number;

        /// Allow assignment from a Snarl object, fluffing it up into a full SnarlRecord
        SnarlRecord& operator=(const Snarl& other) {
            // Just call the base assignment operator
            (*(Snarl*)this) = other;
            return *this;
        }
    };
    
    /// Get the const SnarlRecord for a const managed snarl
    inline const SnarlRecord* record(const Snarl* snarl) const {
        return (const SnarlRecord*) snarl;
    }
    
    /// Get the SnarlRecord for a managed snarl
    inline SnarlRecord* record(Snarl* snarl) {
        return (SnarlRecord*) snarl;
    }
    
    /// Get the const Snarl owned by a const SnarlRecord
    inline const Snarl* unrecord(const SnarlRecord* record) const {
        return (const Snarl*) record;
    }
    
    /// Get the Snarl owned by a SnarlRecord
    inline Snarl* unrecord(SnarlRecord* record) {
        return (Snarl*) record;
    }
    

    /// Master list of the snarls in the graph.
    /// Use a deque so pointers never get invalidated but we still have some locality.
    deque<SnarlRecord> snarls;
        
    /// Roots of snarl trees
    vector<const Snarl*> roots;
    /// Chains of root-level snarls. Uses a deque so Chain* pointers don't get invalidated.
    deque<Chain> root_chains;
        
    /// Map of node traversals to the snarls they point into
    unordered_map<pair<int64_t, bool>, const Snarl*> snarl_into;
        
    /// Builds tree indexes after Snarls have been added to the snarls vector
    void build_indexes();
        
    /// Actually compute chains for a set of already indexed snarls, which
    /// is important when chains were not provided. Returns the chains.
    deque<Chain> compute_chains(const vector<const Snarl*>& input_snarls);
    
    /// Modify the snarls and chains to enforce a couple of invariants:
    ///
    /// 1. The start node IDs of the snarls in a chain shall be unique.
    ///
    /// (This is needed by the distance indexing code, which identifies child
    /// snarls by their start nodes. TODO: That distance indexing code needs to
    /// also work out unary snarls abitting the ends of chains, which may be
    /// allowed eventually.)
    ///
    /// 2. Snarls will be oriented forward in their chains.
    ///
    /// 3. Snarls will be oriented in a chain to maximize the number of snarls
    /// that start with lower node IDs than they end with.
    ///
    /// Depends on the indexes from build_indexes() having been built.
    void regularize();
        
    // Chain computation uses these pseudo-chain-traversal functions, which
    // walk around based on the snarl boundary index. This basically gets
    // you chains, except for structures that look like circular chains,
    // which are actually presented as circular chains and not linear ones.
    // They also let you walk into unary snarls.
        
    /// Get a Visit to the snarl coming after the given Visit to a snarl, or
    /// a Visit with no Snarl no next snarl exists. Accounts for snarls'
    /// orientations.
    Visit next_snarl(const Visit& here) const;
        
    /// Get a Visit to the snarl coming before the given Visit to a snarl,
    /// or a Visit with no Snarl no previous snarl exists. Accounts for
    /// snarls' orientations.
    Visit prev_snarl(const Visit& here) const;
        
    /// Get the Snarl, if any, that shares this Snarl's start node as either
    /// its start or its end. Does not count this snarl, even if this snarl
    /// is unary. Basic operation used to traverse a chain. Caller must
    /// account for snarls' orientations within a chain.
    const Snarl* snarl_sharing_start(const Snarl* here) const;
        
    /// Get the Snarl, if any, that shares this Snarl's end node as either
    /// its start or its end. Does not count this snarl, even if this snarl
    /// is unary. Basic operation used to traverse a chain. Caller must
    /// account for snarls' orientations within a chain.
    const Snarl* snarl_sharing_end(const Snarl* here) const;
};

template <typename SnarlIterator>
SnarlManager::SnarlManager(SnarlIterator begin, SnarlIterator end) {
    // add snarls to master list
    for (auto iter = begin; iter != end; ++iter) {
        add_snarl(*iter);
    }
    // record the tree structure and build the other indexes
    finish();
}

}

#endif

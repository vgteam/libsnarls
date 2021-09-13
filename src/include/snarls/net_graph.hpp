#ifndef LIBSNARLS_NET_GRAPH_HPP_INCLUDED
#define LIBSNARLS_NET_GRAPH_HPP_INCLUDED

#include <handlegraph/handle_graph.hpp>

#include <vg/vg.pb.h>

#include <unordered_set>
#include <unordered_map>

namespace snarls {

using namespace std;
using namespace vg;
using namespace handlegraph;

/**
 * Allow traversing a graph of nodes and child snarl chains within a snarl
 * within another HandleGraph. Uses its own internal child index because
 * it's used in the construction of snarls to feed to SnarlManagers.
 *
 * Assumes that the snarls in the chains we get are in the order they
 * occur in the graph.
 *
 * We adapt the handle graph abstraction as follows:
 *
 * A chain becomes a single node with the ID and local forward orientation
 * of its first snarl's start.
 *
 * A chain node connects on its left to everything connected to its first
 * start and on its right to everything connected to its last end.
 *
 * A unary snarl becomes a single node, too. It is identified by its
 * boundary node's ID.
 *
 * If you're not using internal connectivity, a chain node or a unary snarl
 * node behaves just like an ordinary node.
 *
 * If you are using internal connectivity, edges are slightly faked:
 *
 * A chain node also sees out its right everything that is out its left if
 * it has a left-left connected snarl before any disconnected snarl.
 *
 * And similarly for the mirror case.
 *
 * All the edges on either side of a unary snarl node are the same.
 *
 * In this part of the code we talk about "heads" (the inward-facing base
 * graph handles used to represent child snarls/chains), and "tails" (the
 * inward-facing ending handles of child chains).
 * 
 */
class NetGraph : public HandleGraph {
public:
        
    /// Make a new NetGraph for the given snarl in the given backing graph,
    /// using the given chains as child chains. Unary snarls are stored as
    /// trivial chains just like other trivial chains.
    template<typename ChainContainer>
    NetGraph(const Visit& start, const Visit& end,
             const ChainContainer& child_chains_mixed,
             const HandleGraph* graph,
             bool use_internal_connectivity = false) : NetGraph(start, end, graph, use_internal_connectivity) {
            
        // All we need to do is index the children. They come mixed as real chains and unary snarls.
            
        for (auto& chain : child_chains_mixed) {
            if (chain.size() == 1 && chain.front().first->type() == UNARY) {
                // This is a unary snarl wrapped in a chain
                add_unary_child(chain.front().first);
            } else {
                // This is a real (but possibly singlr-snarl) chain
                add_chain_child(chain);
            }
        }
            
    }
        
    /// Make a net graph from the given chains and unary snarls (as pointers) in the given backing graph.
    template<typename ChainContainer, typename SnarlContainer>
    NetGraph(const Visit& start, const Visit& end,
             const ChainContainer& child_chains,
             const SnarlContainer& child_unary_snarls, const HandleGraph* graph,
             bool use_internal_connectivity = false) : NetGraph(start, end, graph, use_internal_connectivity) {
            
        // All we need to do is index the children.
        for (const Snarl* unary : child_unary_snarls) {
            add_unary_child(unary);
        }
            
        for (auto& chain : child_chains) {
            add_chain_child(chain);
        }
    }
            
    /// Make a net graph from the given chains and unary snarls (as raw values) in the given backing graph.
    /// Mostly for testing.
    NetGraph(const Visit& start, const Visit& end,
             const vector<vector<pair<Snarl, bool>>>& child_chains,
             const vector<Snarl>& child_unary_snarls,
             const HandleGraph* graph,
             bool use_internal_connectivity = false);

    /// Method to check if a node exists by ID
    virtual bool has_node(id_t node_id) const;
    
    /// Look up the handle for the node with the given ID in the given orientation
    virtual handle_t get_handle(const id_t& node_id, bool is_reverse = false) const;
        
    /// Get the ID from a handle
    virtual id_t get_id(const handle_t& handle) const;
        
    /// Get the orientation of a handle
    virtual bool get_is_reverse(const handle_t& handle) const;
        
    /// Invert the orientation of a handle (potentially without getting its ID)
    virtual handle_t flip(const handle_t& handle) const;
        
    /// Get the length of a node
    virtual size_t get_length(const handle_t& handle) const;
        
    /// Get the sequence of a node, presented in the handle's local forward
    /// orientation.
    virtual string get_sequence(const handle_t& handle) const;
        
    /// Loop over all the handles to next/previous (right/left) nodes. Passes
    /// them to a callback which returns false to stop iterating and true to
    /// continue. Returns true if we finished and false if we stopped early.
    virtual bool follow_edges_impl(const handle_t& handle, bool go_left, const function<bool(const handle_t&)>& iteratee) const;
        
    /// Loop over all the nodes in the graph in their local forward
    /// orientations, in their internal stored order. Stop if the iteratee returns false.
    virtual bool for_each_handle_impl(const function<bool(const handle_t&)>& iteratee, bool parallel = false) const;
        
    /// Return the number of nodes in the graph
    virtual size_t get_node_count() const;
    
    /// Return the smallest ID used. 
    virtual id_t min_node_id() const;
    
    /// Return the largest ID used.
    virtual id_t max_node_id() const;
        
    // We also have some extra functions
        
    /// Get the inward-facing start handle for this net graph. Useful when
    /// working with traversals.
    const handle_t& get_start() const;vector
        
    /// Get the outward-facing end handle for this net graph. Useful when
    /// working with traversals.
    const handle_t& get_end() const;
        
    /// Returns true if the given handle represents a meta-node for a child
    /// chain or unary snarl, and false if it is a normal node actually in
    /// the net graph snarl's contents.
    bool is_child(const handle_t& handle) const;
    
    /// Get the handle in the backing graph reading into the child chain or
    /// unary snarl in the orientation represented by this handle to a node
    /// representing a child chain or unary snarl.
    handle_t get_inward_backing_handle(const handle_t& child_handle) const;
    
    /// Given a handle to a node in the backing graph that reads into a child
    /// chain or snarl (in either direction), get the handle in this graph used
    /// to represent that child chain or snarl in that orientation.
    handle_t get_handle_from_inward_backing_handle(const handle_t& backing_handle) const;
        
protected:
    
    /// Make a NetGraph without filling in any of the child indexes.
    NetGraph(const Visit& start, const Visit& end, const HandleGraph* graph, bool use_internal_connectivity = false);
    
    /// Add a unary child snarl to the indexes.
    void add_unary_child(const Snarl* unary);
        
    /// Add a chain of one or more non-unary snarls to the index.
    void add_chain_child(const Chain& chain);
    
    // Save the backing graph
    const HandleGraph* graph;
        
    // And the start and end handles that bound the snarl we are working on.
    handle_t start;
    handle_t end;
        
    // Should we use the internal connectivity of chain nodes and unary
    // snarl nodes?
    bool use_internal_connectivity;
    
    // We keep the unary snarl boundaries, reading in with the unary snarl
    // contents to the right.
    unordered_set<handle_t> unary_boundaries;
        
    // We keep a map from handles that enter the ends of chains to the
    // reverse handles to their fronts. Whenever the backing graph tells us
    // to emit the one, we emit the other instead. This makes them look like
    // one big node.
    unordered_map<handle_t, handle_t> chain_end_rewrites;
        
    // We keep basically the reverse map, from chain start in chain forward
    // orientation to chain end in chain forward orientation. This lets us
    // find the edges off the far end of a chain.
    unordered_map<handle_t, handle_t> chain_ends_by_start;
        
    // Stores whether a chain or unary snarl, identified by the ID of its
    // start handle, is left-left, right-right, or left-right connected.
    unordered_map<id_t, tuple<bool, bool, bool>> connectivity;
        
};
    
}

#endif

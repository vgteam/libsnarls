#include "snarls/handle_graph_snarl_finder.hpp"
#include "snarls/snarl_manager.hpp"
#include "snarls/net_graph.hpp"

namespace snarls {

using namespace std;
using namespace vg;
using namespace handlegraph;

HandleGraphSnarlFinder::HandleGraphSnarlFinder(const HandleGraph* graph) : graph(graph) {
    // Nothing to do!
}

SnarlManager HandleGraphSnarlFinder::find_snarls_unindexed() {
    // Start with an empty SnarlManager
    SnarlManager snarl_manager;
    
    // We need a stack with the information we need to translate the traversal
    // into vg::Snarl and vg::Chain objects, so we can compute connectivity and
    // snarl classification as we go up.
    struct TranslationFrame {
        // This will hold the unmanaged scratch snarl we pass to the manager.
        Snarl snarl;
        // This will hold all the child snarls that need their parent information filled in before they can become managed.
        // They are sorted by chain.
        vector<vector<Snarl>> child_chains;
        // For creating the current chain for this frame, we need to know where the chain claimed to start.
        // If the start = the end and the chain is inside a snarl, it's just a trivial chain (single node) and we drop it.
        handle_t current_chain_start;
    };
    
    // Stack that lets us connect snarls to their parents.
    // Holds each snarl and the child snarls we have finished for it so far.
    vector<TranslationFrame> stack;
   
    traverse_decomposition([&](handle_t chain_start) {
        // We got the start of a (possibly empty) chain.
        if (!stack.empty()) {
            // We're in a snarl, so we're a chain that we need for snarl connectivity/classification.
            stack.back().current_chain_start = chain_start;
            
            // Allocate a place to store the snarls in the chain.
            stack.back().child_chains.emplace_back();
        }
    }, [&](handle_t chain_end) {
        // We got the end of a (possibly empty) chain.
        if (!stack.empty() && stack.back().current_chain_start == chain_end) {
            // We're an empty chain in an actual snarl.
            // Get rid of our empty chain vector that got no snarls in it
            assert(stack.back().child_chains.back().empty());
            stack.back().child_chains.pop_back();
        }
    }, [&](handle_t snarl_start) {
        // Stack up a snarl
        stack.emplace_back();
        // And fill in its start
        auto& snarl = stack.back().snarl;
        snarl.mutable_start()->set_node_id(graph->get_id(snarl_start));
        snarl.mutable_start()->set_backward(graph->get_is_reverse(snarl_start));
    }, [&](handle_t snarl_end) {
        // Fill in its end
        auto& snarl = stack.back().snarl;
        snarl.mutable_end()->set_node_id(graph->get_id(snarl_end));
        snarl.mutable_end()->set_backward(graph->get_is_reverse(snarl_end));
        
        // We need to manage all our children and put them in Chain objects that net graphs can understand.
        vector<Chain> managed_child_chains;
        
        for (auto& child_chain : stack.back().child_chains) {
            // For every child chain
            
            // Make a translated version
            managed_child_chains.emplace_back();
            for (auto& child : child_chain) {
                // For each child snarl, fill us in as the parent (before we have connectivity info filled in)
                *child.mutable_parent() = snarl;
                // And report it to the manager with the cross-reference to us filled in.
                const Snarl* managed_child = snarl_manager.add_snarl(child);
                // And save it in the child chain.
                // We know it must be forward in the chain.
                managed_child_chains.back().emplace_back(managed_child, false);
            }
        }
        
        // This snarl is real, we care about type and connectivity.
        // All its children are done.

        /////
        // Determine connectivity
        /////
        
        // Make a net graph for the snarl that uses internal connectivity
        NetGraph connectivity_net_graph(snarl.start(), snarl.end(), managed_child_chains, graph, true);
        
        // Evaluate connectivity
        // A snarl is minimal, so we know out start and end will be normal nodes.
        handle_t start_handle = connectivity_net_graph.get_handle(snarl.start().node_id(), snarl.start().backward());
        handle_t end_handle = connectivity_net_graph.get_handle(snarl.end().node_id(), snarl.end().backward());
        
        // Start out by assuming we aren't connected
        bool connected_start_start = false;
        bool connected_end_end = false;
        bool connected_start_end = false;
        
        // We do a couple of direcred walk searches to test connectivity.
        list<handle_t> queue{start_handle};
        unordered_set<handle_t> queued{start_handle};
        auto handle_edge = [&](const handle_t& other) {
#ifdef debug
            cerr << "\tCan reach " << connectivity_net_graph.get_id(other)
            << " " << connectivity_net_graph.get_is_reverse(other) << endl;
#endif
            
            // Whenever we see a new node orientation, queue it.
            if (!queued.count(other)) {
                queue.push_back(other);
                queued.insert(other);
            }
        };
        
#ifdef debug
        cerr << "Looking for start-start turnarounds and through connections from "
             << connectivity_net_graph.get_id(start_handle) << " " <<
            connectivity_net_graph.get_is_reverse(start_handle) << endl;
#endif
        
        while (!queue.empty()) {
            handle_t here = queue.front();
            queue.pop_front();
            
            if (here == end_handle) {
                // Start can reach the end
                connected_start_end = true;
            }
            
            if (here == connectivity_net_graph.flip(start_handle)) {
                // Start can reach itself the other way around
                connected_start_start = true;
            }
            
            if (connected_start_end && connected_start_start) {
                // No more searching needed
                break;
            }
            
            // Look at everything reachable on a proper rightward directed walk.
            connectivity_net_graph.follow_edges(here, false, handle_edge);
        }
        
        auto end_inward = connectivity_net_graph.flip(end_handle);
        
#ifdef debug
        cerr << "Looking for end-end turnarounds from " << connectivity_net_graph.get_id(end_inward)
             << " " << connectivity_net_graph.get_is_reverse(end_inward) << endl;
#endif
        
        // Reset and search the other way from the end to see if it can find itself.
        queue = {end_inward};
        queued = {end_inward};
        while (!queue.empty()) {
            handle_t here = queue.front();
            queue.pop_front();
            
#ifdef debug
            cerr << "Got to " << connectivity_net_graph.get_id(here) << " "
                 << connectivity_net_graph.get_is_reverse(here) << endl;
#endif
            
            if (here == end_handle) {
                // End can reach itself the other way around
                connected_end_end = true;
                break;
            }
            
            // Look at everything reachable on a proper rightward directed walk.
            connectivity_net_graph.follow_edges(here, false, handle_edge);
        }
        
        // Save the connectivity info. TODO: should the connectivity flags be
        // calculated based on just the net graph, or based on actual connectivity
        // within child snarls.
        snarl.set_start_self_reachable(connected_start_start);
        snarl.set_end_self_reachable(connected_end_end);
        snarl.set_start_end_reachable(connected_start_end);

#ifdef debug
        cerr << "Connectivity: " << connected_start_start << " " << connected_end_end << " " << connected_start_end << endl;
#endif

        /////
        // Determine tip presence
        /////
        
        // Make a net graph that just pretends child snarls/chains are ordinary nodes
        NetGraph flat_net_graph(snarl.start(), snarl.end(), managed_child_chains, graph);
        
        // Having internal tips in the net graph disqualifies a snarl from being an ultrabubble
        auto tips = handlealgs::find_tips(&flat_net_graph);

#ifdef debug
        cerr << "Tips: " << endl;
        for (auto& tip : tips) {
            cerr << "\t" << flat_net_graph.get_id(tip) << (flat_net_graph.get_is_reverse(tip) ? '-' : '+') << endl;
        }
#endif

        // We should have at least the bounding nodes.
        assert(tips.size() >= 2);
        bool has_internal_tips = (tips.size() > 2); 
        
        /////
        // Determine cyclicity/acyclicity
        /////
    
        // This definitely should be calculated based on the internal-connectivity-ignoring net graph.
        snarl.set_directed_acyclic_net_graph(handlealgs::is_directed_acyclic(&flat_net_graph));

        /////
        // Determine classification
        /////

        // Now we need to work out if the snarl can be a unary snarl or an ultrabubble or what.
        if (snarl.start().node_id() == snarl.end().node_id()) {
            // Snarl has the same start and end (or no start or end, in which case we don't care).
            snarl.set_type(UNARY);
#ifdef debug
            cerr << "Snarl is UNARY" << endl;
#endif
        } else if (!snarl.start_end_reachable()) {
            // Can't be an ultrabubble if we're not connected through.
            snarl.set_type(UNCLASSIFIED);
#ifdef debug
            cerr << "Snarl is UNCLASSIFIED because it doesn't connect through" << endl;
#endif
        } else if (snarl.start_self_reachable() || snarl.end_self_reachable()) {
            // Can't be an ultrabubble if we have these cycles
            snarl.set_type(UNCLASSIFIED);
            
#ifdef debug
            cerr << "Snarl is UNCLASSIFIED because it allows turning around, creating a directed cycle" << endl;
#endif

        } else {
            // See if we have all ultrabubble children
            bool all_ultrabubble_children = true;
            for (auto& chain : managed_child_chains) {
                for (auto& child : chain) {
                    if (child.first->type() != ULTRABUBBLE) {
                        all_ultrabubble_children = false;
                        break;
                    }
                }
                if (!all_ultrabubble_children) {
                    break;
                }
            }
            
            if (!all_ultrabubble_children) {
                // If we have non-ultrabubble children, we can't be an ultrabubble.
                snarl.set_type(UNCLASSIFIED);
#ifdef debug
                cerr << "Snarl is UNCLASSIFIED because it has non-ultrabubble children" << endl;
#endif
            } else if (has_internal_tips) {
                // If we have internal tips, we can't be an ultrabubble
                snarl.set_type(UNCLASSIFIED);
                
#ifdef debug
                cerr << "Snarl is UNCLASSIFIED because it contains internal tips" << endl;
#endif
            } else if (!snarl.directed_acyclic_net_graph()) {
                // If all our children are ultrabubbles but we ourselves are cyclic, we can't be an ultrabubble
                snarl.set_type(UNCLASSIFIED);
                
#ifdef debug
                cerr << "Snarl is UNCLASSIFIED because it is not directed-acyclic" << endl;
#endif
            } else {
                // We have only ultrabubble children and are acyclic.
                // We're an ultrabubble.
                snarl.set_type(ULTRABUBBLE);
#ifdef debug
                cerr << "Snarl is an ULTRABUBBLE" << endl;
#endif
            }
        }
        
        // Now we know all about our snarl, but we don't know about our parent.
        
        if (stack.size() > 1) {
            // We have a parent. Join it as a child, at the end of the current chain
            assert(!stack[stack.size() - 2].child_chains.empty());
            stack[stack.size() - 2].child_chains.back().emplace_back(std::move(snarl));
        } else {
            // Just manage ourselves now, because our parent can't manage us.
            snarl_manager.add_snarl(snarl);
        }
        
        // Leave the stack
        stack.pop_back();
    });
    
    // Give it back
    return snarl_manager;
}

SnarlManager HandleGraphSnarlFinder::find_snarls() {
    // Find all the snarls
    auto snarl_manager(find_snarls_unindexed());
    
    // Index them
    snarl_manager.finish();
    
    // Return the finished SnarlManager
    return snarl_manager;
}


}

#include "snarls/net_graph.hpp"

namespace snarls {

using namespace std;
using namespace vg;
using namespace handlegraph;


NetGraph::NetGraph(const Visit& start, const Visit& end, const HandleGraph* graph, bool use_internal_connectivity) :
    graph(graph),
    start(graph->get_handle(start.node_id(), start.backward())),
    end(graph->get_handle(end.node_id(), end.backward())),
    use_internal_connectivity(use_internal_connectivity) {
    // Nothing to do!
    
#ifdef debug
    cerr << "Creating net graph of " << graph->get_id(this->start) << (graph->get_is_reverse(this->start) ? "-" : "+")
        << "->" << graph->get_id(this->end) << (graph->get_is_reverse(this->end) ? "-" : "+") << endl;
#endif
    
}
    
NetGraph::NetGraph(const Visit& start, const Visit& end,
                   const vector<vector<pair<Snarl, bool>>>& child_chains,
                   const vector<Snarl>& child_unary_snarls,
                   const HandleGraph* graph,
                   bool use_internal_connectivity) :
                   NetGraph(start, end, graph, use_internal_connectivity) {
        
    for (auto& unary : child_unary_snarls) {
        add_unary_child(&unary);
    }
        
    for (auto& chain : child_chains) {
        Chain converted_chain;
        for (auto& item : chain) {
            // Convert from actual snarls to pointers
            converted_chain.emplace_back(&item.first, item.second);
        }
        add_chain_child(converted_chain);
    }
   
}
    
void NetGraph::add_unary_child(const Snarl* unary) {
    // For each unary snarl, make its bounding handle
    handle_t snarl_bound = graph->get_handle(unary->start().node_id(), unary->start().backward());
        
    // Get its ID
    id_t snarl_id = unary->start().node_id();
        
    // Make sure it is properly specified to be unary (in and out the same node in opposite directions)
    assert(unary->end().node_id() == snarl_id);
    assert(unary->end().backward() == !unary->start().backward());
        
    // Save it as a unary snarl
    unary_boundaries.insert(snarl_bound);
    
#ifdef debug
    cerr << "\tAdd unary child snarl on " << graph->get_id(snarl_bound) << (graph->get_is_reverse(snarl_bound) ? "-" : "+") << endl;
#endif
        
    if (use_internal_connectivity) {
        // Save its connectivity
        connectivity[snarl_id] = make_tuple(unary->start_self_reachable(), unary->end_self_reachable(),
                                            unary->start_end_reachable());
    } else {
        // Use the connectivity of an ordinary node that has a different
        // other side. Don't set connected_start_end because, for a real
        // unary snarl, the end and the start are the same, so that
        // means you can turn aroiund.
        connectivity[snarl_id] = make_tuple(false, false, false);
    }
}
    
void NetGraph::add_chain_child(const Chain& chain) {
    // For every chain, get its bounding handles in the base graph
    auto start_visit = get_start_of(chain);
    handle_t chain_start_handle = graph->get_handle(start_visit.node_id(), start_visit.backward());
    auto end_visit = get_end_of(chain);
    handle_t chain_end_handle = graph->get_handle(end_visit.node_id(), end_visit.backward());
        
    // Save the links that let us cross the chain.
    chain_ends_by_start[chain_start_handle] = chain_end_handle;
    chain_end_rewrites[graph->flip(chain_end_handle)] = graph->flip(chain_start_handle);
    
#ifdef debug
    cerr << "\tAdd child chain " << graph->get_id(chain_start_handle) << (graph->get_is_reverse(chain_start_handle) ? "-" : "+")
        << " -> " << graph->get_id(chain_end_handle) << (graph->get_is_reverse(chain_end_handle) ? "-" : "+") << endl;
#endif
        
    if (use_internal_connectivity) {
        
        // Determine child snarl connectivity.
        bool connected_left_left = false;
        bool connected_right_right = false;
        bool connected_left_right = true;
            
        for (auto it = chain_begin(chain); it != chain_end(chain); ++it) {
            // Go through the oriented child snarls from left to right
            const Snarl* child = it->first;
            bool backward = it->second;
                
            // Unpack the child's connectivity
            bool start_self_reachable = child->start_self_reachable();
            bool end_self_reachable = child->end_self_reachable();
            bool start_end_reachable = child->start_end_reachable();
                
            if (backward) {
                // Look at the connectivity in reverse
                std::swap(start_self_reachable, end_self_reachable);
            }
                
            if (start_self_reachable) {
                // We found a turnaround from the left
                connected_left_left = true;
            }
                
            if (!start_end_reachable) {
                // There's an impediment to getting through.
                connected_left_right = false;
                // Don't keep looking for turnarounds
                break;
            }
        }
            
        for (auto it = chain_rbegin(chain); it != chain_rend(chain); ++it) {
            // Go through the oriented child snarls from left to right
            const Snarl* child = it->first;
            bool backward = it->second;
                
            // Unpack the child's connectivity
            bool start_self_reachable = child->start_self_reachable();
            bool end_self_reachable = child->end_self_reachable();
            bool start_end_reachable = child->start_end_reachable();
                
            if (backward) {
                // Look at the connectivity in reverse
                std::swap(start_self_reachable, end_self_reachable);
            }
                
            if (end_self_reachable) {
                // We found a turnaround from the right
                connected_right_right = true;
                break;
            }
                
            if (!start_end_reachable) {
                // Don't keep looking for turnarounds
                break;
            }
        }
            
        // Save the connectivity
        connectivity[graph->get_id(chain_start_handle)] = tie(connected_left_left,
                                                              connected_right_right, connected_left_right);
    } else {
        // Act like a normal connected-through node.
        connectivity[graph->get_id(chain_start_handle)] = make_tuple(false, false, true);
    }
}

bool NetGraph::has_node(id_t node_id) const {
    return graph->has_node(node_id);
}
    
handle_t NetGraph::get_handle(const id_t& node_id, bool is_reverse) const {
    // We never let anyone see any node IDs that aren't assigned to child snarls/chains or content nodes.
    return graph->get_handle(node_id, is_reverse);
}
    
id_t NetGraph::get_id(const handle_t& handle) const {
    // We just use the handle/ID mapping of the backing graph
    return graph->get_id(handle);
}
    
bool NetGraph::get_is_reverse(const handle_t& handle) const {
    // We just use the handle/orientation mapping of the backing graph
    return graph->get_is_reverse(handle);
}
    
handle_t NetGraph::flip(const handle_t& handle) const {
    // We just use the flip implementation of the backing graph
    return graph->flip(handle);
}
    
size_t NetGraph::get_length(const handle_t& handle) const {
    // TODO: We don't really want to support this operation; maybe lengths
    // and sequences should be factored out into another interface.
    throw runtime_error("Cannot expose sequence lengths via NetGraph");
}
    
string NetGraph::get_sequence(const handle_t& handle) const {
    // TODO: We don't really want to support this operation; maybe lengths
    // and sequences should be factored out into another interface.
    throw runtime_error("Cannot expose sequences via NetGraph");
}
    
bool NetGraph::follow_edges_impl(const handle_t& handle, bool go_left, const function<bool(const handle_t&)>& iteratee) const {
    // Now we do the real work.
        
#ifdef debug
    cerr << "Look for edges in net graph of " << graph->get_id(start) << (graph->get_is_reverse(start) ? "-" : "+")
        << "->" << graph->get_id(end) << (graph->get_is_reverse(end) ? "-" : "+") << " on " << graph->get_id(handle) << (graph->get_is_reverse(handle) ? "-" : "+")
         << " going " << (go_left ? "left" : "right") << endl;
#endif
        
    // We also need to deduplicate edges. Maybe the start and end of a chain
    // connect to the same next node, and we could read out both traversing
    // the chain.
    unordered_set<handle_t> seen;
            
    // This deduplicates and emits the edges, and also handles rewriting
    // visits to the ends of chains as visits to the start, which we use to
    // represent the whole chain.
    auto handle_edge = [&](const handle_t& other) -> bool {
            
        handle_t real_handle = other;
        if (chain_end_rewrites.count(other)) {
            // We're reading into the end of a chain.
            
#ifdef debug
            cerr << "\tRead into chain end; warp to start" << endl;
#endif
            
            // Warp to the start.
            real_handle = chain_end_rewrites.at(other);
        } else if (chain_end_rewrites.count(graph->flip(other))) {
            // We're backing into the end of a chain.
            
#ifdef debug
            cerr << "\tBack into chain end; warp to start" << endl;
#endif
            
            // Warp to the start.
            real_handle = graph->flip(chain_end_rewrites.at(graph->flip(other)));
        }
            
#ifdef debug
        cerr << "\tFound edge " << (go_left ? "from " : "to ") << graph->get_id(other) << (graph->get_is_reverse(other) ? "-" : "+") << endl;
#endif
            
        if (!seen.count(real_handle)) {
            seen.insert(real_handle);
#ifdef debug
            cerr << "\t\tReport as " << graph->get_id(real_handle) << (graph->get_is_reverse(real_handle) ? "-" : "+") << endl;
#endif
                
            return iteratee(real_handle);
        } else {
#ifdef debug
            cerr << "\t\tEdge has been seen" << endl;
#endif
            return true;
        }
    };
        
    // This does the same as handle_edge, but flips the real handle
    auto flip_and_handle_edge = [&](const handle_t& other) -> bool {
            
        handle_t real_handle = other;
        if (chain_end_rewrites.count(other)) {
            // We're reading into the end of a chain.
#ifdef debug
        cerr << "\tRead into chain end; warp to start" << endl;
#endif
            // Warp to the start.
            real_handle = chain_end_rewrites.at(other);
        } else if (chain_end_rewrites.count(graph->flip(other))) {
            // We're backing into the end of a chain.
            
#ifdef debug
            cerr << "\tBack into chain end; warp to start" << endl;
#endif
            
            // Warp to the start.
            real_handle = graph->flip(chain_end_rewrites.at(graph->flip(other)));
        }
            
        real_handle = graph->flip(real_handle);
            
#ifdef debug
        cerr << "\tFound edge " << (go_left ? "from " : "to ") << graph->get_id(other) << (graph->get_is_reverse(other) ? "-" : "+") << endl;
#endif
            
        if (!seen.count(real_handle)) {
            seen.insert(real_handle);
#ifdef debug
            cerr << "\t\tReport as " << graph->get_id(real_handle) << (graph->get_is_reverse(real_handle) ? "-" : "+") << endl;
#endif
                
            return iteratee(real_handle);
        } else {
#ifdef debug
            cerr << "\t\tEdge has been seen" << endl;
#endif
            return true;
        }
    };
        
    // Each way of doing things needs to support going either left or right
        
    if (end != start &&
      ((handle == end && !go_left) || (handle == graph->flip(end) && go_left) ||
        (handle == graph->flip(start) && !go_left) || (handle == start && go_left))) {
        // If we're looking outside of the snarl this is the net graph for, don't admit to having any edges.
        //If start and end are the same, all edges are within the net graph
            
#ifdef debug
        cerr << "\tWe are at the bound of the graph so don't say anything" << endl;
#endif
        return true;
    }
        
    if (chain_ends_by_start.count(handle) || chain_ends_by_start.count(graph->flip(handle))) {
        // If we have an associated chain end for this start, we have to use chain connectivity to decide what to do.
            
#ifdef debug
        cerr << "\tWe are a chain node" << endl;
#endif
            
        bool connected_start_start;
        bool connected_end_end;
        bool connected_start_end;
        tie(connected_start_start, connected_end_end, connected_start_end) = connectivity.at(graph->get_id(handle));
            
#ifdef debug
        cerr << "\t\tConnectivity: " << connected_start_start << " " << connected_end_end << " " << connected_start_end << endl;
#endif
            
        if (chain_ends_by_start.count(handle)) {
            // We visit the chain in its forward orientation
                
#ifdef debug
            cerr << "\t\tWe are visiting the chain forward" << endl;
#endif
                
            if (go_left) {
                // We want predecessors.
                // So we care about end-end connectivity (how could we have left our end?)
                    
#ifdef debug
                cerr << "\t\t\tWe are going left from a forward chain" << endl;
#endif
                    
                if (connected_end_end) {
                    
#ifdef debug
                    cerr << "\t\t\t\tWe can reverse and go back out the end" << endl;
#endif
                    
                    // Anything after us but in its reverse orientation could be our predecessor
                    // But a thing after us may be a chain, in which case we need to find its head before flipping.
                    if (!graph->follow_edges(chain_ends_by_start.at(handle), false, flip_and_handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                    
                if (connected_start_end) {
                    
#ifdef debug
                    cerr << "\t\t\t\tWe can continue through and go out the start" << endl;
#endif
                    
                    // Look left out of the start of the chain (which is the handle we really are on)
                    if (!graph->follow_edges(handle, true, handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                    
            } else {
                // We want our successors
                    
#ifdef debug
                cerr << "\t\t\tWe are going right from a forward chain" << endl;
#endif
                    
                if (connected_start_start) {
                    
#ifdef debug
                    cerr << "\t\t\t\tWe can reverse and go back out the start" << endl;
#endif
                    
                    // Anything before us but in its reverse orientation could be our successor
                    // But a thing before us may be a chain, in which case we need to find its head before flipping.
                    if (!graph->follow_edges(handle, true, flip_and_handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                    
                if (connected_start_end) {
                    
#ifdef debug
                    cerr << "\t\t\t\tWe can continue through and go out the end" << endl;
#endif
                    
                    // Look right out of the end of the chain (which is the handle we really are on)
                    if (!graph->follow_edges(chain_ends_by_start.at(handle), false, handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                    
            }
                
        } else {
            // We visit the chain in its reverse orientation.
            // Just flip the cases of above and reverse all the emitted orientations.
                
#ifdef debug
            cerr << "\t\tWe are visiting the chain in reverse" << endl;
#endif

            if (go_left) {
                // We want predecessors of the reverse version (successors, but flipped)
                    
#ifdef debug
                cerr << "\t\t\tWe are going left from a reverse chain" << endl;
#endif
                    
                if (connected_start_start) {
                    
#ifdef debug
                    cerr << "\t\t\t\tWe can reverse and go back out the start" << endl;
#endif
                    
                    if (!graph->follow_edges(handle, false, flip_and_handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                    
                if (connected_start_end) {
                    
#ifdef debug
                    cerr << "\t\t\t\tWe can continue through and go out the end" << endl;
#endif
                    
                    if (!graph->follow_edges(chain_ends_by_start.at(graph->flip(handle)), false, flip_and_handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                    
            } else {
                // We want successors of the reverse version (predecessors, but flipped)
                    
#ifdef debug
                cerr << "\t\t\tWe are going right from a reverse chain" << endl;
#endif
                    
                if (connected_end_end) {
                    
#ifdef debug
                    cerr << "\t\t\t\tWe can reverse and go back out the end" << endl;
#endif
                    
                    if (!graph->follow_edges(chain_ends_by_start.at(graph->flip(handle)), false, handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                    
                if (connected_start_end) {
                    
#ifdef debug
                    cerr << "\t\t\t\tWe can continue through and go out the start" << endl;
#endif
                    
                    if (!graph->follow_edges(handle, false, handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                    
            }
                
        }
            
        return true;
    }
        
    if (unary_boundaries.count(handle) || unary_boundaries.count(graph->flip(handle))) {
        // We are dealign with a node representing a unary child snarl.
            
#ifdef debug
        cerr << "\tWe are looking at a unary snarl" << endl;
#endif
            
        // We have to use chain connectivity to decide what to do.
        bool connected_start_start;
        bool connected_end_end;
        bool connected_start_end;
        tie(connected_start_start, connected_end_end, connected_start_end) = connectivity.at(graph->get_id(handle));
            
        if (unary_boundaries.count(handle)) {
            // We point into a unary snarl
            if (go_left) {
                // We want the predecessors
                    
                if (!use_internal_connectivity) {
                    // We treat this as a normal node
                    // Get the real predecessors
                    if (!graph->follow_edges(handle, true, handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                // Otherwise just think about what we can traverse to (i.e. nothing)
                    
                // Can't read a forward oriented unary boundary as a
                // predecessor, so no need to support read through.
                    
            } else {
                // We want the successors
                    
                // No real successors
                    
                if (connected_start_start || connected_end_end || connected_start_end) {
                    // Successors also include our predecessors but backward
                    if (!graph->follow_edges(handle, true, flip_and_handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                        
                }
            }
        } else {
            // We point out of a unary snarl.
            // Reverse of above. Sort of.
            if (go_left) {
                if (connected_start_start || connected_end_end || connected_start_end) {
                    if (!graph->follow_edges(handle, false, flip_and_handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                        
                }
                    
            } else {
                if (!use_internal_connectivity) {

                    if (!graph->follow_edges(handle, false, handle_edge)) {
                        // Iteratee is done
                        return false;
                    }
                }
                
            }
            
        }
            
        return true;
    }
        
#ifdef debug
    cerr << "\tWe are an ordinary node" << endl;
#endif
        
    // Otherwise, this is an ordinary snarl content node
    return graph->follow_edges(handle, go_left, handle_edge);
}

bool NetGraph::for_each_handle_impl(const function<bool(const handle_t&)>& iteratee, bool parallel) const {
    // Find all the handles by a traversal.
        
#ifdef debug
    cerr << "Look for contents of net graph of " << graph->get_id(start) << (graph->get_is_reverse(start) ? "-" : "+")
        << "->" << graph->get_id(end) << (graph->get_is_reverse(end) ? "-" : "+") << endl;
#endif
        
    // We have to do the traversal on the underlying backing graph, because
    // the traversal functions we implemented on the graph we present will
    // maybe use internal child snarl connectivity, which can mean parts of
    // the graph are in this snarl but not actually reachable.
        
    // We let both the starts and ends of child snarls into the queue.
    // But we'll only reveal the starts to our iteratee.
    list<handle_t> queue;
    unordered_set<id_t> queued;
        
    // We define a function to queue up nodes we could visit next
    auto see_node = [&](const handle_t& other) {
        // Whenever we see a new node, add it to the queue
        auto found = queued.find(graph->get_id(other));
        if (found == queued.end()) {

#ifdef debug
            cerr << "\t\t\tFound new contained node " << graph->get_id(other) << (graph->get_is_reverse(other) ? "-" : "+") << endl;
#endif
        
            queue.push_back(other);
            queued.emplace_hint(found, graph->get_id(other));
        }
    };
        
    // Start at both the start and the end of the snarl.
    see_node(start);
    see_node(end);
        
    while (!queue.empty()) {
        handle_t here = queue.front();
        queue.pop_front();
            
#ifdef debug
        cerr << "\tVisit node " << graph->get_id(here) << (graph->get_is_reverse(here) ? "-" : "+") << endl;
#endif
            
        if (unary_boundaries.count(graph->flip(here))) {
            // This is a backward unary child snarl head, so we need to look at it the other way around.
            here = graph->flip(here);
            
#ifdef debug
            cerr << "\t\tReverse to match unary child boundary" << endl;
#endif
            
        } else if (chain_ends_by_start.count(graph->flip(here))) {
            // This is a backward child chain head, so we need to look at it the other way around.
            here = graph->flip(here);
            
#ifdef debug
            cerr << "\t\tReverse to match child chain head" << endl;
#endif
            
        } else if (chain_end_rewrites.count(graph->flip(here))) {
            // This is a backward child chain tail, so we need to look at it the other way around.
            here = graph->flip(here);
            
#ifdef debug
            cerr << "\t\tReverse to match child chain tail" << endl;
#endif
            
        }
            
        if (!chain_end_rewrites.count(here)) {
            // This is not a chain end, so it's either a real contained node or a chain head.
            // We can emit it.
            
#ifdef debug
            cerr << "\t\tVisit forward version" << endl;
#endif
                
            if (graph->get_is_reverse(here)) {
                if (!iteratee(graph->flip(here))) {
                    // Run the iteratee on the forward version, and stop if it wants to stop
                    return false;
                }
            } else {
                if (!iteratee(here)) {
                    // Run the iteratee, and stop if it wants to stop.
                    return false;
                }
            }
                
        } else {
#ifdef debug
            cerr << "\t\tSkip chain end but see start at " << graph->get_id(chain_end_rewrites.at(here)) << (graph->get_is_reverse(chain_end_rewrites.at(here)) ? "-" : "+") << endl;
#endif
            // If we reach a chain end, make sure to eventually visit the chain start.
            // There might not be any other edges to it.
            see_node(chain_end_rewrites.at(here));
        }
            
        // We already have flipped any backward heads or tails frontward. So
        // we don't need to check if the backward version of us is in
        // anything.
            
        if ( ((start != end && here != end && here != graph->flip(start)) ||
               start == end)
            && !unary_boundaries.count(here) &&
            !chain_ends_by_start.count(here)  && !chain_end_rewrites.count(here)) {
            
#ifdef debug
            cerr << "\t\tRight side faces into net graph" << endl;
#endif
                
            // We have normal graph to our right and not the exterior of this snarl or the interior of a child.
            graph->follow_edges(here, false, see_node);
        }
            
        if ((start != end && here != start && here != graph->flip(end)) ||
             start == end) {
             
#ifdef debug
            cerr << "\t\tLeft side faces into net graph" << endl;
#endif
             
            // We have normal graph to our left.
            graph->follow_edges(here, true, see_node);
        }
            
        if (chain_end_rewrites.count(here)) {
        
#ifdef debug
            cerr << "\t\tWe are chain end; look right off reverse start at " << graph->get_id(chain_end_rewrites.at(here)) << (graph->get_is_reverse(chain_end_rewrites.at(here)) ? "-" : "+") << endl;
#endif
        
            // We need to look right off the reverse head of this child snarl.
            graph->follow_edges(chain_end_rewrites.at(here), false, see_node);
        }
            
        if (chain_ends_by_start.count(here)) {
        
#ifdef debug
            cerr << "\t\tWe are chain start; look right off end at " << graph->get_id(chain_ends_by_start.at(here)) << (graph->get_is_reverse(chain_ends_by_start.at(here)) ? "-" : "+") << endl;
#endif
        
            // We need to look right off the (reverse) tail of this child snarl.
            graph->follow_edges(chain_ends_by_start.at(here), false, see_node);
        }
    }
    
    return true;
}
    
size_t NetGraph::get_node_count() const {
    // TODO: this is inefficient!
    size_t size = 0;
    for_each_handle([&](const handle_t& ignored) {
            size++;
        });
    return size;
}

id_t NetGraph::min_node_id() const {
    // TODO: this is inefficient!
    id_t winner = numeric_limits<id_t>::max();
    for_each_handle([&](const handle_t& handle) {
            winner = min(winner, this->get_id(handle));
        });
    return winner;
}

id_t NetGraph::max_node_id() const {
    // TODO: this is inefficient!
    id_t winner = numeric_limits<id_t>::min();
    for_each_handle([&](const handle_t& handle) {
            winner = max(winner, this->get_id(handle));
        });
    return winner;
}
    
const handle_t& NetGraph::get_start() const {
    return start;
}
    
const handle_t& NetGraph::get_end() const {
    return end;
}
    
bool NetGraph::is_child(const handle_t& handle) const {
    // It's a child if we're going forward or backward through a chain, or into a unary snarl.
    
#ifdef debug
    cerr << "Is " << graph->get_id(handle) << " " << graph->get_is_reverse(handle) << " a child?" << endl;
    
    for (auto& kv : chain_ends_by_start) {
        cerr << "\t" << graph->get_id(kv.first) << " " << graph->get_is_reverse(kv.first) << " is a child." << endl;
    }
    
    for (auto& kv : chain_ends_by_start) {
        auto flipped = graph->flip(kv.first);
        cerr << "\t" << graph->get_id(flipped) << " " << graph->get_is_reverse(flipped) << " is a child." << endl;
    }
    
    for (auto& boundary : unary_boundaries) {
        cerr << "\t" << graph->get_id(boundary) << " " << graph->get_is_reverse(boundary) << " is a child." << endl;
    }
#endif
    
    return chain_ends_by_start.count(handle) ||
        chain_ends_by_start.count(flip(handle)) ||
        unary_boundaries.count(handle);
}

handle_t NetGraph::get_inward_backing_handle(const handle_t& child_handle) const {
    if (chain_ends_by_start.count(child_handle)) {
        // Reading into a chain, so just return this
        return child_handle;
    } else if (chain_ends_by_start.count(flip(child_handle))) {
        // Reading out of a chain, so get the outward end of the chain and flip it
        return graph->flip(chain_ends_by_start.at(flip(child_handle)));
    } else if (unary_boundaries.count(child_handle)) {
        // Reading into a unary snarl.
        // Always already facing inward.
        // TODO: what if we're reading out of a chain *and* into a unary snarl?
        return child_handle;
    } else {
        throw runtime_error("Cannot get backing handle for a handle that is not a handle to a child's node in the net graph");
    }
}

handle_t NetGraph::get_handle_from_inward_backing_handle(const handle_t& backing_handle) const {
    if (chain_ends_by_start.count(backing_handle)) {
        // If it's a recorded chain start it gets passed through
        return backing_handle;
    } else if (chain_end_rewrites.count(backing_handle)) {
        // If it's a known chain end, we produce the start in reverse orientation, which we stored.
        return chain_end_rewrites.at(backing_handle);
    } else if (unary_boundaries.count(backing_handle)) {
        // Unary snarl handles are passed through too.
        return backing_handle;
    } else {
        throw runtime_error("Cannot assign backing handle to a child chain or unary snarl");
    }
}

}

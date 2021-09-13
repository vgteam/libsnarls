#include "snarls/snarl_manager.hpp"

namespace snarls {

using namespace std;
using namespace vg;
using namespace handlegraph;

SnarlManager::SnarlManager(istream& in) : SnarlManager([&in](const function<void(Snarl&)>& consume_snarl) -> void {
    // Find all the snarls in the input stream and use each of them in the callback-based constructor
    for (vg::io::ProtobufIterator<Snarl> iter(in); iter.has_current(); iter.advance()) {
        consume_snarl(*iter);
    }
}) {
    // Nothing to do!
}

SnarlManager::SnarlManager(const function<void(const function<void(Snarl&)>&)>& for_each_snarl) {
    for_each_snarl([&](Snarl& snarl) {
        // Add each snarl to us
        add_snarl(snarl);
    });
    // Record the tree structure and build the other indexes
    finish();
}

void SnarlManager::serialize(ostream& out) const {
    
    vg::io::ProtobufEmitter<Snarl> emitter(out);
    list<const Snarl*> stack;

    for (const Snarl* root : top_level_snarls()) {
        stack.push_back(root);
        
        while (!stack.empty()) {
            // Grab a snarl from the stack
            const Snarl* snarl = stack.back();
            stack.pop_back();
            
            // Write out the snarl
            emitter.write_copy(*root);

            for (const Snarl* child_snarl : children_of(snarl)) {
                // Stack up its children
                stack.push_back(child_snarl);
            }
        }
    }
}
    
const vector<const Snarl*>& SnarlManager::children_of(const Snarl* snarl) const {
    if (snarl == nullptr) {
        // Looking for top level snarls
        return roots;
    }
    return record(snarl)->children;
}
    
const Snarl* SnarlManager::parent_of(const Snarl* snarl) const {
    return record(snarl)->parent;
}
    
const Snarl* SnarlManager::snarl_sharing_start(const Snarl* here) const {
    // Look out the start and see what we come to
    const Snarl* next = into_which_snarl(here->start().node_id(), !here->start().backward());
        
    // Return it, unless it's us, in which case we're a unary snarl that should go nowhere.
    return next == here ? nullptr : next;
        
}

    
const Snarl* SnarlManager::snarl_sharing_end(const Snarl* here) const {
    // Look out the end and see what we come to
    const Snarl* next = into_which_snarl(here->end().node_id(), here->end().backward());
        
    // Return it, unless it's us, in which case we're a unary snarl that should go nowhere.
    return next == here ? nullptr : next;
}
    
const Chain* SnarlManager::chain_of(const Snarl* snarl) const {
    return record(snarl)->parent_chain;
}

bool SnarlManager::chain_orientation_of(const Snarl* snarl) const { 
    const Chain* chain = chain_of(snarl);
    if (chain != nullptr) {
        // Go get the orientation flag.
        return chain->at(record(snarl)->parent_chain_index).second;
    }
    return false;
}

size_t SnarlManager::chain_rank_of(const Snarl* snarl) const { 
    const Chain* chain = chain_of(snarl);
    if (chain != nullptr) {
        // The index is a perfectly good rank.
        return record(snarl)->parent_chain_index;
    }
    // If you're in a single-snarl chain you are at index 0.
    return 0;
}

bool SnarlManager::in_nontrivial_chain(const Snarl* here) const {
    return chain_of(here)->size() > 1;
}
    
Visit SnarlManager::next_snarl(const Visit& here) const {
    // Must be a snarl visit
    assert(here.node_id() == 0);
    const Snarl* here_snarl = manage(here.snarl());
    assert(here_snarl != nullptr);
        
    // What visit are we going to return?
    Visit to_return;
        
    // And what snarl are we visiting next?
    const Snarl* next = here.backward() ? snarl_sharing_start(here_snarl) : snarl_sharing_end(here_snarl);

    if (next == nullptr) {
        // Nothing next
        return to_return;
    }
        
    // Fill in the start and end of the next snarl;
    transfer_boundary_info(*next, *to_return.mutable_snarl());
        
    if (here.backward()) {
        // We came out our start. So the next thing is also backward as long as its end matches our start. 
        to_return.set_backward(next->end().node_id() == here_snarl->start().node_id());
    } else {
        // We came out our end. So the next thing is backward if its start doesn't match our end.
        to_return.set_backward(next->start().node_id() != here_snarl->end().node_id());
    }
    
    return to_return;
}
    
Visit SnarlManager::prev_snarl(const Visit& here) const {
    return reverse(next_snarl(reverse(here)));
}
    
const deque<Chain>& SnarlManager::chains_of(const Snarl* snarl) const {
    if (snarl == nullptr) {
        // We want the root chains
        return root_chains;
    }
    
    // Otherwise, go look up the child chains of this snarl.
    return record(snarl)->child_chains;
}
    
NetGraph SnarlManager::net_graph_of(const Snarl* snarl, const HandleGraph* graph, bool use_internal_connectivity) const {
    // Just get the chains and forward them on to the NetGraph.
    // TODO: The NetGraph ends up computing its own indexes.
    return NetGraph(snarl->start(), snarl->end(), chains_of(snarl), graph, use_internal_connectivity);
}
    
bool SnarlManager::is_leaf(const Snarl* snarl) const {
    return record(snarl)->children.size() == 0;
}
    
bool SnarlManager::is_root(const Snarl* snarl) const {
    return parent_of(snarl) == nullptr;
}

bool SnarlManager::is_trivial(const Snarl* snarl, const HandleGraph& graph) const {
    // If it's an ultrabubble with no children and no contained nodes, it is a trivial snarl.
    return snarl->type() == ULTRABUBBLE &&
        is_leaf(snarl)
        && shallow_contents(snarl, graph, false).first.size() == 0;
}

bool SnarlManager::all_children_trivial(const Snarl* snarl, const HandleGraph& graph) const {
    for (auto& child : children_of(snarl)) {
        if (!is_trivial(child, graph)) {
            return false;
        }
    }
    return true;
}
    
const vector<const Snarl*>& SnarlManager::top_level_snarls() const {
    return roots;
}
    
void SnarlManager::for_each_top_level_snarl_parallel(const function<void(const Snarl*)>& lambda) const {
    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 0; i < roots.size(); i++) {
                #pragma omp task firstprivate(i)
                {
                    lambda(roots[i]);
                }
            }
        }
    }
}
    
void SnarlManager::for_each_top_level_snarl(const function<void(const Snarl*)>& lambda) const {
    for (const Snarl* snarl : roots) {
        lambda(snarl);
    }
}
    
void SnarlManager::for_each_snarl_preorder(const function<void(const Snarl*)>& lambda) const {
    // We define a recursive function to apply the lambda in a preorder traversal of the snarl tree.
    std::function<void(const Snarl*)> process = [&](const Snarl* parent) {
        // Do the parent
        lambda(parent);
        for (auto child : children_of(parent)) {
            // Then do each child
            process(child);
        }
    };
        
    // Then we run that on the root of each tree of snarls.
    for_each_top_level_snarl(process);
}
    
void SnarlManager::for_each_snarl_parallel(const function<void(const Snarl*)>& lambda) const {
    // We define a recursive function to apply the lambda in a preorder traversal of the snarl tree.
    std::function<void(const Snarl*)> process = [&](const Snarl* parent) {
        // Do the parent
        lambda(parent);
            
        auto& children = children_of(parent);
            
#pragma omp parallel for
        for (size_t i = 0; i < children.size(); i++) {
            // Then do each child in parallel
            process(children[i]);
        }
    };
        
    for_each_top_level_snarl_parallel(process);
}

void SnarlManager::for_each_top_level_chain(const function<void(const Chain*)>& lambda) const {
    for (const Chain& chain : root_chains) {
        lambda(&chain);
    }    
}

void SnarlManager::for_each_top_level_chain_parallel(const function<void(const Chain*)>& lambda) const {
#pragma omp parallel for schedule(dynamic, 1)
    for (size_t i = 0; i < root_chains.size(); ++i) {
        lambda(&root_chains[i]);
    }
}

void SnarlManager::for_each_chain(const function<void(const Chain*)>& lambda) const {

    // We define a function to run a bunch of chains in serial
    auto do_chain_list = [&](const deque<Chain>& chains) {
        for (size_t i = 0; i < chains.size(); i++) {
            lambda(&chains[i]);
        }
    };
    
    // Do our top-level chains
    do_chain_list(root_chains);
    
    for_each_snarl_preorder([&](const Snarl* snarl) {
        // Then in preorder order through all the snarls, do the child chains.
        do_chain_list(chains_of(snarl));
    });
}

void SnarlManager::for_each_chain_parallel(const function<void(const Chain*)>& lambda) const {

    // We define a function to run a bunch of chains in parallel
    auto do_chain_list = [&](const deque<Chain>& chains) {
#pragma omp parallel for
        for (size_t i = 0; i < chains.size(); i++) {
            lambda(&chains[i]);
        }
    };
    
    // Do our top-level chains in parallel.
    do_chain_list(root_chains);
    
    for_each_snarl_parallel([&](const Snarl* snarl) {
        // Then in parallel through all the snarls, do the child chains in parallel.
        do_chain_list(chains_of(snarl));
    });
}

void SnarlManager::for_each_snarl_unindexed(const function<void(const Snarl*)>& lambda) const {
    for (const SnarlRecord& snarl_record : snarls) {
        lambda(unrecord(&snarl_record));
    }
}

const Snarl* SnarlManager::discrete_uniform_sample(minstd_rand0& random_engine)const{
    // have to set the seed to the random engine in the unit tests , pass the random engine 

    int number_of_snarls = num_snarls();
#ifdef debug
    cerr << "number_of_snarls "<< number_of_snarls <<endl;
    for (int i =0; i< snarls.size(); i++){
        const Snarl* snarl  = unrecord(&snarls[i]);
        cerr << snarl->start().node_id() << " -> "<<snarl->end().node_id() <<endl;
    }
#endif

    // if we have no snarls we return a flag 
    if(number_of_snarls ==0){
        return nullptr;
    }
    
    // we choose a snarl from the master list of snarls in the graph at random uniformly
    // unif[a,b],  deque starts at index 0 so upperbound is size-1
    uniform_int_distribution<int> distribution(0, number_of_snarls-1);  
    int random_num = distribution(random_engine);
#ifdef debug
    cerr << "modifying snarl num " << random_num << endl;  
    if(unrecord(&snarls[random_num]) == nullptr){
        cerr << "unrecorded snarl is null" <<endl;
    }else{
       const Snarl* snarl  = unrecord(&snarls[random_num]);
       cerr << snarl->start() << endl;
       cerr << snarl->end() <<endl;
    }
#endif

    return unrecord(&snarls[random_num]);

} 

int SnarlManager::num_snarls()const{
    // get size of snarls in the master list deque<SnarlRecord> snarls 
    int num_snarls = this->snarls.size();
    return num_snarls;

}

    
void SnarlManager::flip(const Snarl* snarl) {
        
    // Get a non-const pointer to the SnarlRecord, which we own.
    // Allowed because we ourselves aren't const.
    SnarlRecord* to_flip = (SnarlRecord*) record(snarl);
    // Get the Snarl of it
    Snarl* to_flip_snarl = unrecord(to_flip);
    // swap and reverse the start and end Visits
    int64_t start_id = to_flip_snarl->start().node_id();
    bool start_orientation = to_flip_snarl->start().backward();
        
    to_flip_snarl->mutable_start()->set_node_id(to_flip_snarl->end().node_id());
    to_flip_snarl->mutable_start()->set_backward(!to_flip_snarl->end().backward());
        
    to_flip_snarl->mutable_end()->set_node_id(start_id);
    to_flip_snarl->mutable_end()->set_backward(!start_orientation);
    
    if (to_flip->parent_chain != nullptr) {
        // Work out where we keep the orientation of this snarl in its parent chain
        bool& to_invert = (*to_flip->parent_chain)[to_flip->parent_chain_index].second;
        // And flip it
        to_invert = !to_invert;
    }
        
    // Note: snarl_into index is invariant to flipping.
    // All the other indexes live in the SnarlRecords and don't need to change.
}

void SnarlManager::flip(const Chain* chain) {

    if (chain->empty()) {
        // Empty chains are already flipped
        return;
    }

    // Get ahold of a non-const version of the chain, without casting.
    Chain* mutable_chain = record(chain_begin(*chain)->first)->parent_chain;

    // Bust open the chain abstraction and flip it.
    // First reverse the order
    std::reverse(mutable_chain->begin(), mutable_chain->end());
    for (auto& chain_entry : *mutable_chain) {
        // Flip all the orientation flags so now we know the snarls are the other way relative to their chain.
        chain_entry.second = !chain_entry.second;
        
        // Get a mutable snarl record
        SnarlRecord* mutable_snarl = (SnarlRecord*) record(chain_entry.first);
        
        // Flip around its index in its chain so it can find its record again.
        mutable_snarl->parent_chain_index = chain->size() - mutable_snarl->parent_chain_index - 1;
    }

}
    
const Snarl* SnarlManager::add_snarl(const Snarl& new_snarl) {

    // Allocate a default SnarlRecord
    snarls.emplace_back();
    
    SnarlRecord* new_record = &snarls.back();
    
    // Hackily copy the snarl in
    *new_record = new_snarl;

    // Initialized snarl number for each record as deque is being filled
    new_record->snarl_number = (size_t)snarls.size()-1;
    
    // TODO: Should this be a non-default SnarlRecord constructor?

#ifdef debug
    cerr << "Adding snarl " << new_snarl.start().node_id() << " " << new_snarl.start().backward() << " -> "
         << new_snarl.end().node_id() << " " << new_snarl.end().backward() << endl;
#endif
        
    // We will set the parent and children and snarl_into and chain info when we finish().

    return unrecord(new_record);
}

void SnarlManager::finish() {
    // Build all the indexes from the snarls we were given
    build_indexes();
    
    // Clean up the snarl and chain orientations so everything is predictably and intuitively oriented
    regularize();

}

const Snarl* SnarlManager::into_which_snarl(int64_t id, bool reverse) const {
    return snarl_into.count(make_pair(id, reverse)) ? snarl_into.at(make_pair(id, reverse)) : nullptr;
}
    
const Snarl* SnarlManager::into_which_snarl(const Visit& visit) const {
    return visit.has_snarl() ? manage(visit.snarl()) : into_which_snarl(visit.node_id(), visit.backward());
}
    
unordered_map<pair<int64_t, bool>, const Snarl*> SnarlManager::snarl_boundary_index() const {
    unordered_map<pair<int64_t, bool>, const Snarl*> index;
    for (const SnarlRecord& snarl_record : snarls) {
        const Snarl& snarl = *unrecord(&snarl_record);
        index[make_pair(snarl.start().node_id(), snarl.start().backward())] = &snarl;
        index[make_pair(snarl.end().node_id(), !snarl.end().backward())] = &snarl;
    }
    return index;
}
    
unordered_map<pair<int64_t, bool>, const Snarl*> SnarlManager::snarl_end_index() const {
    unordered_map<pair<int64_t, bool>, const Snarl*> index;
    for (const SnarlRecord& snarl_record : snarls) {
        const Snarl& snarl = *unrecord(&snarl_record);
        index[make_pair(snarl.end().node_id(), !snarl.end().backward())] = &snarl;
    }
    return index;
}
    
unordered_map<pair<int64_t, bool>, const Snarl*> SnarlManager::snarl_start_index() const {
    unordered_map<pair<int64_t, bool>, const Snarl*> index;
    for (const SnarlRecord& snarl_record : snarls) {
        const Snarl& snarl = *unrecord(&snarl_record);
        index[make_pair(snarl.start().node_id(), snarl.start().backward())] = &snarl;
    }
    return index;
}
    
void SnarlManager::build_indexes() {
#ifdef debug
    cerr << "Building SnarlManager index of " << snarls.size() << " snarls" << endl;
#endif

    // Reserve space for the snarl_into index, so we hopefully don't need to rehash or move anything.
    snarl_into.reserve(snarls.size() * 2);

    for (SnarlRecord& rec : snarls) {
        Snarl& snarl = *unrecord(&rec);
    
        // Build the snarl_into index first so we can manage() to resolve populated-snarl cross-references to parents later.
        snarl_into[make_pair(snarl.start().node_id(), snarl.start().backward())] = &snarl;
        snarl_into[make_pair(snarl.end().node_id(), !snarl.end().backward())] = &snarl;
#ifdef debug
        cerr << snarl.start().node_id() << " " << snarl.start().backward() << " reads into " << pb2json(snarl) << endl;
        cerr << snarl.end().node_id() << " " << !snarl.end().backward() << " reads into " << pb2json(snarl) << endl;
#endif
    }
    
        
    for (SnarlRecord& rec : snarls) {
        Snarl& snarl = *unrecord(&rec);
            
#ifdef debug
        cerr << pb2json(snarl) << endl;
#endif
            
        // is this a top-level snarl?
        if (snarl.has_parent()) {
            // add this snarl to the parent-to-children index
#ifdef debug
            cerr << "\tSnarl is a child" << endl;
#endif
            
            // Record it as a child of its parent
            SnarlRecord* parent = (SnarlRecord*) record(manage(snarl.parent()));
            parent->children.push_back(&snarl);
            
            // And that its parent is its parent
            rec.parent = unrecord(parent);
        }
        else {
            // record top level status
#ifdef debug
            cerr << "\tSnarl is top-level" << endl;
#endif
            roots.push_back(&snarl);
            
            rec.parent = nullptr;
        }
    }
        
    // Compute the chains using the into and out-of indexes.
    
    // Compute the chains for the root level snarls
    root_chains = compute_chains(roots);
        
    // Build the back index from root snarl to containing chain
    for (Chain& chain : root_chains) {
        for (size_t i = 0; i < chain.size(); i++) {
            auto& oriented_snarl = chain[i];
            
            // Get the mutable record for each child snarl in the chain
            SnarlRecord* child_record = (SnarlRecord*) record(oriented_snarl.first);
            
            // Give it a pointer to the chain.
            child_record->parent_chain = &chain;
            // And where it is in it
            child_record->parent_chain_index = i;
        }
    }
    
    for (SnarlRecord& rec : snarls) {
        if (rec.children.empty()) {
            // Only look at snarls with children.
            continue;
        }
        
        // Compute the chains among the children
        rec.child_chains = compute_chains(rec.children);
        
        // Build the back index from child snarl to containing chain
        for (Chain& chain : rec.child_chains) {
            for (size_t i = 0; i < chain.size(); i++) {
                auto& oriented_snarl = chain[i];
                
                // Get the mutable record for each child snarl in the chain
                SnarlRecord* child_record = (SnarlRecord*) record(oriented_snarl.first);
                
                // Give it a pointer to the chain.
                child_record->parent_chain = &chain;
                // And where it is in it
                child_record->parent_chain_index = i;
            }
        }
    }
}

deque<Chain> SnarlManager::compute_chains(const vector<const Snarl*>& input_snarls) {
    // We populate this
    deque<Chain> to_return;
        
    // We track the snarls we have seen in chain traversals so we only have to see each chain once.
    unordered_set<const Snarl*> seen;
        
    for (const Snarl* snarl : input_snarls) {
        // For every snarl in this snarl (or, if snarl is null, every top level snarl)
            
        if (seen.count(snarl)) {
            // Already in a chain
            continue;
        }
            
        // Make a new chain for this child, with it in the forward direction in the chain.
        list<pair<const Snarl*, bool>> chain{{snarl, false}};
            
        // Mark it as seen
        seen.insert(snarl);
            
        // Make a visit to the child in forward orientation
        Visit here;
        transfer_boundary_info(*snarl, *here.mutable_snarl());
        // The default is already not-backward, but we set it anyway
        here.set_backward(false);
        
        for (Visit walk_left = prev_snarl(here);
             walk_left.has_snarl() && !seen.count(manage(walk_left.snarl()));
             walk_left = prev_snarl(walk_left)) {
            
            // For everything in the chain left from here, until we hit the
            // end or come back to the start
             
            // Add it to the chain in the orientation we find it
            chain.emplace_front(manage(walk_left.snarl()), walk_left.backward());
            // Mark it as seen
            seen.insert(chain.front().first);
        }
            
        for (Visit walk_right = next_snarl(here);
             walk_right.has_snarl() && !seen.count(manage(walk_right.snarl()));
             walk_right = next_snarl(walk_right)) {
                
            // For everything in the chain right from here, until we hit the
            // end or come back to the start
            
            // Add it to the chain in the orientation we find it
            chain.emplace_back(manage(walk_right.snarl()), walk_right.backward());
            // Mark it as seen
            seen.insert(chain.back().first);
        }
            
        // Copy from the list into a vector
        to_return.emplace_back(chain.begin(), chain.end());
    }
        
    return to_return;
}

void SnarlManager::regularize() {
    // Algorithm:
    // For each chain
    // Flip any snarls that are backward in the chain
    // If now the majority of the snarls end lower than they start, flip all the snarls and invert the chain.
    
#ifdef debug
    cerr << "Regularizing snarls and chains" << endl;
#endif
    
    for_each_chain_parallel([&](const Chain* chain) {
        // For every chain
        
        // Make a list of snarls to flip
        vector<const Snarl*> backward;
        // And a list of snarls to not flip
        vector<const Snarl*> forward;
        
        // Count the snarls that go low to high, as they should
        size_t correctly_oriented = 0;
        
        auto chain_start = chain_begin(*chain);
        auto chain_stop = chain_end(*chain);
        for (auto it = chain_start; it != chain_stop; ++it) {
            // For each snarl in the chain
            if (it->second) {
                // If it is backward, remember to flip it
                backward.push_back(it->first);
                
#ifdef debug
                cerr << "Snarl " << it->first->start() << " -> " << it->first->end() << " is backward in chain " << chain << endl;
#endif
                
                if (it->first->end().node_id() <= it->first->start().node_id()) {
                    // Count it as correctly oriented if it will be
#ifdef debug
                    cerr << "\tWill be graph-ascending when brought in line with chain" << endl;
#endif
                    correctly_oriented++;
                }
            } else {
                // If it is forward, remember that
                forward.push_back(it->first);
#ifdef debug
                cerr << "Snarl " << it->first->start() << " -> " << it->first->end() << " is forward in chain " << chain << endl;
#endif
                
                if (it->first->start().node_id() <= it->first->end().node_id()) {
                    // Count it as correctly oriented if it is
#ifdef debug
                    cerr << "\tIs graph-ascending already" << endl;
#endif
                    correctly_oriented++;
                }
            }
        }
        
#ifdef debug
        cerr << "Found " << correctly_oriented << "/" << chain->size() << " snarls of chain in graph-ascending orientation" << endl;
#endif
        
        if (correctly_oriented * 2 < chain->size()) {
            // Fewer than half the snarls are pointed the right way when they
            // go with the chain. (Don't divide chain size because then a chain
            // size of 1 requires 0 correctly oriented sanrls.)
            
#ifdef debug
            cerr << "Chain is in the worse orientation overall. Flipping!" << endl;
#endif
            
            // Really we want to invert the entire chain around the snarls, and
            // then only flip the formerly-chain-forward snarls.
            flip(chain);
            
            // Now set up to flip the other set of snarls
            backward.swap(forward);
            
        }
        
        for (auto& to_flip : backward) {
            // Flip all the snarls we found to flip to agree with the chain,
            // while not looping over the chain.
            flip(to_flip);
            
#ifdef debug
            cerr << "Flipped snarl to produce " << to_flip->start() << " " << to_flip->end() << endl;
#endif
        }
    });
    
}
    
pair<unordered_set<nid_t>, unordered_set<edge_t> > SnarlManager::shallow_contents(const Snarl* snarl, const HandleGraph& graph,
                                                                                 bool include_boundary_nodes) const {
    
    pair<unordered_set<nid_t>, unordered_set<edge_t> > to_return;
        
    unordered_set<nid_t> already_stacked;
        
    // initialize stack for DFS traversal of site
    vector<handle_t> stack;
        
    handle_t start_node = graph.get_handle(snarl->start().node_id());
    handle_t end_node = graph.get_handle(snarl->end().node_id());
        
    // mark the boundary nodes as already stacked so that paths will terminate on them
    already_stacked.insert(graph.get_id(start_node));
    already_stacked.insert(graph.get_id(end_node));
        
    // add boundary nodes as directed
    if (include_boundary_nodes) {
        to_return.first.insert(graph.get_id(start_node));
        to_return.first.insert(graph.get_id(end_node));
    }

    // stack up the nodes one edge inside the snarl from the start
    graph.follow_edges(start_node, snarl->start().backward(), [&](const handle_t& node) {            

            if (!already_stacked.count(graph.get_id(node))) {
                stack.push_back(node);
                already_stacked.insert(graph.get_id(node));
            }
            if (snarl->start().backward()) {
                to_return.second.insert(graph.edge_handle(node, start_node));
            } else {
                to_return.second.insert(graph.edge_handle(start_node, node));
            }
        });
      
    // stack up the nodes one edge inside the snarl from the end
    graph.follow_edges(end_node, !snarl->end().backward(), [&](const handle_t& node) {
            
            if (!already_stacked.count(graph.get_id(node))) {
                stack.push_back(node);
                already_stacked.insert(graph.get_id(node));
            }
            if (snarl->end().backward()) {
                to_return.second.insert(graph.edge_handle(end_node, node));
            } else {
                to_return.second.insert(graph.edge_handle(node, end_node));
            }
        });
        
    // traverse the snarl with DFS, skipping over any child snarls
    // do not pay attention to valid walks since we also want to discover any tips
    while (stack.size()) {
            
        // pop the top node off the stack
        handle_t node = stack.back();
        stack.pop_back();
            
        // record that this node is in the snarl
        to_return.first.insert(graph.get_id(node));
            
        const Snarl* forward_snarl = into_which_snarl(graph.get_id(node), false);
        const Snarl* backward_snarl = into_which_snarl(graph.get_id(node), true);
        if (forward_snarl) {
            // this node points into a snarl
                
            // What's on the other side of the snarl?
            nid_t other_id = forward_snarl->start().node_id() == graph.get_id(node) ? forward_snarl->end().node_id() : forward_snarl->start().node_id();
                
            // stack up the node on the opposite side of the snarl
            // rather than traversing it
            handle_t opposite_node = graph.get_handle(other_id);
            if (!already_stacked.count(other_id)) {
                stack.push_back(opposite_node);
                already_stacked.insert(other_id);
            }
        }
            
        if (backward_snarl) {
            // the reverse of this node points into a snarl
                
            // What's on the other side of the snarl?
            nid_t other_id = backward_snarl->end().node_id() == graph.get_id(node) ? backward_snarl->start().node_id(): backward_snarl->end().node_id();
                
            // stack up the node on the opposite side of the snarl
            // rather than traversing it
            handle_t opposite_node = graph.get_handle(other_id);
            if (!already_stacked.count(other_id)) {
                stack.push_back(opposite_node);
                already_stacked.insert(other_id);
            }
        }
            
        graph.follow_edges(node, false, [&](const handle_t& next_node) {
                edge_t edge = graph.edge_handle(node, next_node);
                // does this edge point forward or backward?
                if ((graph.get_is_reverse(node) && !backward_snarl) ||
                    (!graph.get_is_reverse(node) && !forward_snarl)) {

                        to_return.second.insert(edge);
                        
                        if (!already_stacked.count(graph.get_id(next_node))) {
                            
                            stack.push_back(next_node);
                            already_stacked.insert(graph.get_id(next_node));
                        }
                }
            });
        
        graph.follow_edges(node, true, [&](const handle_t& prev_node) {
                edge_t edge = graph.edge_handle(prev_node, node);
                // does this edge point forward or backward?
                if ((graph.get_is_reverse(node) && !forward_snarl) ||
                    (!graph.get_is_reverse(node) && !backward_snarl)) {

                    to_return.second.insert(edge);
                        
                    if (!already_stacked.count(graph.get_id(prev_node))) {
                            
                        stack.push_back(prev_node);
                        already_stacked.insert(graph.get_id(prev_node));
                    }
                }
            });
    }
        
    return to_return;
}
    
pair<unordered_set<nid_t>, unordered_set<edge_t> > SnarlManager::deep_contents(const Snarl* snarl, const HandleGraph& graph,
                                                                              bool include_boundary_nodes) const {
        
    pair<unordered_set<nid_t>, unordered_set<edge_t> > to_return;
        
    unordered_set<nid_t> already_stacked;
        
    // initialize stack for DFS traversal of site
    vector<handle_t> stack;

    handle_t start_node = graph.get_handle(snarl->start().node_id());
    handle_t end_node = graph.get_handle(snarl->end().node_id());
        
    // mark the boundary nodes as already stacked so that paths will terminate on them
    already_stacked.insert(graph.get_id(start_node));
    already_stacked.insert(graph.get_id(end_node));
        
    // add boundary nodes as directed
    if (include_boundary_nodes) {
        to_return.first.insert(graph.get_id(start_node));
        to_return.first.insert(graph.get_id(end_node));
    }

    // stack up the nodes one edge inside the snarl from the start
    graph.follow_edges(start_node, snarl->start().backward(), [&](const handle_t& node) {            

            if (!already_stacked.count(graph.get_id(node))) {
                stack.push_back(node);
                already_stacked.insert(graph.get_id(node));
            }
            if (snarl->start().backward()) {
                to_return.second.insert(graph.edge_handle(node, start_node));
            } else {
                to_return.second.insert(graph.edge_handle(start_node, node));
            }
        });
      
    // stack up the nodes one edge inside the snarl from the end
    graph.follow_edges(end_node, !snarl->end().backward(), [&](const handle_t& node) {
            
            if (!already_stacked.count(graph.get_id(node))) {
                stack.push_back(node);
                already_stacked.insert(graph.get_id(node));
            }
            if (snarl->end().backward()) {
                to_return.second.insert(graph.edge_handle(end_node, node));
            } else {
                to_return.second.insert(graph.edge_handle(node, end_node));
            }
        });
        
    // traverse the snarl with DFS, skipping over any child snarls
    // do not pay attention to valid walks since we also want to discover any tips
    while (stack.size()) {
            
        // pop the top node off the stack
        handle_t node = stack.back();
        stack.pop_back();
            
        // record that this node is in the snarl
        to_return.first.insert(graph.get_id(node));

        graph.follow_edges(node, false, [&] (const handle_t& next_node) {
            edge_t edge = graph.edge_handle(node, next_node);
            to_return.second.insert(edge);
            if (!already_stacked.count(graph.get_id(next_node))) {
                stack.push_back(next_node);
                already_stacked.insert(graph.get_id(next_node));
            }
            });
        
        graph.follow_edges(node, true, [&] (const handle_t& prev_node) {
            edge_t edge = graph.edge_handle(prev_node, node);
            to_return.second.insert(edge);
            if (!already_stacked.count(graph.get_id(prev_node))) {
                stack.push_back(prev_node);
                already_stacked.insert(graph.get_id(prev_node));
            }
            });
    }
        
    return to_return;
}
    
const Snarl* SnarlManager::manage(const Snarl& not_owned) const {
    // TODO: keep the Snarls in some kind of sorted order to make lookup
    // efficient. We could also have a map<Snarl, Snarl*> but that would be
    // a tremendous waste of space.
    
    // Work out how to read into the snarl
    pair<int64_t, bool> reading_in = make_pair(not_owned.start().node_id(), not_owned.start().backward());
    
    // Get the cannonical pointer to the snarl that we are reading into with the start, inward visit.
    auto it = snarl_into.find(reading_in);
        
    if (it == snarl_into.end()) {
        // It's not there. Someone is trying to manage a snarl we don't
        // really own. Complain.
        throw runtime_error("Unable to find snarl " +  pb2json(not_owned) + " in SnarlManager");
    }
        
    // Return the official copy of that snarl
    return it->second;
}
    
vector<Visit> SnarlManager::visits_right(const Visit& visit, const HandleGraph& graph, const Snarl* in_snarl) const {
        
#ifdef debug
    cerr << "Look right from " << visit << endl;
#endif
        
    // We'll populate this
    vector<Visit> to_return;
        
    // Find the right side of the visit we're on
    NodeSide right_side = to_right_side(visit);
        
    if (visit.node_id() == 0) {
        // We're leaving a child snarl, so we are going to need to check if
        // another child snarl shares this boundary node in the direction
        // we're going.
            
        const Snarl* child = into_which_snarl(right_side.node, !right_side.is_end);
        if (child != nullptr && child != in_snarl
            && into_which_snarl(right_side.node, right_side.is_end) != in_snarl) {
            // We leave the one child and immediately enter another!
                
            // Make a visit to it
            Visit child_visit;
            transfer_boundary_info(*child, *child_visit.mutable_snarl());
                
            if (right_side.node == child->end().node_id()) {
                // We came in its end
                child_visit.set_backward(true);
            } else {
                // We should have come in its start
                assert(right_side.node == child->start().node_id());
            }
                
            // Bail right now, so we don't try to explore inside this child snarl.
            to_return.push_back(child_visit);
            return to_return;
                
        }
            
    }

    graph.follow_edges(graph.get_handle(right_side.node), !right_side.is_end, [&](const handle_t& next_handle) {
        // For every NodeSide attached to the right side of this visit
        NodeSide attached(graph.get_id(next_handle), right_side.is_end ? graph.get_is_reverse(next_handle) : !graph.get_is_reverse(next_handle));
            
#ifdef debug
        cerr << "\tFind NodeSide " << attached << endl;
#endif
            
        const Snarl* child = into_which_snarl(attached.node, attached.is_end);
        if (child != nullptr && child != in_snarl
            && into_which_snarl(attached.node, !attached.is_end) != in_snarl) {
            // We're reading into a child
                
#ifdef debug
            cerr << "\t\tGoes to Snarl " << *child << endl;
#endif
                
            if (attached.node == child->start().node_id()) {
                // We're reading into the start of the child
                    
                // Make a visit to the child snarl
                Visit child_visit;
                transfer_boundary_info(*child, *child_visit.mutable_snarl());
                    
#ifdef debug
                cerr << "\t\tProduces Visit " << child_visit << endl;
#endif
                    
                // Put it in in the forward orientation
                to_return.push_back(child_visit);
            } else if (attached.node == child->end().node_id()) {
                // We're reading into the end of the child
                    
                // Make a visit to the child snarl
                Visit child_visit;
                transfer_boundary_info(*child, *child_visit.mutable_snarl());
                child_visit.set_backward(true);
                    
#ifdef debug
                cerr << "\t\tProduces Visit " << child_visit << endl;
#endif
                    
                // Put it in in the reverse orientation
                to_return.push_back(child_visit);
            } else {
                // Should never happen
                throw runtime_error("Read into child " + pb2json(*child) + " with non-matching traversal");
            }
        } else {
            // We just go into a normal node
            to_return.emplace_back();
            Visit& next_visit = to_return.back();
            next_visit.set_node_id(attached.node);
            next_visit.set_backward(attached.is_end);
            
#ifdef debug
            cerr << "\t\tProduces Visit " << to_return.back() << endl;
#endif
                
        }
    });
        
    return to_return;
        
}
    
vector<Visit> SnarlManager::visits_left(const Visit& visit, const HandleGraph& graph, const Snarl* in_snarl) const {
        
    // Get everything right of the reversed visit
    vector<Visit> to_return = visits_right(reverse(visit), graph, in_snarl);
        
    // Un-reverse them so they are in the correct orientation to be seen
    // left of here.
    for (auto& v : to_return) {
        v = reverse(v);
    }
        
    return to_return;
        
}


}

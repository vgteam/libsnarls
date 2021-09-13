#ifndef LIBSNARLS_HANDLE_GRAPH_SNARL_FINDER_HPP_INCLUDED
#define LIBSNARLS_HANDLE_GRAPH_SNARL_FINDER_HPP_INCLUDED

#include <handlegraph/handle_graph.hpp>

#include <functional>

namespace snarls {

using namespace std;
using namespace vg;
using namespace handlegraph;

class SnarlManager;

/**
 * Wrapper base class that can convert a bottom-up traversal of snarl
 * boundaries into a full snarl finder. Mostly worries about snarl
 * classification and connectivity information.
 */
class HandleGraphSnarlFinder : public SnarlFinder {
protected:
    /**
     * The graph we are finding snarls on. It must outlive us.
     */
    const HandleGraph* graph;
    
    /**
     * Find all the snarls, and put them into a SnarlManager, but don't finish it.
     * More snarls can be added later before it is finished.
     */
    virtual SnarlManager find_snarls_unindexed();
    
public:

    /**
     * Create a HandleGraphSnarlFinder to find snarls in the given graph.
     */
    HandleGraphSnarlFinder(const HandleGraph* graph);

    virtual ~HandleGraphSnarlFinder() = default;

    /**
     * Find all the snarls, and put them into a SnarlManager.
     */
    virtual SnarlManager find_snarls();
    
    /**
     * Visit all snarls and chains, including trivial snarls and single-node
     * empty chains.
     *
     * Calls begin_chain and end_chain when entrering and exiting chains in the
     * traversal. Within each chain, calls begin_snarl and end_snarl when
     * entering and exiting each snarl, in order. The caller is intended to
     * maintain its own stack to match up begin and end events.
     *
     * Each begin/end call receives the handle reading into/out of the snarl or
     * chain. 
     *
     * Both empty and cyclic chains have the in and out handles the same.
     * They are distinguished by context; empty chains have no shild snarls,
     * while cyclic chains do.
     *
     * Roots the decomposition at a global snarl with no bounding nodes, for
     * which begin_snarl is not called. So the first call will be begin_chain.
     *
     * Start handles are inward facing and end handles are outward facing.
     * Snarls must be oriented forward in their chains.
     */
    virtual void traverse_decomposition(const function<void(handle_t)>& begin_chain, const function<void(handle_t)>& end_chain,
        const function<void(handle_t)>& begin_snarl, const function<void(handle_t)>& end_snarl) const = 0;
};


}

#endif

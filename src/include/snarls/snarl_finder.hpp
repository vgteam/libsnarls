#ifndef LIBSNARLS_SNARL_FINDER_HPP_INCLUDED
#define LIBSNARLS_SNARL_FINDER_HPP_INCLUDED

namespace snarls {

using namespace std;

class SnarlManager;

/**
 * Represents a strategy for finding (nested) sites in a vg graph that can be described
 * by snarls. Polymorphic base class/interface.
 */
class SnarlFinder {
public:
    virtual ~SnarlFinder() = default;

    /**
     * Find all the snarls, and put them into a SnarlManager.
     */
    virtual SnarlManager find_snarls() = 0;

    /**
     * Find all the snarls of weakly connected components, optionally in
     * parallel. If not implemented, defaults to the single-threaded
     * implementation.
     */
    virtual SnarlManager find_snarls_parallel();
};

}

#endif

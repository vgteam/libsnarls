#include "snarls/pair_hash.hpp"
#include "snarls/snarl_finder.hpp"
#include "snarls/snarl_manager.hpp"

namespace snarls {

SnarlManager SnarlFinder::find_snarls_parallel() {
    // By default, just use a single thread, unless this finder has a parallel
    // overriding implementation.
    return find_snarls();
}

}

#ifndef LIBSNARLS_VG_TYPES_HPP_INCLUDED
#define LIBSNARLS_VG_TYPES_HPP_INCLUDED

// We need the vg Protobuf types like vg::Snarl and vg::Visit.
// But for some reason the Protobuf generated headers fail to compile unless an std::hash implementation for std::pair is available.
// So we make sure we always have one when we use them by including it first.

#include "snarls/pair_hash.hpp"

#include <vg/vg.pb.h>

// This header is the Right Way to get the vg Protobuf types defined in this library.

#endif

# `libsnarls`: Finding variation in graphs

[Pangenome graphs](https://pangenome.github.io/) are an increasingly popular representation for multiple individuals' genomes, and describe the ways in which they are the same and the ways in which they differ. However, as these graphs contain full-length descriptions of all the stored genomes, it is challenging to describe and compute on the differences between genomes in an idiomatic way. Linear genome users are able to go through [VCF files](https://samtools.github.io/hts-specs/VCFv4.2.pdf) listing "variants" with "genotypes" for each individual. Graph genome users have had few tools to deal with the explosive variety of possible paths through the graph that a genome could follow. 

This library provides tools for identifying and operating on "snarls", a natural mathematical representation of differences between genomes, or variable sites, in a pangenome graph. Snarls are defined in [the paper "Superbubbles, Ultrabubbles and Cacti"](https://link.springer.com/chapter/10.1007%2F978-3-319-56970-3_11). Basically, a snarl is a part of the graph that is completely contained between two ends of sequence nodes, and is roughly the equivalent of a "variant" in VCF. While VCF variants are difficult to interpret when they overlap, snarls are naturally recursive. While VCF variants are arranged along a linear chromosomal coordinate system, snarls form "chains" which samples can visit in sequence, while acocunting for the possibility of turning around.

## Installation

### As a component in a CMake project

To use the library in a larger CMake project, we recommend adding the Git repository as a submodule at, for example, `deps/libsnarls`):

```
mkdir deps
cd deps
git submodule add https://github.com/vgteam/libsnarls.git
```

Then you can use the CMake project as a subdirectory:

```
add_subdirectory("deps/libvgio")
```

This will produce a shared library target `snarls_shared` and a static library target `snarls_static`. You can then tell your CMake targets to link against either one:

```
target_link_libraries(my_dynamic_binary PUBLIC snarls_shared)
```

### As a System Library

After cloning the repository, perform a CMake out-of-source build:

```
mkdir build
cd build
cmake ..
make
```

After that, you can `sudo make install` to install the library and dependencies on your system.

### As a User Library

Determine a prefix to install to (such as `$HOME/.local`). Then, perform a CMake out-of-source build using that prefix:

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
make
```

After that, you can `make install` to install the library and dependencies into your prefix.

## Usage

First, you will want to get your graph available as a [`libhandlegraph`](https://github.com/vgteam/libhandlegraph) `HandleGraph`; `libhandlegraph` is bundled as a dependency. We recommend using `bdsg::PackedGraph`:

```
#include <bdsg/packed_graph.hpp>
```

Then, you will need to use a `snarls::SnarlFinder` to identify snarl structures in the graph. Currently there is one implementation, `snarls::IntegratedSnarlFinder`:

```
#include <snarls/integrated_snarl_finder.hpp>
```

Then, use the snarl finder to get a `snarls::SnarlManager` describing the snarls in the graph:

```
snarls::IntegratedSnarlFinder finder(graph);
snarls::SnarlManager manager = finder.find_snarls_parallel();
```

The `snarls::SnarlManager` offers useful methods, such as:
 * `into_which_snarl()`, which lets you know if a node reads into a snarl in a certain direction
 * `children_of()`, which tells you the child snarls in a given snarl
 * `top_level_snarls()`, which gives you all of the snarls that are top-level sites, with no containing parent snarls
 
Currently the API is based around `const Snarl*` pointers, which point to [Protobuf Snarl objects defined by `libvgio`](https://github.com/vgteam/libvgio/blob/25e922ff8a556a233b0741617c2b88c2d41de8ed/deps/vg.proto#L254). 




/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef MATRIX_GRAPH_WRAPPER_H
#define MATRIX_GRAPH_WRAPPER_H

#include <vector>

//This Wrapper provides all methods that are needed for TarjanSCC, when the graph is given in a
//matrix representation (e.g. as output from a distance table call)

template <typename T> class MatrixGraphWrapper {
public:

    MatrixGraphWrapper(std::vector<T> table, const unsigned number_of_nodes) : table_(table), number_of_nodes_(number_of_nodes) {};

    unsigned GetNumberOfNodes() const {
        return number_of_nodes_;
    }

    std::vector<unsigned> GetAdjacentEdgeRange(const unsigned node) const {
        std::vector<unsigned> edges;
        const auto maxint = std::numeric_limits<int>::max();
        for (auto i = 0; i < number_of_nodes_; ++i) {
            if (*(table_.begin() + node * number_of_nodes_ + i) != maxint) {
                edges.push_back(i);
            }
        }
        return edges;
    }

    unsigned GetTarget(const unsigned edge) const {
        return edge;
    }

private:
    std::vector<T> table_;
    const unsigned number_of_nodes_;
};


#endif // MATRIX_GRAPH_WRAPPER_H

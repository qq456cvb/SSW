//
//  unionset.cpp
//  SSW
//
//  Created by Neil on 16/05/2018.
//  Copyright © 2018 Neil. All rights reserved.
//

#include "unionset.hpp"
#include <algorithm>

Union *find(Union *&ref) {
    while (ref->parent != ref) {
        ref->parent = ref->parent->parent;
        ref = ref->parent;
    }
    return ref;
}

bool sameParent(Union *x, Union *y) {
    return find(x) == find(y);
}

Union *merge(Union *x, Union *y) {
    auto x_root = find(x);
    auto y_root = find(y);
    
    if (x == y) {
        return x_root;
    }
    
    if (x_root->rank < y_root->rank) {
        std::swap(x_root, y_root);
    }
    
    y_root->parent = x_root;
    for (auto nbr : y_root->neighbours) {
        x_root->neighbours.insert(find(nbr));
    }
//    (y_root->neighbours.begin(), y_root->neighbours.end());
    for (int i = 0; i < 8; i++) {
        x_root->hist[i] = (x_root->hist[i] * x_root->size + y_root->hist[i] * y_root->size) / (x_root->size + y_root->size);
    }
    x_root->size += y_root->size;
    if (x_root->rank == y_root->rank) {
        x_root->rank++;
    }
    return x_root;
}

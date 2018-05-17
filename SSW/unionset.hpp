//
//  unionset.hpp
//  SSW
//
//  Created by Neil on 16/05/2018.
//  Copyright © 2018 Neil. All rights reserved.
//

#ifndef unionset_hpp
#define unionset_hpp

#include <stdio.h>
#include <stdint.h>
#include <unordered_set>
#include <vector>

class Edge;
class Union {
public:
    int inter = 0;
    Union *parent = this;
    int rank = 0;
    int size = 1;
    
    std::unordered_set<Union*> neighbours;
    float hist[8] = { 0 };
};

class Edge {
public:
    float sim;
    int weight = 0;
    Union *first, *second;
    Edge(Union *a, Union *b, int w) : first(a), second(b), weight(w) {
    };
    Edge(Union *a, Union *b, float s) : first(a), second(b), sim(s) {
    };
//    bool operator==(const Edge &e) {
//        return (this->first == e.first && this->second == e.second) ||
//        (this->second == e.first && this->first == e.second);
//    }
};


Union *find(Union *&ref);
bool sameParent(Union *x, Union *y);
Union *merge(Union *x, Union *y);

#endif /* unionset_hpp */

//
//  main.cpp
//  SSW
//
//  Created by Neil on 16/05/2018.
//  Copyright © 2018 Neil. All rights reserved.
//

#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include "unionset.hpp"
using namespace cv;
using namespace std;

class Mypair {
public:
    Union *first, *second;
    Mypair(Union *a, Union *b) : first(a), second(b) {};
    bool operator==(const Mypair &p) {
        return (first == p.first && second == p.second) || (first == p.second && second == p.first);
    }
};

struct PairHasher
{
    std::size_t operator()(const Mypair& p) const
    {
        using std::size_t;
        using std::hash;
        
        return (hash<void*>()(p.first) ^ hash<void*>()(p.second));
    }
};

struct EdgeHasher
{
    std::size_t operator()(const Edge& e) const
    {
        using std::size_t;
        using std::hash;
        
        return (hash<void*>()(e.first) ^ hash<void*>()(e.second));
    }
};

bool operator==(const Edge &e1, const Edge &e2) {
    return (e1.first == e2.first && e1.second == e2.second) ||
    (e1.second == e2.first && e1.first == e2.second);
}



void showUnions(std::vector<Union> &unions, int rows, int cols) {
    static vector<Vec3b> colors;
    if (colors.empty()) {
        for (int i = 0; i < unions.size(); i++) {
            colors.emplace_back(rand() % 255, rand() % 255, rand() % 255);
        }
    }
    unordered_map<Union*, Vec3b> set;
    for (int i = 0; i < unions.size(); i++) {
        Union *tmp = &unions[i];
        set[find(tmp)] = colors[i];
    }
    printf("number of components: %lu\n", set.size());
    Mat result(rows, cols, CV_8UC3);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Union *tmp = &unions[i * cols + j];
            result.at<Vec3b>(i, j) = set[find(tmp)];
        }
    }
    cv::imshow("result", result);
    cv::waitKey(0);
}

int main(int argc, const char * argv[]) {
    auto img = imread("lena.jpg");
    //    resize(img, img, Size(256, 256));
    //    resize(img, img, Size(320, 240));
    //    Mat bgr[3];
    //    split(img, bgr);
    //    img = bgr[0];
    cvtColor(img, img, CV_BGR2GRAY);
    
    float sigma = 0.8f;
    GaussianBlur(img, img, Size(0, 0), sigma);
    Mat dx, dy;
    spatialGradient(img, dx, dy);
    
    int k = 300;
    auto ptr = (uint8_t*)img.data;
    std::vector<Edge> edges;
    edges.reserve(img.rows * img.cols * 4);
    std::vector<Union> unions;
    unions.reserve(img.rows * img.cols);
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            int idx = i * img.cols + j;
            unions.emplace_back();
            if (i > 0) {
                edges.emplace_back(&unions[unions.size() - 1 - img.cols], &unions.back(), abs((int)ptr[idx] - (int)ptr[idx - img.cols]));
                unions.back().neighbours.insert(&unions[unions.size() - 1 - img.cols]);
                unions[unions.size() - 1 - img.cols].neighbours.insert(&unions.back());
            }
            if (j > 0) {
                edges.emplace_back(&unions[unions.size() - 2], &unions.back(), abs((int)ptr[idx] - (int)ptr[idx - 1]));
                unions.back().neighbours.insert(&unions[unions.size() - 2]);
                unions[unions.size() - 2].neighbours.insert(&unions.back());
            }
            if (i > 0 && j > 0) {
                edges.emplace_back(&unions[unions.size() - 2 - img.cols], &unions.back(), abs((int)ptr[idx] - (int)ptr[idx - 1 - img.cols]));
                unions.back().neighbours.insert(&unions[unions.size() - 2 - img.cols]);
                unions[unions.size() - 2 - img.cols].neighbours.insert(&unions.back());
            }
            if (j < img.cols - 1 && i > 0) {
                edges.emplace_back(&unions[unions.size() - img.cols], &unions.back(), abs((int)ptr[idx] - (int)ptr[idx + 1 - img.cols]));
                unions.back().neighbours.insert(&unions[unions.size() - img.cols]);
                unions[unions.size() - img.cols].neighbours.insert(&unions.back());
            }
            
            float angle = atan2(float(dy.at<uchar>(i, j)), float(dx.at<uchar>(i, j)));
            if (angle < 0) angle += M_PI / 2;
            int bin_idx = static_cast<int>(angle / M_PI * 4);
            unions.back().hist[bin_idx] = 1.f;
        }
    }
    sort(edges.begin(), edges.end(), [](const Edge &e1, const Edge &e2){
        return e1.weight < e2.weight;
    });
    for (int i = 0; i < edges.size(); i++)  {
        Union *a = find(edges[i].first);
        Union *b = find(edges[i].second);
        if (a != b) {
            if (edges[i].weight <= fmin(a->inter + k / (float)a->size, b->inter + k / (float)b->size)) {
                merge(a, b)->inter = edges[i].weight;
            }
        }
    }
    showUnions(unions, img.rows, img.cols);
    // hash map, runtime: NK, N is # of components, K is # of neighbours
    unordered_set<Union*> set;
    for (int i = 0; i < unions.size(); i++) {
        Union *tmp = &unions[i];
        set.insert(find(tmp));
    }
    
    unordered_set<Edge, EdgeHasher> sims;
    for (auto u : set) {
        for (auto nbr : u->neighbours) {
            Union *other = find(nbr);
            if (other == u) continue;
            float score_size =1. - (u->size + other->size) / double(img.cols * img.rows);
            float score_texture = 0.f;
            for (int i = 0; i < 8; i++) {
                score_texture += u->hist[i] * other->hist[i];
            }
            float sim = score_size + score_texture;
            
            sims.emplace(u, other, sim);
        }
    }
    
    for (auto sim : sims) {
        for (auto nbr : sim.first->neighbours) {
            if (sim.first == find(nbr)) continue;
            assert(sims.find(Edge(find(sim.first), find(nbr), 0.f)) != sims.end());
        }
    }
    
    int cnt = 0;
    auto target = set.size() - 2;
    while (cnt++ < target) {
        auto min_edge = *std::max_element(sims.begin(), sims.end(), [](const Edge &e1, const Edge &e2) {
            return e1.sim < e2.sim;
        });
        auto u1 = find(min_edge.first);
        auto u2 = find(min_edge.second);
        if (u1 == u2) continue;
        for (auto nbr : u1->neighbours) {
            assert(u1->parent == u1);
            if (u1 == find(nbr)) continue;
            if (sims.find(Edge(u1, find(nbr), 0.f)) != sims.end()) {
                sims.erase(sims.find(Edge(u1, find(nbr), 0.f)));
            }
        }
        for (auto nbr : u2->neighbours) {
            assert(u2->parent == u2);
            if (u2 == find(nbr)) continue;
            if (sims.find(Edge(u2, find(nbr), 0.f)) != sims.end()) {
                sims.erase(sims.find(Edge(u2, find(nbr), 0.f)));
            }
        }
        auto merged = merge(u1, u2);
        for (auto nbr : merged->neighbours) {
            Union *other = find(nbr);
            if (other == merged) continue;
            float score_size =1. - (merged->size + other->size) / double(img.cols * img.rows);
            float score_texture = 0.f;
            for (int i = 0; i < 8; i++) {
                score_texture += merged->hist[i] * other->hist[i];
            }
            float sim = score_size + score_texture;
            
            sims.emplace(merged, other, sim);
        }
        if (cnt % 100 == 0) {
            showUnions(unions, img.rows, img.cols);
        }
    }
    
    return 0;
}


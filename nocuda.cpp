/// this file exists to remove C++11 from CUDA, to support outdated nvcc compilers
#include "rtree.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>    
#include <assert.h>
#include <math.h>
#include "mergesort.hh"
#include "tbb/tbb.h"

namespace {
/// \todo put these in a header, so they aren't duplicated
/// initialize boundary so the first udpate overrides it.
inline void init_boundary(struct rtree_rect* boundary) {
  boundary->top = ord_t_max;
  boundary->bottom = ord_t_lowest;
  boundary->left = ord_t_max;
  boundary->right = ord_t_lowest;
}
inline void update_boundary(struct rtree_rect* boundary, struct rtree_point* p) {
  /// \todo replace these with CUDA min/max which won't use conditionals
  boundary->top = fmin(p->y, boundary->top);
  boundary->bottom = fmax(p->y, boundary->bottom);
  boundary->left = fmin(p->x, boundary->left);
  boundary->right = fmax(p->x, boundary->right);
}
inline void update_boundary(struct rtree_rect* boundary, struct rtree_rect* node) {
  /// \todo replace these with CUDA min/max which won't use conditionals
  boundary->top = fmin(node->top, boundary->top);
  boundary->bottom = fmax(node->bottom, boundary->bottom);
  boundary->left = fmin(node->left, boundary->left);
  boundary->right = fmax(node->right, boundary->right);
}
} // namespace

// x value ALONE is used for comparison, to create an xpack
bool operator<(const rtree_point& rhs, const rtree_point& lhs) {
  return rhs.x < lhs.x;
}

struct rtree_point* tbb_sort(struct rtree_point* points, const size_t len) {
//  auto lowxpack = [](const struct rtree_point& rhs, const struct rtree_point& lhs) {
//    return rhs.x < rhs.y;
//  };
  tbb::parallel_sort(points, points + len);
  return points;
}

struct rtree cuda_create_rtree_heterogeneously_mergesort(struct rtree_point* points, const size_t len) {
  struct rtree_leaf* leaves = cuda_create_leaves_together(parallel_mergesort(points, points + len), len);
  const size_t leaves_len = DIV_CEIL(len, RTREE_NODE_SIZE);

  rtree_node* previous_level = (rtree_node*) leaves;
  size_t      previous_len = leaves_len;
  size_t      depth = 1; // leaf level is 0
  while(previous_len > RTREE_NODE_SIZE) {
    previous_level = cuda_create_level(previous_level, previous_len);
    previous_len = DIV_CEIL(previous_len, RTREE_NODE_SIZE);
    ++depth;
  }

  rtree_node* root = (rtree_node*) malloc(sizeof(rtree_node));
  init_boundary(&root->bounding_box);
  root->num = previous_len;
  root->children = previous_level;
  for(size_t i = 0, end = previous_len; i != end; ++i)
    update_boundary(&root->bounding_box, &root->children[i].bounding_box);
  ++depth;

  struct rtree tree = {depth, root};
  return tree;
}

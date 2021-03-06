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
inline void init_boundary(rtree_rect* boundary) {
  boundary->top = ord_t_max;
  boundary->bottom = ord_t_lowest;
  boundary->left = ord_t_max;
  boundary->right = ord_t_lowest;
}
inline void update_boundary(rtree_rect* boundary, rtree_point* p) {
  /// \todo replace these with CUDA min/max which won't use conditionals
  boundary->top = fmin(p->y, boundary->top);
  boundary->bottom = fmax(p->y, boundary->bottom);
  boundary->left = fmin(p->x, boundary->left);
  boundary->right = fmax(p->x, boundary->right);
}
inline void update_boundary(rtree_rect* boundary, rtree_rect* node) {
  /// \todo replace these with CUDA min/max which won't use conditionals
  boundary->top = fmin(node->top, boundary->top);
  boundary->bottom = fmax(node->bottom, boundary->bottom);
  boundary->left = fmin(node->left, boundary->left);
  boundary->right = fmax(node->right, boundary->right);
}
} // namespace

// x value ALONE is used for comparison, to create an xpack
static bool operator<(const rtree_point& rhs, const rtree_point& lhs) {
  return rhs.x < lhs.x;
}

rtree_point* tbb_sort(rtree_point* points, const size_t len, const size_t threads) {
//  auto lowxpack = [](const rtree_point& rhs, const rtree_point& lhs) {
//    return rhs.x < rhs.y;
//  };
  tbb::task_scheduler_init init(threads);
  tbb::parallel_sort(points, points + len);
  return points;
}

/// \param threads the number of threads to use when sorting. ONLY used in the 'sort' part of the algorithm
rtree cuda_create_rtree_heterogeneously_mergesort(rtree_point* points, const size_t len, const size_t threads) {
  rtree_leaf* leaves = cuda_create_leaves_together(parallel_mergesort(points, points + len, threads), len);
  const size_t leaves_len = DIV_CEIL(len, RTREE_NODE_SIZE);

  rtree_node* previous_level = (rtree_node*) leaves;
  size_t      previous_len = leaves_len;
  size_t      depth = 1; // leaf level is 0
  while(previous_len > RTREE_NODE_SIZE) {
    previous_level = cuda_create_level(previous_level, previous_len);
    previous_len = DIV_CEIL(previous_len, RTREE_NODE_SIZE);
    ++depth;
  }

  rtree_node* root = new rtree_node();
  init_boundary(&root->bounding_box);
  root->num = previous_len;
  root->children = previous_level;
  for(size_t i = 0, end = previous_len; i != end; ++i)
    update_boundary(&root->bounding_box, &root->children[i].bounding_box);
  ++depth;

  rtree tree = {depth, root};
  return tree;
}

/// SISD sort via single CPU core (for benchmarks)
rtree cuda_create_rtree_sisd(rtree_point* points, const size_t len) {
  std::sort(points, points + len);
  rtree_leaf* leaves = cuda_create_leaves_together(points, len);
  const size_t leaves_len = DIV_CEIL(len, RTREE_NODE_SIZE);

  rtree_node* previous_level = (rtree_node*) leaves;
  size_t      previous_len = leaves_len;
  size_t      depth = 1; // leaf level is 0
  while(previous_len > RTREE_NODE_SIZE) {
    previous_level = cuda_create_level(previous_level, previous_len);
    previous_len = DIV_CEIL(previous_len, RTREE_NODE_SIZE);
    ++depth;
  }

  rtree_node* root = new rtree_node();
  init_boundary(&root->bounding_box);
  root->num = previous_len;
  root->children = previous_level;
  for(size_t i = 0, end = previous_len; i != end; ++i)
    update_boundary(&root->bounding_box, &root->children[i].bounding_box);
  ++depth;

  rtree tree = {depth, root};
  return tree;
}

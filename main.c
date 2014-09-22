#include "rtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define title() do{printf("%s\n", __func__);} while(0)
/*
static inline void msg(const char* m, ...) {
  va_list args;
  va_start(args, m);
  vprintf(m, args);
}
*/

/// generate a uniform random between min and max exclusive
static inline ord_t uniform_frand(const ord_t min, const ord_t max) {
  const double r = (double)rand() / RAND_MAX;
  return min + r * (max - min);
}

static inline struct rtree_point* create_points_together(const size_t num) {
  const ord_t min = 0.0f;
  const ord_t max = 100.0f;

  struct rtree_point* points = malloc(sizeof(struct rtree_point) * num);
  for(size_t i = 0, end = num; i != end; ++i) {
    points[i].x = uniform_frand(min, max);
    points[i].y = uniform_frand(min, max);
    points[i].key = i;
  }
  return points;
}

static inline struct rtree_points create_points(const size_t num) {
  const ord_t min = 0.0f;
  const ord_t max = 100.0f;

  ord_t* x = malloc(sizeof(ord_t) * num);
  struct rtree_y_key* ykey = malloc(sizeof(struct rtree_y_key) * num);
  struct rtree_points points = {x, ykey, num};
  for(size_t i = 0, end = num; i != end; ++i) {
    points.x[i] = uniform_frand(min, max);
    points.ykey[i].y = uniform_frand(min, max);
    points.ykey[i].key = i;
  }
  return points;
}

static inline void destroy_points(struct rtree_points points) {
  free(points.x);
  free(points.ykey);
}

static inline void print_points(struct rtree_points points) {
  printf("x\ty\tkey\n");
  for(size_t i = 0, end = points.length; i != end; ++i)
    printf("%f\t%f\t%d\n", points.x[i], points.ykey[i].y, points.ykey[i].key);
  printf("\n");
}

static inline void print_points_together(struct rtree_point* points, const size_t len) {
  printf("x\ty\tkey\n");
  for(size_t i = 0, end = len; i != end; ++i)
    printf("%f\t%f\t%d\n", points[i].x, points[i].y, points[i].key);
  printf("\n");
}


static inline void test_rtree(const size_t num) {
  title();

  struct rtree_points points = create_points(num);

  struct rtree tree = cuda_create_rtree(points);

  print_points(points);

  rtree_print(tree);

  destroy_points(points);
}

static inline void test_rtree_together(const size_t num) {
  title();

  struct rtree_point* points = create_points_together(num);

  struct rtree tree = cuda_create_rtree_heterogeneously(points, num);

  print_points_together(points, num);

  rtree_print(tree);

  free(points);
}

struct app_arguments {
  bool        success;
  const char* app_name;
  size_t      test_num;
  size_t      array_size;
};

static struct app_arguments parse_args(const int argc, const char** argv) {
  struct app_arguments args;
  args.success = false;

  size_t arg_i = 0;
  if(argc <= arg_i)
    return args;
  args.app_name = argv[arg_i];
  ++arg_i;

  if(argc <= arg_i)
    return args;
  args.test_num = strtol(argv[arg_i], NULL, 10);
  ++arg_i;

  if(argc <= arg_i)
    return args;
  args.array_size = strtol(argv[arg_i], NULL, 10);
  ++arg_i;

  args.success = true;
  return args;
}

static void print_usage(const char* app_name) {
  const char* default_app_name = "rtree";
  printf("usage: %s test_num array_size\n", strlen(app_name) == 0 ? default_app_name : app_name);
  printf("       test 0: CUDA construction\n");
  printf("       test 1: heterogeneous CUDA+TBB construction\n");
  printf("\n");
}

int main(const int argc, const char** argv) {
  srand(4242); // seed deterministically, so results are reproducible

  const struct app_arguments args = parse_args(argc, argv);
  if(!args.success) {
    print_usage(args.app_name);
    return 0;
  }

  if(args.test_num == 0)
    test_rtree(args.array_size);
  else
    test_rtree_together(args.array_size);

  printf("\n");
  return 0;
}

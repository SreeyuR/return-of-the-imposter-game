#include "collision.h"
#include "polygon.h"
#include "scene.h"
#include "test_util.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

void test_collisions() {
    const double M = 1;
    const double L1 = 20;
    const double L2 = 10;
    assert(L2 < L1);
    const double INITIAL_SEPARATION = 100;
    const double V = 5;
    const double DT = 0.1;
    const int STEPS = 10000;
    const double D_THETA = 0.1;
    const double PARTIAL_COLLISION_TIME = INITIAL_SEPARATION / V;
    const double FULL_COLLISION_TIME = (INITIAL_SEPARATION + L2) / V;
    // Try running the same test at different angles
    for (double theta = 0; theta < 2 * PI; theta += D_THETA) {
        vector_t dir = {.x = cos(theta), .y = sin(theta)};
        scene_t *scene = scene_init();
        // initialize squares separated by given distance
        list_t *shape1 = initialize_rectangle(0, -L1 / 2, L1, L1 / 2);
        list_t *shape2 =
            initialize_rectangle(L1 + INITIAL_SEPARATION, -L2 / 2,
                                 L1 + INITIAL_SEPARATION + L2, L2 / 2);
        // rotate both around the origin
        polygon_rotate(shape1, theta, VEC_ZERO);
        polygon_rotate(shape2, theta, VEC_ZERO);
        body_t *body1 = body_init(shape1, M, COLOR_BLACK);
        body_t *body2 = body_init(shape2, M, COLOR_BLACK);
        // body1 is stationary, body2 is traveling at -V in radial direction
        body_set_velocity(body2, vec_multiply(-V, dir));
        scene_add_body(scene, body1);
        scene_add_body(scene, body2);
        for (size_t i = 0; i < STEPS; i++) {
            double time = DT * i;
            collision_info_t coll_info = detect_body_collision(body1, body2);
            scene_tick(scene, DT);
            // Don't test when time is very close to a collision status change
            if (isclose(time, PARTIAL_COLLISION_TIME) ||
                isclose(time, FULL_COLLISION_TIME)) {
                continue;
            }
            if (time < PARTIAL_COLLISION_TIME) {
                assert(coll_info.collided == NO_COLLISION);
                assert(vec_isclose(coll_info.axis, dir));
            } else if (time < FULL_COLLISION_TIME) {
                assert(coll_info.collided == PARTIAL_COLLISION);
                assert(vec_isclose(coll_info.axis, dir));
            } else {
                assert(coll_info.collided == FULL_COLLISION);
                break;
            }
        }
        scene_free(scene);
    }
}

int main(int argc, char *argv[]) {
    // Run all tests if there are no command-line arguments
    bool all_tests = argc == 1;
    // Read test name from file
    char testname[100];
    if (!all_tests) {
        read_testname(argv[1], testname, sizeof(testname));
    }

    DO_TEST(test_collisions)
}

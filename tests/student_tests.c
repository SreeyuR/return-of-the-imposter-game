#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "forces.h"
#include "test_util.h"

list_t *make_shape() {
    list_t *shape = list_init(4, free);
    vector_t *v = malloc(sizeof(*v));
    *v = (vector_t){-1, -1};
    list_add(shape, v);
    v = malloc(sizeof(*v));
    *v = (vector_t){+1, -1};
    list_add(shape, v);
    v = malloc(sizeof(*v));
    *v = (vector_t){+1, +1};
    list_add(shape, v);
    v = malloc(sizeof(*v));
    *v = (vector_t){-1, +1};
    list_add(shape, v);
    return shape;
}

/**
 * Tests that in a system with gravity, the center of mass moves at a
 * constant velocity.
 */
void test_gravity_center_of_mass() {
    const double M1 = 10;
    const double M2 = 20;
    const double G = 1e3;
    const vector_t V1 = {.x = 5, .y = 0};
    const vector_t V2 = {.x = -5, .y = 0};
    const vector_t POS2 = {.x = 0, .y = 30};
    const double DT = 1e-6;
    const int STEPS = 1000000;
    // Expected velocity of the center of mass. The velocity of the center
    // of mass should stay at this initial value.
    const vector_t EXPECTED_COM_V = vec_multiply(
        1 / (M1 + M2), vec_add(vec_multiply(M1, V1), vec_multiply(M2, V2)));
    scene_t *scene = scene_init();
    body_t *mass1 = body_init(make_shape(), M1, (rgba_color_t){0, 0, 0});
    body_set_velocity(mass1, V1);
    scene_add_body(scene, mass1);
    body_t *mass2 = body_init(make_shape(), M2, (rgba_color_t){0, 0, 0});
    body_set_velocity(mass2, V2);
    body_set_centroid(mass2, POS2);
    scene_add_body(scene, mass2);
    create_newtonian_gravity(scene, G, mass1, mass2);
    for (int i = 0; i < STEPS; i++) {
        vector_t com_v = vec_multiply(
            1 / (M1 + M2), vec_add(vec_multiply(M1, body_get_velocity(mass1)),
                                   vec_multiply(M2, body_get_velocity(mass2))));
        assert(vec_isclose(com_v, EXPECTED_COM_V));
        scene_tick(scene, DT);
    }
    scene_free(scene);
}

double kinetic_energy(body_t *body) {
    vector_t v = body_get_velocity(body);
    return body_get_mass(body) * vec_dot(v, v) / 2;
}

double spring_potential_energy(double k, body_t *body1, body_t *body2) {
    double dist =
        vec_distance(body_get_centroid(body1), body_get_centroid(body2));
    return 0.5 * k * dist * dist;
}

/**
 * Tests that the spring force conserves kinetic + potential energy
 */
void test_spring_energy_conservation() {
    const double M1 = 3.7;
    const double M2 = 15;
    const double K = 20;
    const vector_t POS2 = {.x = 0, .y = 30};
    const double DT = 1e-6;
    const int STEPS = 1000000;
    scene_t *scene = scene_init();
    body_t *mass1 = body_init(make_shape(), M1, (rgba_color_t){0, 0, 0});
    scene_add_body(scene, mass1);
    body_t *mass2 = body_init(make_shape(), M2, (rgba_color_t){0, 0, 0});
    body_set_centroid(mass2, POS2);
    scene_add_body(scene, mass2);
    create_spring(scene, K, mass1, mass2);
    double initial_energy = spring_potential_energy(K, mass1, mass2);
    for (int i = 0; i < STEPS; i++) {
        double energy = spring_potential_energy(K, mass1, mass2) +
                        kinetic_energy(mass1) + kinetic_energy(mass2);
        assert(within(1e-4, energy / initial_energy, 1.0));
        scene_tick(scene, DT);
    }
    scene_free(scene);
}

/**
 * Tests that a body starting with velocity v, when drag force is applied,
 * follows the equation v(t) = v0 e^(-gamma t/m)
 */
void test_drag_force_velocity() {
    const double M = 10;
    const double GAMMA = 0.3;
    const double V0 = 40;
    const double DT = 1e-6;
    const int STEPS = 1000000;
    scene_t *scene = scene_init();
    body_t *body = body_init(make_shape(), M, (rgba_color_t){0, 0, 0});
    body_set_velocity(body, (vector_t){.x = V0, .y = 0});
    scene_add_body(scene, body);
    create_drag(scene, GAMMA, body);
    for (int i = 0; i < STEPS; i++) {
        double expected_speed = V0 * exp(-GAMMA * i * DT / M);
        double actual_speed = body_get_velocity(body).x;
        assert(within(1e-4, actual_speed / expected_speed, 1.0));
        scene_tick(scene, DT);
    }
    scene_free(scene);
}

int main(int argc, char *argv[]) {
    // Run all tests if there are no command-line arguments
    bool all_tests = argc == 1;
    // Read test name from file
    char testname[100];
    if (!all_tests) {
        read_testname(argv[1], testname, sizeof(testname));
    }

    DO_TEST(test_gravity_center_of_mass)
    DO_TEST(test_spring_energy_conservation)
    DO_TEST(test_drag_force_velocity)

    puts("student_tests PASS");
}

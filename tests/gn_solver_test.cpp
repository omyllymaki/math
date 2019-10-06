#define BOOST_TEST_MODULE gn_solver_unit_tests

#include <boost/test/included/unit_test.hpp>
#include <armadillo>
#include "../src/gauss-newton/gn_solver.h"
#include "test_utils.cpp"

using namespace arma;

mat signals = create_signals();
auto solver = GNSolver(signals);

BOOST_AUTO_TEST_CASE(fit_positive_values)
{
    mat weights = {1, 1, 2};
    mat signal = sum_signal(weights, signals);
    mat result = solver.solve(signal);
    BOOST_CHECK(is_equal(weights, result));
}

BOOST_AUTO_TEST_CASE(fit_zero_values)
{
    mat weights = {0, 0, 0};
    mat signal = sum_signal(weights, signals);
    mat result = solver.solve(signal);
    BOOST_CHECK(is_equal(weights, result));
}

BOOST_AUTO_TEST_CASE(fit_positive_and_negative_values)
{
    mat weights = {-1, 1, 2};
    mat signal = sum_signal(weights, signals);
    mat result = solver.solve(signal);
    BOOST_CHECK(is_equal(weights, result));

    weights = {1, -1, 2};
    signal = sum_signal(weights, signals);
    result = solver.solve(signal);
    BOOST_CHECK(is_equal(weights, result));
}

BOOST_AUTO_TEST_CASE(fit_very_small_and_large_values)
{
    mat weights = {0.01, 1, 50000};
    mat signal = sum_signal(weights, signals);
    mat result = solver.solve(signal);
    BOOST_CHECK(is_equal(weights, result));
}
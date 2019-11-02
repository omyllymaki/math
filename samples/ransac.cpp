#include "plotting.cpp"
#include "data_generation.cpp"
#include "../src/analytical/linear/ls_solver.h"
#include "../src/numerical/gauss-newton/gn_solver.h"
#include "../src/numerical/gradient_descent/gd_solver.h"
#include "../src/logging/easylogging++.h"
#include <math.h>
#include <armadillo>
#include <algorithm>
#include <random>
#include "../src/common.h"

using arma::mat;
using std::cout;
using std::endl;

INITIALIZE_EASYLOGGINGPP

mat WEIGHTS = {100, -20, 50, -0.5};

std::vector<int> sample_without_replacement(int lb, int ub, int n)
{
    std::vector<int> vec;
    for (size_t i = lb; i <= ub; i++)
    {
        vec.push_back(i);
    }
    std::random_device device;
    std::mt19937 generator(device());
    std::shuffle(vec.begin(), vec.end(), generator);
    std::vector<int> out(vec.begin(), vec.begin() + n);
    return out;
}

int main(int argc, char *argv[])
{
    el::Configurations conf("./logging-config.conf");
    el::Loggers::reconfigureLogger("default", conf);

    auto data_generator = DataGenerator();
    auto L = data_generator.generate_library();
    //auto s = data_generator.generate_noisy_linear_signal(WEIGHTS);
    auto s = data_generator.generate_linear_signal(WEIGHTS);

    std::vector<int> indices = {0, 5, 11, 13, 15, 18, 20, 25, 40, 60};
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(0, 5);
    for (auto &&i : indices)
    {
        s[i] += distribution(generator);
    }

    LOG(INFO) << "True: " << WEIGHTS;

    // auto solver = GNSolver(L);
    //auto solver = LSSolver(L);
    auto solver = GDSolver(L);
    mat result = solver.solve(s);
    LOG(INFO) << "Regular fit: " << result;

    // plot_arma_vec(s);
    // plot_arma_vec(solver.get_signal_estimate());
    // plt::show();

    int n_channels = 4;
    int n_max_iter = 1000;
    float accepted_error = 0.1;
    int n_accepted_points = 70;
    float objective_value_threshold = 0.0001;

    arma::mat solution;
    double lowest_objective_value = 100000000;
    for (size_t round = 0; round < n_max_iter; round++)
    {

        LOG(DEBUG) << "Round " << round;

        // Take n_channels randomly
        arma::mat s_rand, L_rand;
        std::vector<int> indices = sample_without_replacement(0, s.n_elem - 1, n_channels);
        arma::uvec indices_arma = arma::conv_to<arma::uvec>::from(indices);
        s_rand = s.elem(indices_arma).t();
        L_rand = L.cols(indices_arma);

        // Solve using only selected channels
        solver.set_library(L_rand);
        auto result = solver.solve(s_rand);

        LOG(DEBUG) << "Analysis result with random channels: " << result;

        // Using all channels, estimate assumed inliers
        // Assumed inlier is channel where residual is small enough
        solver.set_library(L);
        solver.set_signal(s);
        auto estimate = solver.get_signal_estimate();
        auto residual = solver.get_signal_residual();
        arma::uvec inlier_indices = arma::find(arma::abs(residual) < accepted_error);

        LOG(DEBUG) << "Number on inliers " << inlier_indices.size();

        // If number of assumed inliers is large enough, continue to evaluation
        if (inlier_indices.size() > n_accepted_points)
        {

            // Take channels that are considered as inliers
            arma::mat s_inliers = s.elem(inlier_indices).t();
            arma::mat L_inliers = L.cols(inlier_indices);

            // Solve using assumed inliers
            solver.set_library(L_inliers);
            auto result = solver.solve(s_inliers);
            auto estimate = solver.get_signal_estimate();

            LOG(DEBUG) << "Analysis result with all inliers: " << result;

            // Evaluate objective value which is error value calculated for assumed inliers
            auto objective_value = arma::as_scalar(rmse(estimate, s_inliers));

            LOG(DEBUG) << "Objective value: " << objective_value;

            // Update solution if objective value is best so far
            if (objective_value < lowest_objective_value)
            {
                LOG(DEBUG) << "Solution update";
                LOG(DEBUG) << "Round " << round;
                lowest_objective_value = objective_value;
                LOG(DEBUG) << "Lowest objective value so far: " << lowest_objective_value;
                solution = result;
                LOG(DEBUG) << "Updated solution: " << solution;

                // Stop iteration if target objective value is reached
                if (lowest_objective_value < objective_value_threshold)
                {
                    LOG(DEBUG) << "Object value threshold was reached at round " << round;
                    LOG(DEBUG) << "Iteration will be terminated";
                    break;
                }
            }
        }
    }

    return 0;
}

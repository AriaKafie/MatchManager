
#ifndef STATS_H
#define STATS_H

#include <algorithm>
#include <cmath>

inline double inverseErf(double x) {
    const double pi = 3.14159265358979323846;
    double a = 8 * (pi - 3) / (3 * pi * (4 - pi));
    double y = std::log(1 - x * x);
    double z = 2 / (pi * a) + y / 2;

    double result = std::sqrt(std::sqrt(z * z - y / a) - z);

    return x < 0 ? -result : result;
}

inline double phi_inv(double p) {
    return std::sqrt(2) * inverseErf(2 * p - 1);
}

inline double elo_margin(double wins, double losses, double draws, double confidence = 0.95)
{
    double total = wins + losses + draws;

    if (std::abs(total) < 1e-9)
        return 0;

    double winP = wins / total;
    double drawP = draws / total;
    double lossP = std::max(0.0, 1.0 - winP - drawP);
    double score = (wins + 0.5 * draws) / total;

    double winsDev = winP * std::pow(1.0 - score, 2);
    double drawsDev = drawP * std::pow(0.5 - score, 2);
    double lossesDev = lossP * std::pow(0.0 - score, 2);

    double stddev_score = std::sqrt(winsDev + drawsDev + lossesDev) / std::sqrt(total);

    double z = phi_inv(1.0 - (1.0 - confidence) / 2.0);
    double lower = score + z * -stddev_score;
    double upper = score + z * stddev_score;

    lower = std::clamp(lower, 0.0001, 0.9999);
    upper = std::clamp(upper, 0.0001, 0.9999);

    auto to_elo = [](double p) {
        return -400.0 * std::log10(1.0 / p - 1.0);
    };

    double elo_low = to_elo(lower);
    double elo_high = to_elo(upper);

    return (elo_high - elo_low) / 2.0;
}

inline double elo_diff(double wins, double losses, double draws)
{
    double total = wins + losses + draws;

    if (std::abs(total) < 1e-9)
        return 0;

    return -400 * std::log10(1 / ((wins + 0.5 * draws) / total) - 1);
}

#endif

#include "LookbackOption.h"
#include <algorithm>    // Pour std::max_element et std::min_element
#include <cmath>        // Pour std::exp
#include <random>       // Pour std::mt19937 et std::normal_distribution
#include <stdexcept>    // Pour std::logic_error

// Constructeur de la classe LookbackOption
// Initialise les attributs strike (prix d'exercice), maturity (maturit�), et optionType (type d'option, Call ou Put)
LookbackOption::LookbackOption(double strike_, double maturity_, OptionType optionType_)
    : ExoticOption(strike_, maturity_), optionType(optionType_) {}

// Calcul du payoff en fonction de la trajectoire compl�te
// Pour une option call, le payoff d�pend du prix maximum atteint pendant la p�riode
// Pour une option put, il d�pend du prix minimum atteint
double LookbackOption::payoff(const std::vector<double>& path) const {
    if (optionType == OptionType::Call) {
        double maxPrice = *std::max_element(path.begin(), path.end()); // Trouve le prix maximum
        return std::max(maxPrice - strike, 0.0); // Payoff d'un call
    } else { // OptionType::Put
        double minPrice = *std::min_element(path.begin(), path.end()); // Trouve le prix minimum
        return std::max(strike - minPrice, 0.0); // Payoff d'un put
    }
}

// Impl�mentation d'une m�thode inutilis�e
// Cette m�thode n'est pas applicable pour une option lookback, car le payoff d�pend d'une trajectoire
double LookbackOption::payoff(double spot) const {
    throw std::logic_error("payoff(double spot) is not applicable for LookbackOption.");
}

// Calcul du prix via Monte-Carlo avec la maturit� par d�faut
double LookbackOption::price(const BlackScholesModel& model, int numPaths, int steps) const {
    return this->price(model, numPaths, steps, maturity); // Appelle la m�thode avec la maturit� par d�faut
}

// Calcul du prix via Monte-Carlo avec une maturit� ajust�e
double LookbackOption::price(const BlackScholesModel& model, int numPaths, int steps, double adjustedMaturity) const {
    double sumPayoffs = 0.0; // Somme des payoffs simul�s
    double dt = adjustedMaturity / steps; // Pas temporel

    std::mt19937 rng(std::random_device{}()); // G�n�rateur al�atoire avec graine dynamique
    std::normal_distribution<> dist(0.0, 1.0); // Distribution normale standard

    for (int i = 0; i < numPaths; ++i) {
        std::vector<double> path; // Stocke la trajectoire simul�e
        path.reserve(steps + 1); // Pr�allocation pour les �tapes simul�es

        double spot = model.spot; // Prix initial du sous-jacent
        path.push_back(spot); // Ajoute le prix initial � la trajectoire

        // Simulation de la trajectoire du sous-jacent
        for (int j = 0; j < steps; ++j) {
            double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
            double diffusion = model.volatility * std::sqrt(dt) * dist(rng);
            spot *= std::exp(drift + diffusion); // Mise � jour du prix simul�
            path.push_back(spot); // Ajoute le prix simul� � la trajectoire
        }

        sumPayoffs += payoff(path); // Calcule le payoff pour la trajectoire et l'ajoute � la somme
    }

    // Retourne le prix moyen actualis�
    return std::exp(-model.rate * adjustedMaturity) * (sumPayoffs / numPaths);
}

// Calcul du co�t de r�plication bas� sur la strat�gie de delta hedging
double LookbackOption::hedgeCost(const BlackScholesModel& model, int steps) const {
    int numPaths = 10000; // Nombre de trajectoires pour le calcul Monte-Carlo
    double epsilon = 0.01 * model.spot; // Variation pour les diff�rences finies

    // Initialisation du delta
    BlackScholesModel modelUp = model;
    modelUp.spot += epsilon; // Spot augment�
    BlackScholesModel modelDown = model;
    modelDown.spot -= epsilon; // Spot diminu�

    double priceUp = this->price(modelUp, numPaths, steps); // Prix pour le spot augment�
    double priceDown = this->price(modelDown, numPaths, steps); // Prix pour le spot diminu�
    double previousDelta = 0;
    double delta = (priceUp - priceDown) / (2 * epsilon); // Delta initial

    // R�plication dynamique
    double dt = maturity / steps; // Pas temporel
    double spot = model.spot; // Prix initial
    double cash = delta * spot; // Montant initial en cash

    std::vector<double> path; // Stocke la trajectoire simul�e
    path.push_back(spot); // Ajoute le prix initial

    for (int i = 1; i < steps; ++i) {
        // Ajuste la maturit�
        double adjustedMaturity = maturity - i * dt;

        // Simulation du prix du sous-jacent
        double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
        double diffusion = model.volatility * std::sqrt(dt) * ((double)rand() / RAND_MAX - 0.5);
        spot *= std::exp(drift + diffusion); // Mise � jour du prix simul�

        path.push_back(spot); // Ajoute le prix simul� � la trajectoire

        // Mise � jour du delta
        double previousDelta = delta;

        modelUp.spot = spot + epsilon;
        modelDown.spot = spot - epsilon;

        priceUp = this->price(modelUp, numPaths, steps - i);
        priceDown = this->price(modelDown, numPaths, steps - i);

        delta = (priceUp - priceDown) / (2 * epsilon); // Nouveau delta

        // Ajustement du portefeuille
        cash += (delta - previousDelta) * spot; // Ajustement pour le delta
        cash *= std::exp(model.rate * dt); // Actualisation du cash
    }

    // Retourne le co�t total de r�plication ajust� par le payoff final
    return cash - delta * spot + payoff(path);
}

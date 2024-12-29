#include "LookbackOption.h"
#include <algorithm>    // Pour std::max_element et std::min_element
#include <cmath>        // Pour std::exp
#include <random>       // Pour std::mt19937 et std::normal_distribution
#include <stdexcept>    // Pour std::logic_error

// Constructeur de la classe LookbackOption
// Initialise les attributs strike (prix d'exercice), maturity (maturité), et optionType (type d'option, Call ou Put)
LookbackOption::LookbackOption(double strike_, double maturity_, OptionType optionType_)
    : ExoticOption(strike_, maturity_), optionType(optionType_) {}

// Calcul du payoff en fonction de la trajectoire complète
// Pour une option call, le payoff dépend du prix maximum atteint pendant la période
// Pour une option put, il dépend du prix minimum atteint
double LookbackOption::payoff(const std::vector<double>& path) const {
    if (optionType == OptionType::Call) {
        double maxPrice = *std::max_element(path.begin(), path.end()); // Trouve le prix maximum
        return std::max(maxPrice - strike, 0.0); // Payoff d'un call
    } else { // OptionType::Put
        double minPrice = *std::min_element(path.begin(), path.end()); // Trouve le prix minimum
        return std::max(strike - minPrice, 0.0); // Payoff d'un put
    }
}

// Implémentation d'une méthode inutilisée
// Cette méthode n'est pas applicable pour une option lookback, car le payoff dépend d'une trajectoire
double LookbackOption::payoff(double spot) const {
    throw std::logic_error("payoff(double spot) is not applicable for LookbackOption.");
}

// Calcul du prix via Monte-Carlo avec la maturité par défaut
double LookbackOption::price(const BlackScholesModel& model, int numPaths, int steps) const {
    return this->price(model, numPaths, steps, maturity); // Appelle la méthode avec la maturité par défaut
}

// Calcul du prix via Monte-Carlo avec une maturité ajustée
double LookbackOption::price(const BlackScholesModel& model, int numPaths, int steps, double adjustedMaturity) const {
    double sumPayoffs = 0.0; // Somme des payoffs simulés
    double dt = adjustedMaturity / steps; // Pas temporel

    std::mt19937 rng(std::random_device{}()); // Générateur aléatoire avec graine dynamique
    std::normal_distribution<> dist(0.0, 1.0); // Distribution normale standard

    for (int i = 0; i < numPaths; ++i) {
        std::vector<double> path; // Stocke la trajectoire simulée
        path.reserve(steps + 1); // Préallocation pour les étapes simulées

        double spot = model.spot; // Prix initial du sous-jacent
        path.push_back(spot); // Ajoute le prix initial à la trajectoire

        // Simulation de la trajectoire du sous-jacent
        for (int j = 0; j < steps; ++j) {
            double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
            double diffusion = model.volatility * std::sqrt(dt) * dist(rng);
            spot *= std::exp(drift + diffusion); // Mise à jour du prix simulé
            path.push_back(spot); // Ajoute le prix simulé à la trajectoire
        }

        sumPayoffs += payoff(path); // Calcule le payoff pour la trajectoire et l'ajoute à la somme
    }

    // Retourne le prix moyen actualisé
    return std::exp(-model.rate * adjustedMaturity) * (sumPayoffs / numPaths);
}

// Calcul du coût de réplication basé sur la stratégie de delta hedging
double LookbackOption::hedgeCost(const BlackScholesModel& model, int steps) const {
    int numPaths = 10000; // Nombre de trajectoires pour le calcul Monte-Carlo
    double epsilon = 0.01 * model.spot; // Variation pour les différences finies

    // Initialisation du delta
    BlackScholesModel modelUp = model;
    modelUp.spot += epsilon; // Spot augmenté
    BlackScholesModel modelDown = model;
    modelDown.spot -= epsilon; // Spot diminué

    double priceUp = this->price(modelUp, numPaths, steps); // Prix pour le spot augmenté
    double priceDown = this->price(modelDown, numPaths, steps); // Prix pour le spot diminué
    double previousDelta = 0;
    double delta = (priceUp - priceDown) / (2 * epsilon); // Delta initial

    // Réplication dynamique
    double dt = maturity / steps; // Pas temporel
    double spot = model.spot; // Prix initial
    double cash = delta * spot; // Montant initial en cash

    std::vector<double> path; // Stocke la trajectoire simulée
    path.push_back(spot); // Ajoute le prix initial

    for (int i = 1; i < steps; ++i) {
        // Ajuste la maturité
        double adjustedMaturity = maturity - i * dt;

        // Simulation du prix du sous-jacent
        double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
        double diffusion = model.volatility * std::sqrt(dt) * ((double)rand() / RAND_MAX - 0.5);
        spot *= std::exp(drift + diffusion); // Mise à jour du prix simulé

        path.push_back(spot); // Ajoute le prix simulé à la trajectoire

        // Mise à jour du delta
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

    // Retourne le coût total de réplication ajusté par le payoff final
    return cash - delta * spot + payoff(path);
}

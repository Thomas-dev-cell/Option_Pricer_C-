#include "AsianOption.h"
#include <numeric>  // Pour std::accumulate (calcul de la moyenne)
#include <cmath>    // Pour std::exp (exponentielle)
#include <random>   // Pour std::mt19937 et std::normal_distribution (génération de nombres aléatoires)
#include <stdexcept>  // Pour std::logic_error (gestion des exceptions)

// Constructeur de la classe AsianOption
// Initialise les attributs strike, maturity (via la classe mère ExoticOption) et optionType
AsianOption::AsianOption(double strike_, double maturity_, OptionType optionType_)
    : ExoticOption(strike_, maturity_), optionType(optionType_) {}

// Méthode pour calculer le payoff basé sur une trajectoire complète
// Le payoff dépend de la moyenne arithmétique des prix du sous-jacent
double AsianOption::payoff(const std::vector<double>& path) const {
    double average = std::accumulate(path.begin(), path.end(), 0.0) / path.size(); // Moyenne arithmétique
    if (optionType == OptionType::Call) {
        return std::max(average - strike, 0.0); // Payoff pour un call
    } else { // OptionType::Put
        return std::max(strike - average, 0.0); // Payoff pour un put
    }
}

// Implémentation de la méthode inutilisée (héritée de ExoticOption)
// Cette méthode n'a pas de sens pour une option asiatique et génère une exception si appelée
double AsianOption::payoff(double spot) const {
    throw std::logic_error("payoff(double spot) is not applicable for AsianOption.");
}

// Méthode pour calculer le prix par Monte-Carlo en utilisant la maturité par défaut
double AsianOption::price(const BlackScholesModel& model, int numPaths, int steps) const {
    return this->price(model, numPaths, steps, maturity); // Appelle la version surchargée avec la maturité par défaut
}

// Méthode pour calculer le prix par Monte-Carlo en permettant une maturité ajustée
double AsianOption::price(const BlackScholesModel& model, int numPaths, int steps, double adjustedMaturity) const {
    double sumPayoffs = 0.0; // Somme des payoffs simulés
    double dt = adjustedMaturity / steps; // Pas temporel

    std::mt19937 rng(std::random_device{}()); // Générateur aléatoire avec une graine dynamique
    std::normal_distribution<> dist(0.0, 1.0); // Distribution normale standard

    for (int i = 0; i < numPaths; ++i) {
        double spot = model.spot; // Prix initial du sous-jacent
        std::vector<double> path; // Chemin pour stocker les prix simulés
        path.reserve(steps); // Préallocation mémoire

        // Simulation de la trajectoire du sous-jacent
        for (int j = 0; j < steps; ++j) {
            double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
            double diffusion = model.volatility * std::sqrt(dt) * dist(rng);
            spot *= std::exp(drift + diffusion); // Mise à jour du prix
            path.push_back(spot); // Enregistrement du prix simulé
        }

        sumPayoffs += payoff(path); // Ajout du payoff basé sur la trajectoire
    }

    return std::exp(-model.rate * adjustedMaturity) * (sumPayoffs / numPaths); // Actualisation et moyenne des payoffs
}

// Méthode pour calculer le coût de réplication basé sur le delta hedging
double AsianOption::hedgeCost(const BlackScholesModel& model, int steps) const {
    int numPaths = 10000; // Nombre de trajectoires Monte-Carlo
    double epsilon = 0.01 * model.spot; // Variation pour le calcul des différences finies

    // Initialisation du delta
    BlackScholesModel modelUp = model;
    modelUp.spot += epsilon; // Modèle avec spot augmenté
    BlackScholesModel modelDown = model;
    modelDown.spot -= epsilon; // Modèle avec spot diminué

    double priceUp = this->price(modelUp, numPaths, steps); // Prix pour spot augmenté
    double priceDown = this->price(modelDown, numPaths, steps); // Prix pour spot diminué

    double previousDelta = 0; // Delta à l'étape précédente
    double delta = (priceUp - priceDown) / (2 * epsilon); // Delta initial par différences finies

    double dt = maturity / steps; // Pas temporel
    double spot = model.spot; // Prix initial du sous-jacent
    double cash = delta * spot; // Portefeuille initial en cash

    // Pour sauvegarder la trajectoire
    std::vector<double> path;
    path.push_back(spot);

    for (int i = 1; i < steps; ++i) {
        // Maturité ajustée
        double adjustedMaturity = maturity - i * dt;

        // Simulation du prix du sous-jacent
        double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
        double diffusion = model.volatility * std::sqrt(dt) * ((double)rand() / RAND_MAX - 0.5);
        spot *= std::exp(drift + diffusion); // Mise à jour du prix simulé
        path.push_back(spot); // Enregistrement du prix simulé

        // Calcul du nouveau delta
        double previousDelta = delta;

        modelUp.spot = spot + epsilon;
        modelDown.spot = spot - epsilon;

        priceUp = this->price(modelUp, numPaths, steps - i);
        priceDown = this->price(modelDown, numPaths, steps - i);

        delta = (priceUp - priceDown) / (2 * epsilon); // Nouveau delta

        // Ajustement du portefeuille
        cash += (delta - previousDelta) * spot; // Ajustement en fonction du changement de delta
        cash *= std::exp(model.rate * dt); // Actualisation du cash
    }

    // Retourner le coût total de réplication en soustrayant le payoff
    return cash - delta * spot + payoff(path); // Coût total ajusté par le payoff final
}

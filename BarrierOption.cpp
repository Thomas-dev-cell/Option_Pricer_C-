#include "BarrierOption.h"
#include <random>       // Pour std::mt19937 et std::normal_distribution (g�n�ration de nombres al�atoires)
#include <algorithm>    // Pour std::max et std::min
#include <cmath>        // Pour std::exp
#include <iostream>     // Pour le d�bogage avec std::cout

// Constructeur de la classe BarrierOption
// Initialise les attributs strike, maturity (via la classe m�re ExoticOption), barrier, barrierType, et optionType
BarrierOption::BarrierOption(double strike_, double maturity_, double barrier_, BarrierType barrierType_, OptionType optionType_)
    : ExoticOption(strike_, maturity_), barrier(barrier_), barrierType(barrierType_), optionType(optionType_) {}

// M�thode pour calculer le payoff d'une option barri�re bas�e sur le prix final du sous-jacent
double BarrierOption::payoff(double spot) const {
    if (optionType == OptionType::Call) {
        return std::max(spot - strike, 0.0); // Payoff pour un call
    } else { // OptionType::Put
        return std::max(strike - spot, 0.0); // Payoff pour un put
    }
}

// M�thode pour v�rifier si la barri�re a �t� franchie durant la trajectoire
bool BarrierOption::isBarrierTouched(const std::vector<double>& path) const {
    switch (barrierType) {
        case BarrierType::UpAndOut:
        case BarrierType::UpAndIn:
            // V�rifie s'il y a un franchissement � la hausse
            for (size_t i = 1; i < path.size(); ++i) {
                if (path[i - 1] < barrier && path[i] >= barrier) {
                    return true; // Franchissement � la hausse d�tect�
                }
            }
            return false;

        case BarrierType::DownAndOut:
        case BarrierType::DownAndIn:
            // V�rifie s'il y a un franchissement � la baisse
            for (size_t i = 1; i < path.size(); ++i) {
                if (path[i - 1] > barrier && path[i] <= barrier) {
                    return true; // Franchissement � la baisse d�tect�
                }
            }
            return false;

        default:
            return false; // Type de barri�re non reconnu
    }
}

// M�thode pour calculer le prix en utilisant la maturit� par d�faut
double BarrierOption::price(const BlackScholesModel& model, int numPaths, int steps) const {
    return this->price(model, numPaths, steps, maturity); // Appelle la m�thode surcharg�e avec la maturit� par d�faut
}

// M�thode pour calculer le prix en utilisant une maturit� ajust�e
double BarrierOption::price(const BlackScholesModel& model, int numPaths, int steps, double adjustedMaturity) const {
    double sumPayoffs = 0.0; // Somme des payoffs
    double dt = adjustedMaturity / steps; // Pas temporel

    std::mt19937 rng(std::random_device{}()); // G�n�rateur al�atoire avec graine dynamique
    std::normal_distribution<> dist(0.0, 1.0); // Distribution normale standard

    for (int i = 0; i < numPaths; ++i) {
        double spot = model.spot; // Prix initial du sous-jacent
        std::vector<double> path; // Chemin pour stocker les prix simul�s
        path.reserve(steps); // Pr�allocation m�moire

        // Simulation de la trajectoire
        for (int j = 0; j < steps; ++j) {
            double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
            double diffusion = model.volatility * std::sqrt(dt) * dist(rng);
            spot *= std::exp(drift + diffusion);
            path.push_back(spot);
        }

        // V�rification si la barri�re est franchie
        bool touched = isBarrierTouched(path);

        if ((barrierType == BarrierType::UpAndOut || barrierType == BarrierType::DownAndOut) && touched) {
            sumPayoffs += 0.0; // Option invalide si barri�re franchie
        } else if ((barrierType == BarrierType::UpAndIn || barrierType == BarrierType::DownAndIn) && !touched) {
            sumPayoffs += 0.0; // Option inexistante si barri�re non franchie
        } else {
            sumPayoffs += payoff(path.back()); // Payoff bas� sur le prix final
        }
    }

    return std::exp(-model.rate * adjustedMaturity) * (sumPayoffs / numPaths); // Actualisation et moyenne des payoffs
}

// M�thode pour calculer le co�t de r�plication en utilisant la strat�gie de delta hedging
double BarrierOption::hedgeCost(const BlackScholesModel& model, int steps) const {
    int numPaths = 10000; // Nombre de trajectoires pour les calculs Monte-Carlo
    double epsilon = 0.01 * model.spot; // Variation pour les diff�rences finies

    // Initialisation du delta
    BlackScholesModel modelUp = model;
    modelUp.spot += epsilon; // Spot augment�
    double priceUp = this->price(modelUp, numPaths, steps);

    BlackScholesModel modelDown = model;
    modelDown.spot -= epsilon; // Spot diminu�
    double priceDown = this->price(modelDown, numPaths, steps);

    double previousDelta = 0;
    double delta = (priceUp - priceDown) / (2 * epsilon); // Calcul du delta initial

    // R�plication dynamique
    double dt = maturity / steps; // Pas temporel
    double spot = model.spot; // Prix initial
    double cash = delta * spot; // Portefeuille initial

    std::vector<double> path; // Chemin des prix simul�s
    path.push_back(spot); // Ajout du prix initial
    bool barrierTouched = isBarrierTouched(path); // V�rification initiale de la barri�re

    for (int i = 1; i < steps; ++i) {
        double adjustedMaturity = maturity - i * dt; // Maturit� ajust�e

        // Simulation du prix du sous-jacent
        double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
        double diffusion = model.volatility * std::sqrt(dt) * ((double)rand() / RAND_MAX - 0.5);
        spot *= std::exp(drift + diffusion);
        path.push_back(spot); // Enregistrement du prix actuel
        barrierTouched = isBarrierTouched(path); // Mise � jour de l'�tat de la barri�re

        previousDelta = delta;

        if (barrierTouched) {
            delta = 0; // Si barri�re franchie, delta devient nul
        } else {
            modelUp.spot = spot + epsilon;
            modelDown.spot = spot - epsilon;
            priceUp = this->price(modelUp, numPaths, steps - i, adjustedMaturity);
            priceDown = this->price(modelDown, numPaths, steps - i, adjustedMaturity);
            delta = (priceUp - priceDown) / (2 * epsilon); // Mise � jour du delta
        }

        cash += (delta - previousDelta) * spot; // Ajustement du portefeuille
        cash *= std::exp(model.rate * dt); // Actualisation
    }

    // Calcul final du co�t de r�plication
    double finalPayoff = payoff(spot);
    if ((barrierType == BarrierType::UpAndIn || barrierType == BarrierType::DownAndIn) && barrierTouched) {
        cash += finalPayoff;
    } else if ((barrierType == BarrierType::UpAndOut || barrierType == BarrierType::DownAndOut) && !barrierTouched) {
        cash += finalPayoff;
    }

    return cash - delta * spot; // Co�t total ajust�
}

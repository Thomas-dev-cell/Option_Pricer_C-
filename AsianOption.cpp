#include "AsianOption.h"
#include <numeric>  // Pour std::accumulate (calcul de la moyenne)
#include <cmath>    // Pour std::exp (exponentielle)
#include <random>   // Pour std::mt19937 et std::normal_distribution (g�n�ration de nombres al�atoires)
#include <stdexcept>  // Pour std::logic_error (gestion des exceptions)

// Constructeur de la classe AsianOption
// Initialise les attributs strike, maturity (via la classe m�re ExoticOption) et optionType
AsianOption::AsianOption(double strike_, double maturity_, OptionType optionType_)
    : ExoticOption(strike_, maturity_), optionType(optionType_) {}

// M�thode pour calculer le payoff bas� sur une trajectoire compl�te
// Le payoff d�pend de la moyenne arithm�tique des prix du sous-jacent
double AsianOption::payoff(const std::vector<double>& path) const {
    double average = std::accumulate(path.begin(), path.end(), 0.0) / path.size(); // Moyenne arithm�tique
    if (optionType == OptionType::Call) {
        return std::max(average - strike, 0.0); // Payoff pour un call
    } else { // OptionType::Put
        return std::max(strike - average, 0.0); // Payoff pour un put
    }
}

// Impl�mentation de la m�thode inutilis�e (h�rit�e de ExoticOption)
// Cette m�thode n'a pas de sens pour une option asiatique et g�n�re une exception si appel�e
double AsianOption::payoff(double spot) const {
    throw std::logic_error("payoff(double spot) is not applicable for AsianOption.");
}

// M�thode pour calculer le prix par Monte-Carlo en utilisant la maturit� par d�faut
double AsianOption::price(const BlackScholesModel& model, int numPaths, int steps) const {
    return this->price(model, numPaths, steps, maturity); // Appelle la version surcharg�e avec la maturit� par d�faut
}

// M�thode pour calculer le prix par Monte-Carlo en permettant une maturit� ajust�e
double AsianOption::price(const BlackScholesModel& model, int numPaths, int steps, double adjustedMaturity) const {
    double sumPayoffs = 0.0; // Somme des payoffs simul�s
    double dt = adjustedMaturity / steps; // Pas temporel

    std::mt19937 rng(std::random_device{}()); // G�n�rateur al�atoire avec une graine dynamique
    std::normal_distribution<> dist(0.0, 1.0); // Distribution normale standard

    for (int i = 0; i < numPaths; ++i) {
        double spot = model.spot; // Prix initial du sous-jacent
        std::vector<double> path; // Chemin pour stocker les prix simul�s
        path.reserve(steps); // Pr�allocation m�moire

        // Simulation de la trajectoire du sous-jacent
        for (int j = 0; j < steps; ++j) {
            double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
            double diffusion = model.volatility * std::sqrt(dt) * dist(rng);
            spot *= std::exp(drift + diffusion); // Mise � jour du prix
            path.push_back(spot); // Enregistrement du prix simul�
        }

        sumPayoffs += payoff(path); // Ajout du payoff bas� sur la trajectoire
    }

    return std::exp(-model.rate * adjustedMaturity) * (sumPayoffs / numPaths); // Actualisation et moyenne des payoffs
}

// M�thode pour calculer le co�t de r�plication bas� sur le delta hedging
double AsianOption::hedgeCost(const BlackScholesModel& model, int steps) const {
    int numPaths = 10000; // Nombre de trajectoires Monte-Carlo
    double epsilon = 0.01 * model.spot; // Variation pour le calcul des diff�rences finies

    // Initialisation du delta
    BlackScholesModel modelUp = model;
    modelUp.spot += epsilon; // Mod�le avec spot augment�
    BlackScholesModel modelDown = model;
    modelDown.spot -= epsilon; // Mod�le avec spot diminu�

    double priceUp = this->price(modelUp, numPaths, steps); // Prix pour spot augment�
    double priceDown = this->price(modelDown, numPaths, steps); // Prix pour spot diminu�

    double previousDelta = 0; // Delta � l'�tape pr�c�dente
    double delta = (priceUp - priceDown) / (2 * epsilon); // Delta initial par diff�rences finies

    double dt = maturity / steps; // Pas temporel
    double spot = model.spot; // Prix initial du sous-jacent
    double cash = delta * spot; // Portefeuille initial en cash

    // Pour sauvegarder la trajectoire
    std::vector<double> path;
    path.push_back(spot);

    for (int i = 1; i < steps; ++i) {
        // Maturit� ajust�e
        double adjustedMaturity = maturity - i * dt;

        // Simulation du prix du sous-jacent
        double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
        double diffusion = model.volatility * std::sqrt(dt) * ((double)rand() / RAND_MAX - 0.5);
        spot *= std::exp(drift + diffusion); // Mise � jour du prix simul�
        path.push_back(spot); // Enregistrement du prix simul�

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

    // Retourner le co�t total de r�plication en soustrayant le payoff
    return cash - delta * spot + payoff(path); // Co�t total ajust� par le payoff final
}

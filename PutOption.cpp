#include "PutOption.h"
#include "BlackScholesModel.h" // Pour accéder aux paramètres du modèle Black-Scholes
#include <cmath> // Pour les calculs mathématiques, notamment std::max et std::exp
#include <algorithm> // Pour std::max

// Constructeur de la classe PutOption
// Initialise les paramètres strike (prix d'exercice) et maturity (maturité) via la classe mère Option
PutOption::PutOption(double strike_, double maturity_)
    : Option(strike_, maturity_) {}

// Méthode pour calculer le payoff d'un put
// Le payoff d'un put est défini comme max(K - S, 0), où S est le prix du sous-jacent et K le strike
double PutOption::payoff(double spot) const {
    return std::max(strike - spot, 0.0); // Renvoie le payoff du put
}

// Méthode pour calculer le coût de réplication par delta hedging
// Utilise le modèle Black-Scholes pour ajuster dynamiquement le portefeuille
double PutOption::hedgeCost(const BlackScholesModel& model, int steps) const {
    double dt = maturity / steps; // Intervalle de temps entre deux étapes
    double spot = model.spot; // Prix initial du sous-jacent
    double previousDelta = 0.0; // Delta à l'étape précédente, initialisé à 0

    // Calcul du delta initial (t = 0)
    double d1 = (std::log(spot / strike) +
                (model.rate - model.dividend + 0.5 * model.volatility * model.volatility) * maturity) /
                (model.volatility * std::sqrt(maturity)); // Formule de Black-Scholes pour d1
    double currentDelta = std::exp(-model.dividend * maturity) *
                          (0.5 * (1.0 + std::erf(d1 / std::sqrt(2.0))) - 1.0); // Delta initial pour un put
    double cash = currentDelta * spot; // Montant initial en cash

    // Boucle pour ajuster dynamiquement le portefeuille à chaque étape
    for (int i = 1; i <= steps; ++i) {
        double time = i * dt; // Temps actuel

        // Simulation du prix du sous-jacent
        if (i < steps) { // Pas de simulation au dernier pas
            double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
            double diffusion = model.volatility * std::sqrt(dt) * ((double)rand() / RAND_MAX - 0.5); // Perturbation aléatoire
            spot *= std::exp(drift + diffusion); // Mise à jour du prix simulé
        }

        previousDelta = currentDelta; // Stockage du delta précédent

        // Calcul du delta à l'instant t (t = maturity - time)
        d1 = (std::log(spot / strike) +
                     (model.rate - model.dividend + 0.5 * model.volatility * model.volatility) * (maturity - time)) /
                    (model.volatility * std::sqrt(maturity - time)); // Mise à jour de d1
        currentDelta = std::exp(-model.dividend * (maturity - time)) *
                       (0.5 * (1.0 + std::erf(d1 / std::sqrt(2.0))) - 1.0); // Nouveau delta pour un put

        // Ajustement du portefeuille
        cash += (currentDelta - previousDelta) * spot; // Ajustement du cash pour refléter le changement de delta
        cash *= std::exp(model.rate * dt); // Actualisation du cash avec le taux sans risque
    }

    // Retourner le coût total de réplication
    // Le coût est ajusté par le dernier delta et le payoff final
    return cash - currentDelta * spot + payoff(spot);
}

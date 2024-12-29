#include "PutOption.h"
#include "BlackScholesModel.h" // Pour acc�der aux param�tres du mod�le Black-Scholes
#include <cmath> // Pour les calculs math�matiques, notamment std::max et std::exp
#include <algorithm> // Pour std::max

// Constructeur de la classe PutOption
// Initialise les param�tres strike (prix d'exercice) et maturity (maturit�) via la classe m�re Option
PutOption::PutOption(double strike_, double maturity_)
    : Option(strike_, maturity_) {}

// M�thode pour calculer le payoff d'un put
// Le payoff d'un put est d�fini comme max(K - S, 0), o� S est le prix du sous-jacent et K le strike
double PutOption::payoff(double spot) const {
    return std::max(strike - spot, 0.0); // Renvoie le payoff du put
}

// M�thode pour calculer le co�t de r�plication par delta hedging
// Utilise le mod�le Black-Scholes pour ajuster dynamiquement le portefeuille
double PutOption::hedgeCost(const BlackScholesModel& model, int steps) const {
    double dt = maturity / steps; // Intervalle de temps entre deux �tapes
    double spot = model.spot; // Prix initial du sous-jacent
    double previousDelta = 0.0; // Delta � l'�tape pr�c�dente, initialis� � 0

    // Calcul du delta initial (t = 0)
    double d1 = (std::log(spot / strike) +
                (model.rate - model.dividend + 0.5 * model.volatility * model.volatility) * maturity) /
                (model.volatility * std::sqrt(maturity)); // Formule de Black-Scholes pour d1
    double currentDelta = std::exp(-model.dividend * maturity) *
                          (0.5 * (1.0 + std::erf(d1 / std::sqrt(2.0))) - 1.0); // Delta initial pour un put
    double cash = currentDelta * spot; // Montant initial en cash

    // Boucle pour ajuster dynamiquement le portefeuille � chaque �tape
    for (int i = 1; i <= steps; ++i) {
        double time = i * dt; // Temps actuel

        // Simulation du prix du sous-jacent
        if (i < steps) { // Pas de simulation au dernier pas
            double drift = (model.rate - model.dividend - 0.5 * model.volatility * model.volatility) * dt;
            double diffusion = model.volatility * std::sqrt(dt) * ((double)rand() / RAND_MAX - 0.5); // Perturbation al�atoire
            spot *= std::exp(drift + diffusion); // Mise � jour du prix simul�
        }

        previousDelta = currentDelta; // Stockage du delta pr�c�dent

        // Calcul du delta � l'instant t (t = maturity - time)
        d1 = (std::log(spot / strike) +
                     (model.rate - model.dividend + 0.5 * model.volatility * model.volatility) * (maturity - time)) /
                    (model.volatility * std::sqrt(maturity - time)); // Mise � jour de d1
        currentDelta = std::exp(-model.dividend * (maturity - time)) *
                       (0.5 * (1.0 + std::erf(d1 / std::sqrt(2.0))) - 1.0); // Nouveau delta pour un put

        // Ajustement du portefeuille
        cash += (currentDelta - previousDelta) * spot; // Ajustement du cash pour refl�ter le changement de delta
        cash *= std::exp(model.rate * dt); // Actualisation du cash avec le taux sans risque
    }

    // Retourner le co�t total de r�plication
    // Le co�t est ajust� par le dernier delta et le payoff final
    return cash - currentDelta * spot + payoff(spot);
}

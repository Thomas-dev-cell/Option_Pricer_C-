#include "BlackScholesModel.h"
#include <iostream> // Inclus pour l'affichage et le d�bogage si n�cessaire

// Constructeur
// Initialise les param�tres du mod�le Black-Scholes : spot (prix initial du sous-jacent),
// rate (taux sans risque), volatility (volatilit�), et dividend (taux de dividende)
BlackScholesModel::BlackScholesModel(double spot_, double rate_, double volatility_, double dividend_)
    : spot(spot_), rate(rate_), volatility(volatility_), dividend(dividend_) {}

// M�thode pour le calcul analytique des options vanilles (calls et puts)
// Utilise la formule de Black-Scholes pour calculer le prix
double BlackScholesModel::priceAnalytic(const Option *option, bool isCall) const {
    // Calcul de d1 et d2, des param�tres interm�diaires de la formule de Black-Scholes
    double d1 = (std::log(spot / option->strike) +
                 (rate - dividend + 0.5 * volatility * volatility) * option->maturity) /
                (volatility * std::sqrt(option->maturity));
    double d2 = d1 - volatility * std::sqrt(option->maturity);

    // Calcul des probabilit�s cumul�es associ�es � d1 et d2 pour une loi normale standard
    double Nd1 = normalCDF(d1);  // Probabilit� cumul�e pour d1
    double Nd2 = normalCDF(d2);  // Probabilit� cumul�e pour d2
    double Nmd1 = normalCDF(-d1); // Probabilit� cumul�e pour -d1
    double Nmd2 = normalCDF(-d2); // Probabilit� cumul�e pour -d2

    // Calcul du prix en fonction du type d'option (call ou put)
    if (isCall) {
        // Prix du call : S*exp(-qT)*N(d1) - K*exp(-rT)*N(d2)
        return spot * std::exp(-dividend * option->maturity) * Nd1 -
               option->strike * std::exp(-rate * option->maturity) * Nd2;
    } else {
        // Prix du put : K*exp(-rT)*N(-d2) - S*exp(-qT)*N(-d1)
        return option->strike * std::exp(-rate * option->maturity) * Nmd2 -
               spot * std::exp(-dividend * option->maturity) * Nmd1;
    }
}

// M�thode priv�e pour la fonction de r�partition (CDF) de la loi normale standard
// Utilise la fonction std::erfc, qui donne la fonction d'erreur compl�mentaire
// M_SQRT1_2 est une constante pour 1 / sqrt(2)
double BlackScholesModel::normalCDF(double x) const {
    return 0.5 * std::erfc(-x * M_SQRT1_2); // CDF = 0.5 * erfc(-x / sqrt(2))
}

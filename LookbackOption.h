#ifndef LOOKBACK_OPTION_H
#define LOOKBACK_OPTION_H

#include "ExoticOption.h"
#include "OptionType.h"
#include <vector>

class LookbackOption : public ExoticOption {
public:
    OptionType optionType; // Type d'option (Call ou Put)

    // Constructeur
    LookbackOption(double strike_, double maturity_, OptionType optionType_);

    // Méthode pour le payoff basé sur une trajectoire complète
    double payoff(const std::vector<double>& path) const;

    // Implémentation de la méthode virtuelle pour le payoff (par défaut inutilisée)
    double payoff(double spot) const override;

    // Méthode de pricing Monte-Carlo que l'on va utiliser en cas de maturité constante
    double price(const BlackScholesModel& model, int numPaths, int steps) const override;

    // Surcharge de la méthode ci-dessus permettant de prendre en argument la maturité (utile pour calculer le delta)
    double price(const BlackScholesModel& model, int numPaths, int steps, double adjustedMaturity) const;

    // Coût de réplication basé sur la stratégie de couverture dynamique
    double hedgeCost(const BlackScholesModel& model, int steps) const override;
};

#endif // LOOKBACK_OPTION_H

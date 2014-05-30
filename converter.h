#ifndef CONVERTER_H
#define CONVERTER_H

#include "stable.h"

class Converter : QObject
{
    public:
        static bool StringMatchesRegexp(QString str, QRegExp *reg);
        static QStringList StringMatchesRegexprRes(QString str, QRegExp *reg);
        static QList<QStringList> StringMatchesRegexprMultiRes(QString str, QRegExp *reg);

    private:
        QWidget *parent;

        // Début du fichier cpp
        QString base;
        // Fin du fichier cpp
        QString end;
        // Pseudo code
        QString source;
        // Progression du cpp
        QString cpp;

        // Non utilisés, pourquoi pas dans un futur proche (check si les catégories principales sont bien là)
        bool algohere;
        bool varhere;
        bool instructshere;

        /*
         *  Variables temporaires, pour stocker les sous algos avant de les insérer dans cpp.
         */
        // Stocke tous les sous algos
        QString sousalgo;
        // Stocke le dernier sous algo
        QString lastSousAlgo;
        // Stocke les paramètres
        QString lastParams;
        // Liste des mots qui ont besoin d'être préfixés par "*"
        QStringList needPointer;
        // Si on est dans Paramètres d'entrée
        bool contextParamsEntree;
        // Si on est dans Paramètres de sortie
        bool contextParamsSortie;
        // Liste des sous-algorithmes définis
        QStringList sousAlgosDefinis;

        // Tous les structs
        QString structs;

        // La liste ordonnée des structures de contrôle
        QStringList structuresControl;

        // Toutes les variables "globales"
        QString globalVariables;

        // Les types des variables de base
        QHash<QString, QString> types;

        // Liste des arrays (quand on envoie un array en paramètres d'entrée/de sortie
        QStringList arrays;
        // Les dimensiosn des tableaux en paramètre d'entrée.
        QHash<QString, QString> arraysSizes;
        // Memcopy des arrays
        QHash<QString, QString> arraysToNewArrays;
        // Le template des tailles en paramètres.
        QStringList arrayTemplateSize;

        // Arrête l'algo avec l'erreur de compilation
        QString stop(QString text, QWidget *parent);

        /*
         * Expressions régulières qui permettent de détecter toutes les instructions NF04
         */
        QRegExp *regEqual;
        QRegExp *regAlgoTitle;
        QRegExp *regAffectation;
        QRegExp *regComment;
        QRegExp *regEcrire;
        QRegExp *regEmptyLine;
        QRegExp *regFinPour;
        QRegExp *regFinSi;
        QRegExp *regFinTantQue;
        QRegExp *regInit;
        QRegExp *regLire;
        QRegExp *regParametresEntrees;
        QRegExp *regParametresSorties;
        QRegExp *regPourParPas;
        QRegExp *regSousAlgo;
        QRegExp *regRepeter;
        QRegExp *regSi;
        QRegExp *regSinonSi;
        QRegExp *regSinon;
        QRegExp *regFinAlgo;
        QRegExp *regTypes;
        QRegExp *regTantQue;
        QRegExp *regTraitement;
        QRegExp *regVariableEntier;
        QRegExp *regVariableReel;
        QRegExp *regVariableCaractere;
        QRegExp *regVariableBooleen;
        QRegExp *regVariableTab;
        QRegExp *regVariableTabInside;
        QRegExp *regVariableArtDef;
        QRegExp *regVariableArt;
        QRegExp *regFunctionCall;

    public:
        Converter(QString source, QWidget *parent = 0);
        /*
         * Compilateur en C++
         * partial : si on doit compiler en C++ uniquement une partie de texte
         * Utile par exemple au Selon, à ne pas enlever.
         */
        QString toCPP(bool partial = false);
};

#endif // CONVERTER_H

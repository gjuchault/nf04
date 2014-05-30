#include "converter.h"

Converter::Converter(QString source, QWidget *parent)
{
    this->source = source;
    this->parent = parent;
    QFile baseFile(":/base.cpp");
    baseFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&baseFile);
    base = in.readAll();

    /*
     * Initialisation des variables
     */
    // system("pause") sans system (puisque system est un appel à l'OS)
    end = "\n";
    end += "    cout << \"Appuyez sur entree pour terminer...\";\n";
    end += "    cin.sync();\n";
    end += "    cin.ignore(1);\n";
    end += "    return 0;\n";
    end += "}";
    sousalgo = "";
    globalVariables = "";
    contextParamsSortie = false;
    contextParamsEntree = false;
    regEqual = new QRegExp("([!><])==");

    /*
     * Regexps
     * Modèle :
     * \\s => Espace, \r, \n, \t, \0, etc.
     * \\s* => Espace en début de mot
     * \\S+ => Tout sauf des espaces => alphanumérique
     * (Expr) => Paranthèses capturantes
     * \\/ => Slash
     * ? => caractère optionnel
     * http://gethifi.com/tools/regex
     */
    regAlgoTitle = new QRegExp("Algorithme ?:?(\\S+)", Qt::CaseInsensitive);
    regAffectation = new QRegExp("\\s*(\\S+) ?<- ?(.+)", Qt::CaseInsensitive);
    regComment = new QRegExp("\\s*\\/\\/([^\n]*)", Qt::CaseInsensitive);
    regEcrire = new QRegExp("\\s*Ecrire ?\\(([^\\n]+) ?! ?\\)", Qt::CaseInsensitive);
    regEmptyLine = new QRegExp("^\\s*$", Qt::CaseInsensitive);
    regFinPour = new QRegExp("\\s*FinPour", Qt::CaseInsensitive);
    regFinSi = new QRegExp("\\s*FinSi", Qt::CaseInsensitive);
    regFinAlgo = new QRegExp("\\s*Fin \\S+", Qt::CaseInsensitive);
    regFinTantQue = new QRegExp("\\s*(FinTantQue|FinTq)", Qt::CaseInsensitive);
    regInit = new QRegExp("\\s*Initialisation|Variables?:?", Qt::CaseInsensitive);
    regLire = new QRegExp("Lire ?\\(clavier ?! ?(\\S*)\\)", Qt::CaseInsensitive);
    regParametresEntrees = new QRegExp("Param[è|e]tres? d\\'entr[e|é]e ?:? ?", Qt::CaseInsensitive);
    regParametresSorties = new QRegExp("Param[è|e]tres? de sortie ?:? ?", Qt::CaseInsensitive);
    regPourParPas = new QRegExp("\\s*Pour (\\S+)( allant)? ?de (\\S+) [à|a] (\\S+) par pas de (\\S+)( Faire)? ?", Qt::CaseInsensitive);
    regSi = new QRegExp("\\s*Si (.+) Alors", Qt::CaseInsensitive);
    regSinonSi = new QRegExp("\\s*SinonSi (.+) Alors", Qt::CaseInsensitive);
    regSinon = new QRegExp("\\s*Sinon", Qt::CaseInsensitive);
    regSousAlgo = new QRegExp("Sous-?Algorithme ?:? ?(\\S+)", Qt::CaseInsensitive);
    regTypes = new QRegExp("\\s*Types? ?:?", Qt::CaseInsensitive);
    regTantQue = new QRegExp("\\s*Tant ?que (.+)( [F|f]aire)? ?", Qt::CaseInsensitive);
    regTraitement = new QRegExp("\\s*Instructions?|Traitements? ?:?", Qt::CaseInsensitive);
    regVariableEntier = new QRegExp("\\s*(\\S+) ?: ?entier", Qt::CaseInsensitive);
    regVariableReel = new QRegExp("\\s*(\\S+) ?: ?r[é|e]el", Qt::CaseInsensitive);
    regVariableCaractere = new QRegExp("\\s*(\\S+) ?: ?caract[è|e]re", Qt::CaseInsensitive);
    regVariableBooleen = new QRegExp("\\s*(\\S+) ?: ?bool[é|e]en", Qt::CaseInsensitive);
    regVariableTab = new QRegExp("\\s*(\\S+) ?: ?tableau ?\\[(.+)\\] ?d[e|'] ?([^s]+)s?", Qt::CaseInsensitive);
    // On met un * pour le deuxième => paramètres d'entrée d'un S-A => pas de taille.
    regVariableTabInside = new QRegExp("(\\d+)..(\\d*),? ?", Qt::CaseInsensitive);
    regVariableArtDef = new QRegExp("\\s*(\\S+) ?: ?article ?\\((.+)\\)", Qt::CaseInsensitive);
    // ?!.* => tout ce qui n'est pas Entier, Booléen etc.
    regVariableArt = new QRegExp("\\s*(\\S+) ?: ?(?!.*(Entier|Bool[é|e]en|Caract[è|e]|R[é|e]el))(\\S+)", Qt::CaseInsensitive);
    // * => pas forcément d'entrée ou de retour
    regFunctionCall = new QRegExp("\\s*(\\S+)\\(([^!]*)!(.*)\\)", Qt::CaseInsensitive);
}

QString Converter::toCPP(bool partial)
{
    cpp = "";

    /*
     * Si ce n'est pas une compilation partielle, on ajoute la base, et on check
     * les catégories principales
     */
    if (!partial) {
        cpp = base;

        if (!StringMatchesRegexp(source, regAlgoTitle)) {
            return stop(tr("Rubrique algorithme manquante"), parent);
        }

        if (!StringMatchesRegexp(source, regInit)) {
            return stop(tr("Rubrique Initialisation/Variable(s) manquante"), parent);
        }

        if (!StringMatchesRegexp(source, regTraitement)) {
            return stop(tr("Rubrique Traitement(s)/Instruction(s) manquante"), parent);
        }
    }

    // Rempalcements d'opérateurs "inline" : et, ou, vrai, faux, valeur absolue
    source = source.replace(" et ", " && ", Qt::CaseInsensitive).replace(" ou ", " || ", Qt::CaseInsensitive); // Opérateurs basiques
    source = source.replace("VRAI", "true", Qt::CaseInsensitive).replace("FAUX", "false", Qt::CaseInsensitive);
    source = source.replace("E(", "(int)trunc(");
    source = source.replace("abs(", "real_abs(");
    // L'indentaiton (sera augmentée ou réduite selon les instructions)
    QString indent = "\n    ";
    // On split le fichier ligne par ligne, on garde les lignes vides (aucune idée pourquoi, nvm)
    QStringList sourceSplitted = source.split(QRegExp("\\n"), QString::KeepEmptyParts);
    // r va contenir les paranthèses capturantes à chaque instruction détectée.
    QStringList r;
    // Va augmenter à chaque Pour, réduire à chaque FinPour => Détecter les non fermetures
    int numberOfPour = 0;
    // Idem pour tant que
    int numberOfTantQue = 0;
    /*
     * Trois variables de contextes non utilisées
     */
    bool isInVariables = false;
    bool isInTypes = false;
    bool isInTraitement = false;
    // A chaque template, on doit changer de nom
    int templateCounter = 1;
    // Numéro de ligne dans le code NF04
    int lineN = 1;
    foreach (QString line, sourceSplitted) {

        // Division flottante
        if (!StringMatchesRegexp(line, regEcrire) && !StringMatchesRegexp(line, regComment)) {
            line = line.replace("/", "/(double)");
        }

        /*
         * lineS => "//22"
         * C'est un identificateur ajouté à chaque instruction.
         * Si le compilateur throw une error, il va loger la ligne,
         * Il suffira de récupérer ce commentaire spécial pour retrouver
         * la ligne dans le code NF04 où l'erreur à été thrown
         */
        QString lineS = (partial) ? "" : "//" + QVariant(lineN).toString();
        // La target (avec référence pour modifier) est la cible de l'ajout : soit le sous algo, soit le cpp
        QString &target = (lastSousAlgo == "") ? cpp : lastSousAlgo;

        /*
         * Identification des blocs :
         * Si la ligne correspond à un bloc précis Alors
         *     on récupère le contenu des paranthèses dans r
         *     on écrit l'équivalent en C++
         * Sinon ...
         */
        if (StringMatchesRegexp(line, regComment)) {
            // Si on commence par un commentaire, on continue
            continue;
        } else if (StringMatchesRegexp(line, regAlgoTitle) && !StringMatchesRegexp(line, regSousAlgo)) {
            // Si on match le titre d'un algo, on écrit juste le titre de l'algo
            r = StringMatchesRegexprRes(line, regAlgoTitle);
            target += indent +  "cout << \"Algorithme: " + r.at(1) + "\" << endl;" + lineS;
            algohere = true;
        } else if (StringMatchesRegexp(line, regAffectation)) {
            // Si on a une affecation, deux cas : soit dans une struct, soit non.
            r = StringMatchesRegexprRes(line, regAffectation);

            if (types[r.at(1).split(".")[0]] == "") {
                // Pas de type => c'est un article donc une structure => "->"
                r[1] = r[1].replace(".", "->");
                target += indent + r.at(1) + " =" + r.at(2) + ";" + lineS;
            } else {
                // On remplace la virgule française dans les nombres par un point anglais (décimales)
                if (types[r.at(1)] != "string") {
                    r[2] = r[2].replace(",", ".");
                }

                target += indent + r.at(1) + " = " + r.at(2) + ";" + lineS;
            }
        } else if (StringMatchesRegexp(line, regEcrire)) {
            // Si on match Ecrire, on fait un cout.
            r = StringMatchesRegexprRes(line, regEcrire);
            QString out = r.at(1);
            // Attention, on remplace toutes les "," par des operateurs stream-in.
            QStringList outs = out.replace(",", "<<").split("<<");
            foreach (QString arg, outs) {
                arg = arg.trimmed();
            }
            out = outs.join(" << ");
            target += indent + "cout << " + out.replace("&&", "et").replace("||", "ou").replace("false", "faux").replace("true", "vrai") + " << endl;" + lineS;
        } else if (StringMatchesRegexp(line, regFinPour)) {
            // Si on match un finPour, on baisse l'indent, on ajoute une bracket right.
            numberOfPour--;
            indent = indent.left(indent.length() - 4);
            target += indent + "}" + lineS;
            structuresControl << "finpour";
        } else if (StringMatchesRegexp(line, regFinSi)) {
            // Idem pour le FinSi
            indent = indent.left(indent.length() - 4);
            target += indent + "}" + lineS;
            structuresControl << "finsi";
        } else if (StringMatchesRegexp(line, regFinAlgo)) {
            /*
             * Un sous algo sera de type void, et ses paramètres de sorties seront en fait ses paramètres d'entrée, mais avec leur référence/pointeur.
             * Ici, on ajoute variables qui ont besoin d'un pointeur leur *
             */
            // S'il y a un sous algo
            if (lastSousAlgo != "") {
                // On le ferme
                lastSousAlgo += "\n}\n";
                // On remplace les arguments par les "vrais" arguments
                // Le lastParams.left(lastParams.length() - 2) supprime le dernier ", " ajouté à chaque paramètre
                lastSousAlgo = lastSousAlgo.replace("[args]", lastParams.left(lastParams.length() - 2));
                // Pour chaque variable ayant besoin d'un pointeur, on lui ajoute le pointeur
                foreach(QString pointerneeded, needPointer) {
                    QRegExp addPointer = QRegExp(" " + pointerneeded + "([, )])");
                    // Le \\1 correpond à la première paranthèse capturante
                    lastSousAlgo = lastSousAlgo.replace(addPointer, " *" + pointerneeded + "\\1");
                    // On remplace les strutcs pointeur-isé
                    lastSousAlgo = lastSousAlgo.replace(pointerneeded + ".", pointerneeded + "->");
                    /*
                     * Si on a un identificateur spécial annulant le pointeurneeded, on ne lui ajoute pas de pointeur
                     * Exemple : un pointeur sur un tableau n'a pas besoin d'un deuxième pointeur s'il est renvoyé dans un
                     * autre sous algo.
                     */
                    lastSousAlgo = lastSousAlgo.replace("---" + pointerneeded, pointerneeded);
                }

                // Si on utilise des tableaux en Paramètres d'entrée, on template leur size
                if (arrayTemplateSize.length() > 0) {
                    lastSousAlgo = "template<" + arrayTemplateSize.join(", ") + ">\n" + lastSousAlgo;
                }

                // On fixe au cas où un paramètre de sortie à le même nom que l'algo
                lastSousAlgo = lastSousAlgo.replace("void *", "void ");
                // On réinitialise les variables.
                lastParams = "";
                sousalgo += lastSousAlgo;
                lastSousAlgo = "";
                needPointer = QStringList();
                isInVariables = false;
                isInTypes = false;
                isInTraitement = false;
            }
        } else if (StringMatchesRegexp(line, regFinTantQue)) {
            // Idem FinPour/FinSi
            numberOfTantQue--;
            indent = indent.left(indent.length() - 4);
            target += indent + "}" + lineS;
            structuresControl << "fintq";
        } else if (StringMatchesRegexp(line, regInit)) {
            isInTraitement = false;
            isInTypes = false;
            isInVariables = true;
            varhere = true;

            if (contextParamsSortie) {
                // On vient de finir les parmaètres du sous algo, on les ajoute au sous algo.
                contextParamsSortie = false;
                // On remplace les arguments par les "vrais" arguments
                // Le lastParams.left(lastParams.length() - 2) supprime le dernier ", " ajouté à chaque paramètre
                //lastSousAlgo = lastSousAlgo.replace("[args]", lastParams.left(lastParams.length() - 2));
            }
        } else if (StringMatchesRegexp(line, regLire)) {
            r = StringMatchesRegexprRes(line, regLire);
            QString var = r.at(1);
            QString type = types[var];

            if (type == "custom") {
                return stop("Lecture dans un article à la ligne " + lineS.replace("//", ""), parent);
            } else if (var.trimmed().length() == 0) {
                target += indent + "cin.sync();";
                target += indent + "cin.ignore(1);";
            } else if (type == "string") {
                target += indent + var + " = cin.get();" + lineS;
            } else {
                target += indent + "cin >> " + var + ";" + lineS;
            }
        } else if (StringMatchesRegexp(line, regParametresEntrees)) {
            contextParamsEntree = true; // Utile pour les variables
        } else if (StringMatchesRegexp(line, regParametresSorties)) {
            contextParamsEntree = false;
            contextParamsSortie = true;
        } else if (StringMatchesRegexp(line, regPourParPas)) {
            numberOfPour++;
            r = StringMatchesRegexprRes(line, regPourParPas);
            int gap = QVariant(r.at(5)).toInt();
            QString rgap = QVariant(abs(gap)).toString();

            if (gap < 0) { // Pas négatif => la boucle est inversée
                target += indent + "for (" + r.at(1) + " = " + r.at(3) + "; " + r.at(1) + " >= " + r.at(4) + "; " + r.at(1) + "-=" + rgap + ")" + lineS;
            } else {
                target += indent + "for (" + r.at(1) + " = " + r.at(3) + "; " + r.at(1) + " <= " + r.at(4) + "; " + r.at(1) + "+=" + rgap + ")" + lineS;
            }

            target += indent + "{";
            indent += "    ";
            structuresControl << "pour";
        } else if (StringMatchesRegexp(line, regSi) && !StringMatchesRegexp(line, regSinonSi)) {
            r = StringMatchesRegexprRes(line, regSi);
            QString cond = r.at(1);
            cond.replace("=", "==").replace(*regEqual, "\\1=");
            target += indent + "if(" + cond + ")" + lineS;
            target += indent + "{";
            indent += "    ";
            structuresControl << "si";
        } else if (StringMatchesRegexp(line, regSinonSi)) {
            r = StringMatchesRegexprRes(line, regSinonSi);
            QString cond = r.at(1);
            cond.replace("=", "==").replace(*regEqual, "\\1=");
            indent = indent.left(indent.length() - 4);
            target += indent + "}";
            target += indent + "else if(" + cond + ")" + lineS;
            target += indent + "{";
            indent += "    ";
        } else if (StringMatchesRegexp(line, regSinon) && !StringMatchesRegexp(line, regSinonSi)) {
            r = StringMatchesRegexprRes(line, regSinon);
            indent = indent.left(indent.length() - 4);
            target += indent + "}";
            target += indent + "else" + lineS;
            target += indent + "{";
            indent += "    ";
        } else if (StringMatchesRegexp(line, regSousAlgo)) {
            r = StringMatchesRegexprRes(line, regSousAlgo);
            sousAlgosDefinis.append(r.at(1));
            /*
             * On place un void et on met [args] qui sera remplacé. Le retour se fait avec des pointeurs dans les args
             */
            lastSousAlgo = "void " + r.at(1) + "([args])" + lineS + "\n";
            lastSousAlgo += "{\n";
        } else if (StringMatchesRegexp(line, regTantQue)) {
            numberOfTantQue++;
            r = StringMatchesRegexprRes(line, regTantQue);
            // La regexp absorbe le Faire dans le (.+); il faut donc l'enlever à la main
            QString cond = r[1].replace(QRegExp("Faire", Qt::CaseInsensitive), "");
            cond = cond.replace("=", "==").replace(*regEqual, "\\1=");
            target += indent + "while(" + cond + ")" + lineS;
            target += indent + "{";
            indent += "    ";
            structuresControl << "tq";
        } else if (StringMatchesRegexp(line, regTraitement)) {
            isInTypes = false;
            isInVariables = false;
            isInTraitement = true;
            instructshere = true;

            if (contextParamsSortie) { // Si l'algo n'a pas la catégorie Variables, c'est Traitement qui replace les args
                contextParamsSortie = false;
            }
        } else if (StringMatchesRegexp(line, regTypes)) {
            isInVariables = false;
            isInTraitement = false;
            isInTypes = true;
        }
        // Même syntaxe partout : soit on est dans les arguments d'entrée, soit de sortie (pointeur), soit dans main (classique).
        else if (StringMatchesRegexp(line, regVariableEntier) && !StringMatchesRegexp(line, regVariableArtDef)) {
            if (isInTraitement) {
                return stop(tr("Déclaration de variable dans la section d'instructions à la ligne ") + lineS.replace("//", ""), parent);
            }

            if (isInTypes) {
                return stop(tr("Déclaration de variable dans la section des types à la ligne ") + lineS.replace("//", ""), parent);
            }

            r = StringMatchesRegexprRes(line, regVariableEntier);

            if (contextParamsEntree) {
                lastParams += "int " + r.at(1) + ", ";
            } else if (contextParamsSortie) {
                lastParams += "int " + r.at(1) + ", "; // C'est le need pointer qui ajoutera le pointeur tout seul
                needPointer << r.at(1);
            } else {
                if (partial) {
                    cpp += "int " + r.at(1) + ";";
                } else if (lastSousAlgo.length() > 0) {
                    lastSousAlgo += "    int " + r.at(1) + ";" + lineS + "\n";
                } else {
                    if (types.value(r.at(1), NULL) != NULL) {
                        return stop(tr("Variable déjà déclarée à la ligne ") + lineS.replace("//", ""), parent);
                    }

                    types[r.at(1)] = "int";
                    globalVariables += "    int " + r.at(1) + ";" + lineS + "\n";
                }
            }
        } else if (StringMatchesRegexp(line, regVariableReel) && !StringMatchesRegexp(line, regVariableArtDef)) {
            if (isInTraitement) {
                return stop(tr("Déclaration de variable dans la section d'instructions à la ligne ") + lineS.replace("//", ""), parent);
            }

            if (isInTypes) {
                return stop(tr("Déclaration de variable dans la section des types à la ligne ") + lineS.replace("//", ""), parent);
            }

            r = StringMatchesRegexprRes(line, regVariableReel);

            if (contextParamsEntree) {
                lastParams += "double " + r.at(1) + ", ";
            } else if (contextParamsSortie) {
                lastParams += "double " + r.at(1) + ", ";
                needPointer << r.at(1);
            } else {
                if (partial) {
                    cpp += "double" + r.at(1) + ";";
                } else if (lastSousAlgo.length() > 0) {
                    lastSousAlgo += "    double " + r.at(1) + ";" + lineS + "\n";
                } else {
                    if (types.value(r.at(1), NULL) != NULL) {
                        return stop(tr("Variable déjà déclarée à la ligne ") + lineS.replace("//", ""), parent);
                    }

                    types[r.at(1)] = "double";
                    globalVariables += "    double " + r.at(1) + ";" + lineS + "\n";
                }
            }
        } else if (StringMatchesRegexp(line, regVariableCaractere) && !StringMatchesRegexp(line, regVariableArtDef)) {
            if (isInTraitement) {
                return stop(tr("Déclaration de variable dans la section d'instructions à la ligne ") + lineS.replace("//", ""), parent);
            }

            if (isInTypes) {
                return stop(tr("Déclaration de variable dans la section des types à la ligne ") + lineS.replace("//", ""), parent);
            }

            r = StringMatchesRegexprRes(line, regVariableCaractere);

            if (contextParamsEntree) {
                lastParams += "char " + r.at(1) + ", ";
            } else if (contextParamsSortie) {
                lastParams += "char " + r.at(1) + ", ";
                needPointer << r.at(1);
            } else {
                if (partial) {
                    cpp += "char " + r.at(1) + ";";
                } else if (lastSousAlgo.length() > 0) {
                    lastSousAlgo += "    char " + r.at(1) + ";" + lineS + "\n";
                } else {
                    if (types.value(r.at(1), NULL) != NULL) {
                        return stop(tr("Variable déjà déclarée à la ligne ") + lineS.replace("//", ""), parent);
                    }

                    types[r.at(1)] = "string";
                    globalVariables += "    char " + r.at(1) + ";" + lineS + "\n";
                }
            }
        } else if (StringMatchesRegexp(line, regVariableBooleen) && !StringMatchesRegexp(line, regVariableArtDef)) {
            if (isInTraitement) {
                return stop(tr("Déclaration de variable dans la section d'instructions à la ligne ") + lineS.replace("//", ""), parent);
            }

            if (isInTypes) {
                return stop(tr("Déclaration de variable dans la section des types à la ligne ") + lineS.replace("//", ""), parent);
            }

            r = StringMatchesRegexprRes(line, regVariableBooleen);

            if (contextParamsEntree) {
                lastParams += "bool " + r.at(1) + ", ";
            } else if (contextParamsSortie) {
                lastParams += "bool " + r.at(1) + ", ";
                needPointer << r.at(1);
            } else {
                if (partial) {
                    cpp += "bool " + r.at(1) + ";";
                } else if (lastSousAlgo.length() > 0) {
                    lastSousAlgo += "    bool " + r.at(1) + ";" + lineS + "\n";
                } else {
                    if (types.value(r.at(1), NULL) != NULL) {
                        return stop(tr("Variable déjà déclarée à la ligne ") + lineS.replace("//", ""), parent);
                    }

                    types[r.at(1)] = "bool";
                    globalVariables += "    bool " + r.at(1) + ";" + lineS + "\n";
                }
            }
        } else if (StringMatchesRegexp(line, regVariableArt) && !StringMatchesRegexp(line, regVariableArtDef) && !StringMatchesRegexp(line, regVariableTab)) {
            if (isInTraitement) {
                return stop(tr("Déclaration d'un article dans la section d'instructions à la ligne ") + lineS.replace("//", ""), parent);
            }

            r = StringMatchesRegexprRes(line, regVariableArt);

            if (contextParamsEntree) {
                lastParams += r.at(2) + " " + r.at(1) + ", ";
            } else if (contextParamsSortie) {
                lastParams += r.at(2) + " " + r.at(1) + ", ";
                needPointer << r.at(1);
            } else {
                if (partial) {
                    cpp += r.at(2) + " " + r.at(1) + ";";
                } else if (lastSousAlgo.length() > 0) {
                    lastSousAlgo += "    " + r.at(2) + " " + r.at(1) + ";" + lineS + "\n";
                } else {
                    if (types.value(r.at(1), NULL) != NULL) {
                        return stop(tr("Variable déjà déclarée à la ligne ") + lineS.replace("//", ""), parent);
                    }

                    types[r.at(1)] = "custom";
                    globalVariables += "    " + r.at(2) + " " + r.at(1) + ";" + lineS + "\n";
                }
            }
        } else if (StringMatchesRegexp(line, regVariableTab) && !StringMatchesRegexp(line, regVariableArtDef)) {
            if (isInTraitement) {
                return stop(tr("Déclaration de variable dans la section d'instructions à la ligne ") + lineS.replace("//", ""), parent);
            }

            if (isInTypes) {
                return stop(tr("Déclaration de variable dans la section des types à la ligne ") + lineS.replace("//", ""), parent);
            }

            r = StringMatchesRegexprRes(line, regVariableTab);
            QList<QStringList> r2 = StringMatchesRegexprMultiRes(r.at(2), regVariableTabInside); // QRegexp n'a pas de récursivité, on doit donc découper en deux regex
            QString tab = "";
            QString tabForSousAlgo = "";
            QStringList templateForSousAlgo;
            int tabFullSize = 1;
            foreach (QStringList dimension, r2) {
                /*
                 * Finalement, que le tableau commence à i=0 ou i=1; ça ne change pas grand chose
                 * pour le final user puisqu'il n'est pas au courant que son tableau à une case vide en 0
                 */
                tabFullSize *= dimension.at(2).toInt() + 1;
                // On ajoute au tempalte une variable
                templateForSousAlgo.append("int size" + QVariant(templateCounter).toString());

                // Et on met cette variable en taille du tableau prédéfinie
                tabForSousAlgo += "[size" + QVariant(templateCounter).toString() + "]";

                // On augmente le counter pour éviter d'avoir les mêmes variables dans les templates
                ++templateCounter;
                tab += "[" + QVariant(dimension.at(2).toInt() + 1).toString() + "]";
            }
            QString type = r.at(3).trimmed().toLower().replace("é", "e").replace("è", "e");
            type = (type == "reel") ? "double" : (type == "entier") ? "int" : (type == "caractere") ? "char" : (type == "booleen") ? "bool" : r.at(3).trimmed();
            arrays.append(r.at(1));

            if (contextParamsEntree) {
                /*
                 * J'ai posé la question ici. En passant par référence le tableau; le template fonctionne.
                 * http://stackoverflow.com/questions/22028351/template-size-multidimensionnal-arrays-c/22028707?noredirect=1#22028707
                 */
                // Si on est dans les PE, on ajoute aux params la forme "bizarre" (référence + sizes templates)
                lastParams += type + " (&" + r.at(1) + ")" + tabForSousAlgo + ", ";
                // Et on ajoute le template
                arrayTemplateSize << templateForSousAlgo;
            } else if (contextParamsSortie) {
                // En param de sortie, on met la forme du tableau classique
                lastParams += type + " " + r.at(1) + tab + ", ";
            } else {
                // Sinon on déclare le tableau
                if (types.value(r.at(1), NULL) != NULL) {
                    return stop("Variable déjà déclarée à la ligne " + lineS.replace("//", ""), parent);
                }

                types[r.at(1)] = "array";

                if (partial) {
                    cpp += type + " " + r.at(1) + tab + ";";
                } else if (lastSousAlgo.length() > 0) {
                    lastSousAlgo += "    " + type + " " + r.at(1) + tab + ";" + lineS + "\n";
                } else {
                    // On crée le tableau et son équivalent memcopied
                    globalVariables += type + " " + r.at(1) + tab + ";" + lineS;
                    QString memcopyName = r.at(1) + "_memcopied_";
                    QString createArrayFromThisArray = indent + type + " " + memcopyName + tab + ";\n";
                    createArrayFromThisArray += indent + "memcpy(&" + memcopyName + "[0], &" + r.at(1) + "[0], sizeof(" + r.at(1) + "));";
                    QString resetArrayFromThisArray = "memcpy(&" + memcopyName + "[0], &" + r.at(1) + "[0], sizeof(" + r.at(1) + "));";
                    globalVariables += createArrayFromThisArray;
                    arraysToNewArrays[r.at(1)] = resetArrayFromThisArray;
                    arraysSizes[r.at(1)] = tab;
                }
            }
        } else if (StringMatchesRegexp(line, regFunctionCall) && !StringMatchesRegexp(line, regEcrire) && !StringMatchesRegexp(line, regLire)) {
            // Les fonctions lire et ecrire sont particulières
            r = StringMatchesRegexprRes(line, regFunctionCall);
            QString argsIn = r.at(2);
            QString argsOut = r.at(3);
            // On met des pointeurs en PS
            argsOut = argsOut.trimmed().replace(QRegExp("(\\w+)"), "&\\1");
            // Fix les bugs du type : nomSouAlgo(&u[&1])
            argsOut = argsOut.replace(QRegExp("\\[&(\\d+)\\]"), "[\\1]");
            QStringList argsOutList = argsOut.split(",");

            for (int argsOutI = 0; argsOutI < argsOutList.size(); ++argsOutI) {
                argsOutList[argsOutI] = argsOutList[argsOutI].trimmed();

                /*
                 * Si on envoie un paramètre de sortie en paramètre de sortie, il faut éviter les double pointeurs.
                 * Donc on préfixe par le caractère "---". Quand on ajoutera le sous algo, on remplacera "---" par "".
                 */
                if (needPointer.contains(argsOutList.at(argsOutI).mid(1))) {
                    argsOutList[argsOutI] = "---" + argsOutList.at(argsOutI).mid(1);
                }

                if (arrays.contains(argsOutList.at(argsOutI).mid(1))) {
                    argsOutList[argsOutI] = argsOutList.at(argsOutI).mid(1);
                }
            }

            argsOut = argsOutList.join(", ");
            QStringList argsInList = argsIn.split(",");

            for (int argsInI = 0; argsInI < argsInList.size(); ++argsInI) {
                argsInList[argsInI] = argsInList[argsInI].trimmed();

                if (arrays.contains(argsInList.at(argsInI))) {
                    // Si c'est un tableau en PE, on le memcopy
                    QStringList memcopycode = arraysToNewArrays[argsInList.at(argsInI)].split("\n");
                    target += indent + memcopycode[0];
                    argsInList[argsInI] += "_memcopied_";
                }
            }

            argsIn = argsInList.join(", ");

            if (!sousAlgosDefinis.contains(r.at(1))) {
                return stop(tr("Fonction inconnue à la ligne ") + lineS.replace("//", ""), parent);
            }

            QString replace = r.at(1) + "(";

            if (argsIn.trimmed().length() > 0) {
                replace += argsIn.trimmed();

                if (argsOut.length() > 0) {
                    replace += ", ";
                }
            }

            if (argsOut.length() > 0) {
                replace += argsOut;
            }

            target += indent + replace + ");" + lineS;
        } else if (StringMatchesRegexp(line, regVariableArtDef) && isInTypes) {
            // Article <=> Structure
            if (isInVariables) {
                return stop(tr("Déclaration d'un article dans la section des variables à la ligne ") + lineS.replace("//", ""), parent);
            }

            // Un article est simplement un typedef structuré
            r = StringMatchesRegexprRes(line, regVariableArtDef);
            structs += "typedef struct " + r.at(1) + " " + r.at(1) + ";" + lineS + "\n";
            structs += "struct " + r.at(1) + lineS + "\n";
            structs += "{\n";
            QStringList varsinside = r.at(2).split(";");
            foreach (QString varinside, varsinside) {
                Converter *insideConverter = new Converter(varinside, parent);
                structs += "    " + insideConverter->toCPP(true) + lineS + "\n";
            }
            structs += "};\n";
        } else if (StringMatchesRegexp(line, regEmptyLine)) {
            // Do Nothing
        } else {
            qDebug() << line;
            return stop(tr("Instruction non reconnue à la ligne ") + lineS.replace("//", ""), parent);
        }

        // Si on est dans les variables et qu'on fait autre chose qu'une déclaration : error
        if (isInVariables) {
            if (!StringMatchesRegexp(line, regVariableArt) && !StringMatchesRegexp(line, regVariableBooleen) && !StringMatchesRegexp(line, regVariableCaractere)
                    && !StringMatchesRegexp(line, regVariableEntier) && !StringMatchesRegexp(line, regVariableReel) && !StringMatchesRegexp(line, regVariableTab)
                    && !StringMatchesRegexp(line, regVariableArtDef) && !StringMatchesRegexp(line, regInit) && !StringMatchesRegexp(line, regEmptyLine)) {
                return stop(tr("Instruction impossible dans la section de variables à la ligne ") + lineS.replace("//", ""), parent);
            }
        }

        lineN++;
    }

    if (numberOfPour != 0) {
        return stop(tr("Il y a plus de Pour que de FinPour"), parent);
    }

    if (numberOfTantQue != 0) {
        return stop(tr("Il y a plus de TantQue que de FinTantQue"), parent);
    }

    // Vérification de la structure de contrôle
    QStringList toDown;
    foreach (QString currentStruct, structuresControl) {
        if (currentStruct == "si" || currentStruct == "pour" || currentStruct == "tq") {
            toDown.prepend("fin" + currentStruct);
        } else {
            if (toDown.at(0) == currentStruct) {
                toDown.removeAt(0);
            } else {
                return stop(tr("Erreur dans l'ordre de fermeture des structures conditionnelles."), parent);
            }
        }
    }

    if (!partial) {
        cpp = cpp.replace("sousalgo", sousalgo).replace("structs", structs).replace("globalVariables", globalVariables);
        //cpp = cpp.replace("\\0", "\\n");
        cpp += end;
    }

    cpp = cpp.replace("ç", "\\x87").replace("â", "\\x83").replace("ê", "\\x88").replace("ë", "\\x89").replace("è", "\\x8A").replace("é", "\\x82");
    cpp = cpp.replace("â", "\\x83").replace("à", "\\x85").replace("ï", "\\x8B").replace("î", "\\x8C").replace("ô", "\\x93").replace("û", "\\x96");
    cpp = cpp.replace("ö", "\\x94");
    return cpp;
}

bool Converter::StringMatchesRegexp(QString str, QRegExp *reg)
{
    // Match only
    reg->setCaseSensitivity(Qt::CaseInsensitive);
    int r = reg->indexIn(str);

    if (r == -1) {
        return false;
    }

    return true;
}

QStringList Converter::StringMatchesRegexprRes(QString str, QRegExp *reg)
{
    // Match + return des paranthèses capturantes
    reg->setCaseSensitivity(Qt::CaseInsensitive);
    int r = reg->indexIn(str);

    if (r == -1) {
        return QStringList();
    }

    return reg->capturedTexts();
}

QList<QStringList> Converter::StringMatchesRegexprMultiRes(QString str, QRegExp *reg)
{
    // Match + multiple return des paranthèses capturantes
    reg->setCaseSensitivity(Qt::CaseInsensitive);
    QList<QStringList> list;
    int pos = 0;
    pos = reg->indexIn(str, pos);

    while (pos != -1) {
        list << reg->capturedTexts();
        pos += reg->matchedLength();
        pos = reg->indexIn(str, pos);
    }

    return list;
}


QString Converter::stop(QString text, QWidget *parent)
{
    QMessageBox::critical(parent, tr("Erreur de compilation"), text + ".");
    return "error";
}

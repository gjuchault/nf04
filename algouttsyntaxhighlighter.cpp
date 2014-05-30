#include "algouttsyntaxhighlighter.h"

/**
 * @brief AlgoUttSyntaxHighLighter::AlgoUttSyntaxHighLighter
 * @param document
 * @param dark
 */
AlgoUttSyntaxHighLighter::AlgoUttSyntaxHighLighter(QTextDocument *document, bool dark) : QSyntaxHighlighter(document)
{
    this->dark = dark;

#ifdef Q_OS_WIN
    QFile file(QApplication::applicationDirPath() + "/autocompletion.txt");
#else
    QFile file(QApplication::applicationDirPath() + "/../../../autocompletion.txt");
#endif

    if ( ! file.open(QFile::ReadOnly)) {
        this->words = QStringList();
    }

    while ( ! file.atEnd()) {
        QByteArray line = file.readLine();

        if ( ! line.isEmpty()) {
            // On retire tous les caractères \r, \n, ...; mais pas les espaces
            this->words << QString(line.replace(" ", "0").trimmed()).replace("0", " ");
        }
    }
}

/**
 * Change le style de la coloration.
 *
 * Si « dark » est à true, le thème sera le sombre.
 * Sinon, le thème clair sera utilisé.
 *
 * @brief AlgoUttSyntaxHighLighter::setDark
 * @param dark
 */
void AlgoUttSyntaxHighLighter::setDark(bool dark)
{
    this->dark = dark;
}

/**
 * Application de la couleur aux mots.
 *
 * Chaque mot est passé à la moulinette à travers plusieurs expressions
 * régulières afin de lui assigner une couleur.
 *
 * @brief AlgoUttSyntaxHighLighter::highlightBlock
 * @param text
 */
void AlgoUttSyntaxHighLighter::highlightBlock(const QString &text)
{
    QTextCharFormat format;
    // Couleurs par défaut en fonction du thème
    format.setForeground(QBrush((dark) ? QColor("#DDD") : QColor("#000")));
    setFormat(0, text.length(), format);
    format.setForeground(QBrush((dark) ? QColor("#F92672") : QColor("#1990B8")));

    /*
     * Pour chaque mot, on crée une expression régulière.
     * En fonction de celui-ci, on va utiliser des couleurs prédéfinies différentes.
     * Ensuite on applique les couleurs à chaque bloc qui a matché (en l'occurence
     * chaque mot). On les remplacera un à un en itérant sur le contenu grace à
     * la méthode indexOf().
     */
    for (int i = 0; i < words.length(); ++i) {
        QString pattern = words.at(i);
        pattern = pattern.replace("é", "[é|e]").replace("è", "[è|e]").replace("ù", "[ù|u]").replace("à", "[à|a]");
        QRegExp expression(pattern);
        format.setUnderlineStyle(QTextCharFormat::NoUnderline);
        format.setForeground(QBrush((dark) ? QColor("#F92672") : QColor("#5A5CAD")));


        if (pattern == "Algorithme"
         || pattern == "Fin"
         || pattern == "Initialisation"
         || pattern == "Instructions"
         || pattern == "Parametres d'entree"
         || pattern == "Parametres de sortie"
         || pattern == "Sous-Algorithme"
         || pattern == "Traitement"
         || pattern == "Type"
         || pattern == "Types"
         || pattern == "Variables") {
            format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            format.setUnderlineColor((dark) ? QColor("#66D9EF") : QColor("#2F9C0A"));
            format.setForeground(QBrush((dark) ? QColor("#66D9EF") : QColor("#2F9C0A")));
        }
        else if (pattern == "Entier"
              || pattern == "Entiers"
              || pattern == "R[é|e]el"
              || pattern == "R[é|e]els"
              || pattern == "Bool[é|e]en"
              || pattern == "Bool[é|e]ens"
              || pattern == "Caract[è|e]re"
              || pattern == "Caract[è|e]res") {
            format.setForeground(QBrush((dark) ? QColor("#A6E22E") : QColor("#a67f59")));
        }
        else if (pattern == "E") {
            expression = QRegExp("E\\(");
        }

        expression.setCaseSensitivity(Qt::CaseInsensitive);
        int index = text.indexOf(expression);

        while (index >= 0) {

            int length = expression.matchedLength();

            // Fix un bug sur la valeur absolue
            if (pattern == "E") {
                --length;
            }
            // Evite de colorier le par dans paramètres
            else if (pattern == "par" && text.mid(index, length + 7).toLower() == "parametres") {
                break;
            }
            // Colorie uniquement "de" s'il y a un espace ensuite
            else if (pattern == "de" && text.mid(index, length + 1).toLower() != "de ") {
                break;
            }
            // Idem
            else if (pattern == "Que" && text.mid(index, length + 1).toLower() != "que ") {
                break;
            }
            // Idem
            else if (pattern == "Fin" && text.mid(index, length + 1).toLower() != "fin ") {
                break;
            }

            setFormat(index, length, format);
            index = text.indexOf(expression, index + length);
        }
    }

    QRegExp *isNum = new QRegExp("[0-9\\.]");
    QRegExp *isChar = new QRegExp("[a-zA-Z]");

    for (int j = 0; j < text.length(); ++j) {
        if (text.mid(j, 2) == "//") {
            setFormat(j, text.length() - j, QColor("#75715E"));
            break;
        }

        if (isNum->indexIn(text.mid(j, 1)) != -1) {
            if (j > 0 && isChar->indexIn(text.mid(j - 1, 1)) == -1) {
                setFormat(j, 1, (dark) ? QColor("#AE81FF") : QColor("#164"));
            }
        }
    }

    QString pattern = "\"[^\\n]*\"";
    QRegExp expression(pattern);
    int index = text.indexOf(expression);

    while (index >= 0) {
        int length = expression.matchedLength();
        setFormat(index, length, (dark) ? QColor("#BF79DB") : QColor("#D14"));
        index = text.indexOf(expression, index + length);
    }
}

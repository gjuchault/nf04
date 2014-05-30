#include "converter.h"
#include "aboutwindow.h"
#include "syntaxwindow.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

/**
 * @brief MainWindow::MainWindow
 * @param parent
 * @param lng
 */
MainWindow::MainWindow(QWidget *parent, QString lng) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (lng == "en") {
        ui->actionAnglais->setChecked(true);
        ui->actionFran_ais->setChecked(false);
    } else {
        ui->actionAnglais->setChecked(false);
        ui->actionFran_ais->setChecked(true);
    }

    // Le thème par défaut est le sombre
    this->dark = true;

    // On instancie l'éditeur
    textEdit = new CodeEditor(this, dark);

    // On l'ajoute à la grille
    ui->gridLayout->addWidget(textEdit, 0, 0, 1, 1);

    //On crée la complétion depuis le fichier
#ifdef Q_OS_WIN
    completer = new QCompleter(modelFromFile(QApplication::applicationDirPath() + "/autocompletion.txt"), this);
#else
    qDebug() << QApplication::applicationDirPath() + "/../../../autocompletion.txt";
    completer = new QCompleter(modelFromFile(QApplication::applicationDirPath() + "/../../../autocompletion.txt"), this);
#endif
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    textEdit->setCompleter(completer);
    textEdit->setTabStopWidth(28);

    // On ajoute la font depuis les ressources
    fdb.addApplicationFont(":/ressources/MONACO.TTF");
#ifdef Q_OS_WIN
    QFont monaco = fdb.font("Monaco", "Normal", 9);
#else
    QFont monaco = fdb.font("Monaco", "Normal", 12);
#endif
    monaco.setLetterSpacing(QFont::AbsoluteSpacing , QVariant(0.75).toReal());
    textEdit->setFont(monaco);
    textEdit->setFocus(Qt::OtherFocusReason);

    // On crée le colorieur
    SH = new AlgoUttSyntaxHighLighter(textEdit->document(), dark);

    // On ajoute le bouton start
    QPushButton *start = new QPushButton(tr("Lancer"));
    connect(start, SIGNAL(clicked()), this, SLOT(start()));
    ui->statusBar->addPermanentWidget(start);

    // On connect les actions des menus au slots de MainWindow
    connect(ui->actionNouveau_Fichier, SIGNAL(triggered()), this, SLOT(slotNew()));
    connect(ui->actionOuvrir, SIGNAL(triggered()), this, SLOT(slotOpen()));
    connect(ui->actionOuvrir_dans_une_nouvelle_fen_tre, SIGNAL(triggered()), this, SLOT(slotOpenNewWindow()));
    connect(ui->actionEnregistrer, SIGNAL(triggered()), this, SLOT(slotSave()));
    connect(ui->actionEnregistrer_Sous, SIGNAL(triggered()), this, SLOT(slotSaveAs()));
    connect(ui->actionQuitter, SIGNAL(triggered()), this, SLOT(slotQuit()));
    connect(ui->actionSyntaxe, SIGNAL(triggered()), this, SLOT(slotSyntax()));
    connect(ui->actionA_propos, SIGNAL(triggered()), this, SLOT(slotAbout()));
    connect(ui->actionOuvrir_le_code_C, SIGNAL(triggered()), this, SLOT(slotOpenCPP()));
    connect(ui->actionClair, SIGNAL(triggered()), this, SLOT(slotClair()));
    connect(ui->actionFonc, SIGNAL(triggered()), this, SLOT(slotFonce()));
    connect(ui->actionOuvrir_le_log_de_compilation, SIGNAL(triggered()), this, SLOT(slotOpenLog()));
    connect(textEdit, SIGNAL(sDropEvent(char *)), this, SLOT(autoOpen(char *)));
    connect(ui->actionFran_ais, SIGNAL(triggered()), this, SLOT(slotLngFR()));
    connect(ui->actionAnglais, SIGNAL(triggered()), this, SLOT(slotLngEN()));

    // On connect aussi les raccourcis clavier
    QShortcut *ctrlS = new QShortcut(QKeySequence::Save, this);
    connect(ctrlS, SIGNAL(activated()), this, SLOT(slotSave()));
    QKeySequence seqCtrlMajS(Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    QShortcut *ctrlMajS = new QShortcut(seqCtrlMajS, this);
    connect(ctrlMajS, SIGNAL(activated()), this, SLOT(slotSaveAs()));
    QShortcut *ctrlO = new QShortcut(QKeySequence::Open, this);
    connect(ctrlO, SIGNAL(activated()), this, SLOT(slotOpen()));

    // On init les paths de mingw
    this->lastFilePath = "";
    this->mingwpath = QApplication::applicationDirPath() + "/MinGW/bin/";
    this->opath = QDir::tempPath() + "/nf04/";

    // Accepte le drop de fichiers
    setAcceptDrops(true);
    ui->statusBar->showMessage(tr("Prêt."));
}

/**
 * @brief MainWindow::~MainWindow
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief MainWindow::start
 */
void MainWindow::start()
{
    slotSave();

    // A décommenter si on accepte qu'un fichier soit lancé sans être enregistré.
    if (this->lastFilePath.isEmpty()) {
        //return;
    }

    // On compile le NF04 en C++
    Converter *converter = new Converter(textEdit->toPlainText(), this);
    QString ccode = converter->toCPP(false);

    if (ccode == "error") {
        return;
    }

    ui->statusBar->showMessage(tr("Sauvegarde temporaire..."));
    QDir(this->opath).mkdir(".");

#ifdef Q_OS_WIN

    QFile::remove(this->opath + "algo.log");
    QFile::remove(this->opath + "algo.exe");

    // On lance le .bat
    QString command = "\"" + this->mingwpath + "start.bat" + "\"";
    QString args =  "\"" + this->mingwpath + "\" \"" + opath + "algo.cpp" + "\" \"" + opath + "algo.exe" + "\" \"" + opath + "algo.log" + "\" \"" + opath + "\"";
    QFile outf(this->opath + "algo.cpp");
    outf.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&outf);
    out.setCodec("UTF-8");
    out << ccode;
    outf.close();

    if (!QDir(this->mingwpath).exists() || !QFile::exists(this->mingwpath + "start.bat")) {
        ui->statusBar->showMessage(tr("Impossible de localiser le compilateur"));
        return;
    }

#else
    QString compiler = qgetenv("NF04-CPP");
    if (compiler.isEmpty()) {
        ui->statusBar->showMessage("Utilisation du compilateur par défaut...");
        compiler = "clang++";
    }
#endif

    ui->statusBar->showMessage(tr("Lancement..."));

#ifdef Q_OS_WIN
    // Seul moyen d'éxecuter un .bat comme s'il était double cliqué (sinon qt change tout ce qui est environnement, etc. et la compilation fail)
    SHELLEXECUTEINFO ShExecInfo;
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = 0x00000000;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = command.toStdWString().c_str();
    ShExecInfo.lpParameters = args.toStdWString().c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ShellExecuteEx(&ShExecInfo);
    WaitForSingleObject(ShExecInfo.hProcess, 0);
#else

    this->opath = "/tmp/";

    QString cppPath = this->opath + "algo.cpp";
    QString bashPath = this->opath + "algo.sh";

    QFile::remove(this->opath + "algo.log");
    QFile::remove(this->opath + "algo.exe");

    /* TODO : Écriture du code CPP dans une fonction */
    QFile outf(cppPath);
    outf.open(QIODevice::WriteOnly | QIODevice::Text);
    if (outf.isOpen() && outf.isWritable()) {
        QTextStream out(&outf);
        out.setCodec("UTF-8");
        out << ccode;
        out.flush();
        outf.close();
    }

    QFile bash(bashPath);
    bash.open(QIODevice::WriteOnly | QIODevice::Text);
    if (bash.isOpen() && bash.isWritable()) {
        QTextStream outBash(&bash);
        outBash.setCodec("UTF-8");
        outBash << qApp->applicationDirPath() << "/../../../clang+llvm-3.4/bin/" << compiler << " " << cppPath << " -o " << this->opath << "algo.exe " <<" 2>> " << this->opath << "algo.log";
        outBash.flush();
        bash.close();
    }

    QStringList compilerArgs;
    compilerArgs << bashPath;
    QProcess::startDetached("/bin/bash", compilerArgs);

    sleep(1);

    if (QFile::exists(this->opath + "algo.exe"))
    {
        QStringList args;
        args << "-a" << "/Applications/Utilities/Terminal.app" << " ../../../../../../../../../tmp/algo.exe";
        QProcess::startDetached("/usr/bin/open", args);
    }

#endif

    readLog();
    ui->statusBar->showMessage(tr("Prêt."));
}

/**
 * Sauvegarde du code dans un fichier.
 *
 * @brief MainWindow::slotSave
 */
void MainWindow::slotSave()
{
    // Si le chemin vers le fichier n'est pas encore définir, on propose à l'utilisateur de le choisir.
    if (this->lastFilePath.isEmpty()) {
        slotSaveAs();
    } else {
        // Sinon, on peut procéder à l'écriture de la source dans le fichier.
        QFile outf(lastFilePath);
        outf.open(QIODevice::WriteOnly | QIODevice::Text);
        if (outf.isOpen() && outf.isWritable()) {
            QTextStream out(&outf);
            out.setCodec("UTF-8");
            out << textEdit->toPlainText();
            ui->statusBar->showMessage(tr("Fichier sauvegardé"), 1500);
            outf.close();
        }
        else {
            ui->statusBar->showMessage(tr("Impossible d'écrire dans le fichier"));
        }
    }
}

/**
 * Choix du fichier où sauvegarder le code, puis appel à this->slotSave();
 *
 * @brief MainWindow::slotSaveAs
 */
void MainWindow::slotSaveAs()
{
    this->lastFilePath = QFileDialog::getSaveFileName(this, tr("Sauvegarder un fichier nf04"),
                                                      this->lastFilePath,
                                                      tr("Fichier NF04 (*.nf04);;Tous les fichiers (*.*)"));
    if ( ! this->lastFilePath.isEmpty()) {
#ifdef Q_OS_WIN
        setWindowTitle("NF04 - " + this->lastFilePath.replace("/", "\\"));
#else
        setWindowTitle("NF04 - " + this->lastFilePath);
#endif
        slotSave();
    }
}

/**
 * Ouverture d'un fichier source.
 *
 * @brief MainWindow::slotOpen
 */
void MainWindow::slotOpen()
{
    if ( ! this->lastFilePath.isEmpty()) {
        slotSave();
    }

    QString fileToOpen = QFileDialog::getOpenFileName(this,
                                                      tr("Ouvrir un fichier nf04"),
                                                      lastFilePath,
                                                      tr("Fichier NF04 (*.nf04);;Tous les fichiers (*.*)"));

    loadFile(fileToOpen);
}

/**
 * Création d'une nouvelle instance du programme lors de l'ouverture d'une source.
 *
 * @brief MainWindow::slotOpenNewWindow
 */
void MainWindow::slotOpenNewWindow()
{
    // On peut lancer le programme détaché avec en paramètre par défaut le path
    QString fileToOpen = QFileDialog::getOpenFileName(this,
                                                      tr("Ouvrir un fichier nf04"),
                                                      this->lastFilePath,
                                                      tr("Fichier NF04 (*.nf04);;Tous les fichiers (*.*)"));

    if ( ! fileToOpen.isEmpty()) {
        QProcess process;
        process.startDetached(qApp->applicationFilePath(), QStringList(fileToOpen));
    }
}

/**
 * Affichage de la fenêtre de syntaxe.
 *
 * @brief MainWindow::slotSyntax
 */
void MainWindow::slotSyntax()
{
    SyntaxWindow *sw = new SyntaxWindow(new QWidget());
    sw->show();
}

/**
 * Affichage de la fenêtre d'« À propos »
 *
 * @brief MainWindow::slotAbout
 */
void MainWindow::slotAbout()
{
    AboutWindow *aw = new AboutWindow(new QWidget());
    aw->show();
}

/**
 * Fermeture de l'application.
 *
 * Le fichier sera automatiquement sauvegardé s'il l'a déjà été, sinon un
 * chemin sera demandé à l'utilisateur.
 *
 * @brief MainWindow::slotQuit
 */
void MainWindow::slotQuit()
{
    if (this->lastFilePath.isEmpty()) {
        slotSaveAs();
    }
    qApp->exit(0);
}

/**
 * @brief MainWindow::slotOpenLog
 */
void MainWindow::slotOpenLog()
{
    QFile *file = new QFile(this->opath + "algo.log");

    if (file->exists()) {
        QDesktopServices::openUrl(QUrl("file:///" + this->opath + "algo.log"));
    } else {
        QMessageBox::warning(this, tr("Ouverture du log C++"), tr("Aucun fichier log trouvé; avez-vous lancé l'algorithme ?"));
    }
}

/**
 * @brief MainWindow::slotOpenCPP
 */
void MainWindow::slotOpenCPP()
{
    QFile *file = new QFile(this->opath + "algo.cpp");

    if (file->exists()) {
        QDesktopServices::openUrl(QUrl("file:///" + this->opath + "algo.cpp"));
    } else {
        QMessageBox::warning(this, tr("Ouverture du code C++"), tr("Aucun fichier C++ trouvé; avez-vous lancé l'algorithme ?"));
    }
}

/**
 * @brief MainWindow::slotNew
 */
void MainWindow::slotNew()
{
    QProcess process;
    process.startDetached(qApp->applicationFilePath());
}

/**
 * @brief MainWindow::modelFromFile
 * @param fileName
 * @return
 */
QStringList MainWindow::modelFromFile(const QString &fileName)
{
    QFile file(fileName);

    if (!file.open(QFile::ReadOnly)) {
        return QStringList();
    }

    QStringList words;

    while (!file.atEnd()) {
        QByteArray line = file.readLine();

        if (!line.isEmpty()) {
            words << line.trimmed();
        }
    }

    return words;
}

/**
 * @brief MainWindow::slotClair
 */
void MainWindow::slotClair()
{
    if (ui->actionClair->isChecked()) {
        ui->actionFonc->setChecked(false);
        updateColor(false);
    } else {
        ui->actionFonc->setChecked(true);
        updateColor(true);
    }
}

/**
 * Gestion du choix du thème.
 *
 * @brief MainWindow::slotFonce
 */
void MainWindow::slotFonce()
{
    if (ui->actionFonc->isChecked()) {
        ui->actionClair->setChecked(false);
        updateColor(true);
    }
    else {
        ui->actionClair->setChecked(true);
        updateColor(false);
    }
}

/**
 * Chargement d'un fichier d'autocompletion.
 *
 * @brief MainWindow::setCompletionFile
 * @param file
 */
void MainWindow::setCompletionFile(QString file) {
    // On prend autocompletion.txt en fallback.
    if ( ! file.isEmpty() || ! QFile::exists(file)) {
            file = QApplication::applicationDirPath() + "/autocompletion.txt";
    }
    if (QFile::exists(file)) {
        completer = new QCompleter(modelFromFile(file), this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        this->textEdit->setCompleter(completer);
    }
    else {
        QMessageBox::information(this, "", tr("Impossible de trouver le fichier d'autocompletion."));
    }
}

/**
 * Mise à jour de la couleur en fonction du thème choisi.
 *
 * @brief MainWindow::updateColor
 * @param dark
 */
void MainWindow::updateColor(bool dark)
{
    QString text = textEdit->toPlainText();
    ui->gridLayout->removeWidget(textEdit);

    textEdit = new CodeEditor(this, dark);
    textEdit->setPlainText(text);
    ui->gridLayout->addWidget(textEdit, 0, 0, 1, 1);

#ifdef Q_OS_WIN
    setCompletionFile(QApplication::applicationDirPath() + "/autocompletion.txt");
#else
    setCompletionFile(QApplication::applicationDirPath() + "/../../../autocompletion.txt");
#endif
    textEdit->setTabStopWidth(28);

    // Chargement et application de la police.
    fdb.addApplicationFont(":/ressources/MONACO.TTF");
#ifdef Q_OS_WIN
    QFont monaco = fdb.font("Monaco", "Normal", 9);
#else
    QFont monaco = fdb.font("Monaco", "Normal", 12);
#endif
    monaco.setLetterSpacing(QFont::AbsoluteSpacing , QVariant(0.75).toReal());
    textEdit->setFont(monaco);

    textEdit->setFocus(Qt::OtherFocusReason);
    SH = new AlgoUttSyntaxHighLighter(textEdit->document(), dark);
}

/**
 * @brief MainWindow::dragEnterEvent
 * @param e
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    e->acceptProposedAction();
}

/**
 * @brief MainWindow::dragMoveEvent
 * @param e
 */
void MainWindow::dragMoveEvent(QDragMoveEvent *e)
{
    e->acceptProposedAction();
}

/**
 * @brief MainWindow::dragLeaveEvent
 * @param e
 */
void MainWindow::dragLeaveEvent(QDragLeaveEvent *e)
{
    e->accept();
}

/**
 * Gestion du « glisser-déposer » de fichiers.
 *
 * @brief MainWindow::dropEvent
 * @param e
 */
void MainWindow::dropEvent(QDropEvent *e)
{
    const QMimeData *mimeData = e->mimeData();

    QList<QUrl> urlList;
    char *url = NULL;
    if (mimeData->hasUrls()) {
        urlList = mimeData->urls();
        // On souhaite enlever la chaîne « file://// » du début;
        // Le cast est présent car c_str() retourne un const char *.
        url = (char *) urlList.at(0).toString().mid(8).toStdString().c_str();
        autoOpen(url);
    }
}

/**
 * Ouverture automatique d'un fichier.
 *
 * Si un nom de fichier a été passé en argument lors du démarrage de
 * l'application, alors on tente de l'ouvrir automatiquement.
 *
 * @brief MainWindow::autoOpen
 * @param path
 */
void MainWindow::autoOpen(char *path)
{
#ifdef Q_OS_WIN
    // On tente d'éviter les problèmes d'encodage liés à Windows - la chaîne
    // passée en argument étant en Latin 1.
    QString pathStr = QString::fromLatin1(path);
#else
    QString pathStr(path);
#endif
    QMessageBox::information(this, "", pathStr);
    loadFile(pathStr);
}

/**
 * Chargement d'un fichier source dans l'éditeur.
 *
 * @brief MainWindow::loadFile
 * @param path
 */
void MainWindow::loadFile(QString path)
{
    QFile inf(path);
    inf.open(QIODevice::ReadOnly | QIODevice::Text);
    if (inf.isOpen() && inf.isReadable()) {
        QTextStream in(&inf);
        in.setCodec("UTF-8");
        textEdit->setPlainText(in.readAll());
        this->lastFilePath = path;
#ifdef Q_OS_WIN
        setWindowTitle("NF04 - " + this->lastFilePath.replace("/", "\\"));
#else
        setWindowTitle("NF04 - " + this->lastFilePath);
#endif
    }
    else {
        ui->statusBar->showMessage(tr("Impossible d'ouvrir le fichier en lecture"));
    }
}

/**
 * Lecture du fichier de logs afin d'identifier les différentes erreurs.
 *
 * Toute erreur de compilation est reconnue grâce à une expression régulière
 * passée sur chaque ligne du fichier de logs. Dans le cas où une erreur
 * est détectée dans le fichier (== une ligne matche), un message indiquant une
 * « Erreur de compilation » est affiché à l'utilisateur.
 *
 * @brief MainWindow::readLog
 */
void MainWindow::readLog()
{
    /*
     * On ouvre le fichier. S'il n'existe pas, on réessaie dans 1 seconde.
     * On a des regexp pour les erreurs.
     */
    QFile *qfile = new QFile(this->opath + "algo.log");

    if ( ! qfile->exists()) {
        QTimer::singleShot(1000, this, SLOT(readLog()));
        return;
    }

    if ( ! qfile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTimer::singleShot(1000, this, SLOT(readLog()));
        return;
    }

    // À la moindre erreur à l'ouverture du fichier, on retente.
    if ( ! qfile->isOpen() || ! qfile->isReadable() || ! qfile->isWritable()) {
        QTimer::singleShot(1000, this, SLOT(readLog()));
        return;
    }

    QTextStream in(&*qfile);
    in.setCodec("UTF-8");

    // Lecture du contenu du fichier de log
    QString log = in.readAll();

    // Le log est vide - pas de traitement nécessaire.
    if (log.isEmpty()) {
        return;
    }

    QString message = "";
    bool addLine = false;
    QRegExp *infunction = new QRegExp("In function");
    QRegExp *notdeclared = new QRegExp("(\\S+) was not declared in this scope");
    QRegExp *invalidCast = new QRegExp("(cannot convert)|(invalid conversion)|(cast from)");
    QRegExp *lineNumber = new QRegExp("\\/\\/(\\d+)");
    QRegExp *invalidArray = new QRegExp("invalid array assignment");
    QRegExp *nonExistentArticle = new QRegExp("error: request for member '(\\S+)' in '(\\S+)'");
    QRegExp *incompatibleAssign = new QRegExp("incompatible types in assignment");

    /*
     * On a deux possibilités :
     *   - Soit on match une erreur : on stocke l'erreur et on indique que la
     *     ligne suivante sera la ligne de code en question.
     *   - Soit on match la ligne de code : on ajoute l'erreur à la ligne et on throw.
     */
    foreach (QString line, log.split("\n")) {
        if (addLine) {
            QStringList lineDetector = Converter::StringMatchesRegexprRes(line, lineNumber);

            if (lineDetector.size() > 0) {
                message += lineDetector[1];
                QMessageBox::critical(this, tr("Erreur de compilation"), message);
            }
            message = "";
            break;
        }

        if (Converter::StringMatchesRegexp(line, infunction)) {
            continue;
        }
        else if (Converter::StringMatchesRegexp(line, notdeclared)) {
            QStringList results = Converter::StringMatchesRegexprRes(line, notdeclared);
            message = tr("Variable inconnue ") + results[1] + tr(" à la ligne ");
            addLine = true;
        }
        else if (Converter::StringMatchesRegexp(line, invalidCast)) {
            message = tr("Conversion de types invalide à la ligne ");
            addLine = true;
        }
        else if (Converter::StringMatchesRegexp(line, invalidArray)) {
            message = tr("Impossible d'affecter au tableau à la ligne ");
            addLine = true;
        }
        else if (Converter::StringMatchesRegexp(line, nonExistentArticle)) {
            QStringList results = Converter::StringMatchesRegexprRes(line, nonExistentArticle);
            message = tr("Accès à un membre innexistant") + " (" + results[1] + ") " + tr("de l'article") +" (" + results[2] + ") " + tr("à la ligne ");
            addLine = true;
        }
        else if (Converter::StringMatchesRegexp(line, incompatibleAssign)) {
            message = tr("Impossible d'assigner cette variable directement à la ligne ");
            addLine = true;
        }
    }
    qfile->close();
}

/**
 * Changement de locale.
 *
 * L'application est redémarrée automatiquement après affichage d'un message
 * d'avertissement et ouverture de la fenetre d'enregistrement si le document
 * ne l'a toujours pas été.
 *
 * @brief MainWindow::changeLang
 * @param locale
 */
void MainWindow::changeLang(QString locale)
{
    QFile outf(QDir::tempPath() + "/nf04/language");
    outf.open(QIODevice::WriteOnly | QIODevice::Text);
    if (outf.isOpen() && outf.isWritable()) {
        QTextStream out(&outf);
        out.setCodec("UTF-8");
        out << locale;
        outf.close();
    }
    else {
        ui->statusBar->showMessage(tr("Impossible de changer de langue"));
    }

    // Si le fichier courant n'a pas été enregistré,
    if (this->lastFilePath.isEmpty()) {
        QMessageBox::information(this, tr("Confirmation avant changment de langage"),
                                 tr("Le changement de language demande un redémarrage de l'application. Pensez à sauvegarder."));
        slotSave();
        slotQuit();
        QProcess process;
        process.startDetached(qApp->applicationFilePath(), QStringList());
    }
}

/**
 * Changement de langue vers EN.
 *
 * @brief MainWindow::slotLngEN
 */
void MainWindow::slotLngEN()
{
    this->changeLang("en");
}

/**
 * Changement de langue vers FR.
 *
 * @brief MainWindow::slotLngFR
 */
void MainWindow::slotLngFR()
{
    this->changeLang("fr");
}

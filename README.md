# NF04 #

## Compilation ##

*Requis : Qt Creator*

- Lancer le projet, définir des options de build (supprimer les environnements par défaut - soit par la box ouverte par Qt Creator, soit en supprimant le fichier .user),
- Tout Recompiler.

## Arborescence ##

- `mainwindow.cpp` : Gestion de tous les événements liés à la fenêtre principale. Gère l'appel au compilateur, la lecture du log, etc.
- `converter.cpp` : Compilation du pseudo-code NF04 en C++.
- `algouttsyntaxhighlighter.cpp`, `codeeditor.cpp`, `linenumberarea.cpp` : gestion de l'éditeur, de l'auto-complétion, de la coloration syntaxique.
- `stable.h` : Headers pré-compilés (tous les #include doivent être dedans).

## Aide ##

[Mail](mailto:gabriel.juchault@utt.fr)

## Licence ##

[Licence BSD](http://opensource.org/licenses/MIT)

"""Franzoesische Texte der Zuse-1-Stufen, je deutscher Text (tools/zuse_stufen.py liest sie ein).
Schreibweise: normale Leerzeichen vor ? ! : und in « »; zuse_stufen.py macht daraus geschuetzte Leerzeichen.
Begriffe: UND = ET, ODER = OU, NICHT = NON, NAND = NON-ET, NOR = NON-OU, XODER = OU exclusif, Uebertrag = retenue (R),
Akku = accumulateur (Acc), Takt = horloge, Loeschen = Effacer, Merken = Mémoriser."""
FR = {
    "": "",
    # Namen der entdeckten Regeln (Werkzeuge)
    "NICHT": "NON",
    "UND": "ET",
    "ODER": "OU",
    "NAND": "NON-ET",
    # Auftakt
    "Licht an!": "Lumière !",
    "Ein Rechner kennt nur zwei Zustände: an und aus – genau wie ein Lichtschalter.":
        "Un ordinateur ne connaît que deux états : allumé et éteint – exactement comme un interrupteur.",
    "Wie sagt man einem Rechner »Ja« oder »Nein«?": "Comment dit-on « oui » ou « non » à un ordinateur ?",
    "Zieh eine Leitung vom Lichtschalter zur Lampe und vom Klingelknopf zur Klingel. Der Kippschalter bleibt stehen, "
    "der Taster nur, solange du drückst.":
        "Tire un fil de l’interrupteur à la lampe et du bouton de sonnette à la sonnette. L’interrupteur reste dans "
        "sa position, le bouton-poussoir seulement tant que tu appuies.",
    "An ist 1, aus ist 0 – das ist ein Bit.": "Allumé, c’est 1, éteint, c’est 0 – c’est un bit.",
    "Lichtschalter aus": "Interrupteur éteint",
    "Lichtschalter an": "Interrupteur allumé",
    "Lichtschalter": "Interrupteur",
    "Klingelknopf losgelassen": "Bouton de sonnette relâché",
    "Klingelknopf gedrückt": "Bouton de sonnette enfoncé",
    "Klingelknopf": "Bouton de sonnette",
    "Lampe aus": "Lampe éteinte",
    "Lampe leuchtet": "Lampe allumée",
    "Lampe": "Lampe",
    "Klingel still": "Sonnette muette",
    "Klingel läutet": "La sonnette sonne",
    "Klingel": "Sonnette",
    # NICHT
    "Das Nachtlicht": "La veilleuse",
    "Lenas Nachtlicht hat einen Lichtsensor. Es soll leuchten, wenn es im Zimmer dunkel ist – und ausgehen, sobald es "
    "hell wird.":
        "La veilleuse de Léna a un capteur de lumière. Elle doit s’allumer quand il fait noir dans la chambre – et "
        "s’éteindre dès qu’il fait jour.",
    "Welche Kiste schaltet das Nachtlicht richtig?": "Quelle boîte commande correctement la veilleuse ?",
    "Stell den Sensor auf hell und auf dunkel. Wann soll das Nachtlicht leuchten?":
        "Règle le capteur sur clair puis sur sombre. Quand la veilleuse doit-elle s’allumer ?",
    "NICHT dreht um: Aus 1 wird 0, aus 0 wird 1.": "NON inverse : 1 devient 0, 0 devient 1.",
    "Im Zimmer ist es dunkel": "Il fait noir dans la chambre",
    "Im Zimmer ist es hell": "Il fait clair dans la chambre",
    "Lichtfühler": "Capteur de lumière",
    "Nachtlicht aus": "Veilleuse éteinte",
    "Nachtlicht leuchtet": "Veilleuse allumée",
    "Nachtlicht": "Veilleuse",
    # UND
    "Der Tresor": "Le coffre-fort",
    "Der Tresor der Bank hat zwei Schlösser. Er darf nur aufgehen, wenn beide Schlüssel gedreht sind – sonst käme ein "
    "Dieb mit einem Schlüssel allein hinein.":
        "Le coffre-fort de la banque a deux serrures. Il ne doit s’ouvrir que si les deux clés sont tournées – sinon "
        "un voleur pourrait entrer avec une seule clé.",
    "Welche Kiste schützt den Tresor?": "Quelle boîte protège le coffre-fort ?",
    "Probier alle vier Möglichkeiten: kein Schlüssel, nur einer, nur der andere, beide.":
        "Essaie les quatre possibilités : aucune clé, une seule, seulement l’autre, les deux.",
    "UND: Beide müssen an sein.": "ET : les deux doivent être allumés.",
    "Schlüssel 1 nicht gedreht": "Clé 1 pas tournée",
    "Schlüssel 1 gedreht": "Clé 1 tournée",
    "Schlüssel 1": "Clé 1",
    "Schlüssel 2 nicht gedreht": "Clé 2 pas tournée",
    "Schlüssel 2 gedreht": "Clé 2 tournée",
    "Schlüssel 2": "Clé 2",
    "Tresor bleibt zu": "Le coffre reste fermé",
    "Tresor geht auf": "Le coffre s’ouvre",
    "Tresor": "Coffre-fort",
    # ODER
    "Die Klingel": "La sonnette",
    "Familie Berger hat zwei Türen, aber nur eine Klingel. Sie soll läuten, wenn vorne an der Haustür oder hinten am "
    "Gartentor jemand drückt.":
        "La famille Berger a deux portes, mais une seule sonnette. Elle doit sonner quand quelqu’un appuie devant, à "
        "la porte d’entrée, ou derrière, au portail du jardin.",
    "Welche Kiste lässt die Klingel richtig läuten?": "Quelle boîte fait sonner correctement la sonnette ?",
    "Halte die Taster gedrückt – einzeln und beide zusammen.":
        "Maintiens les boutons enfoncés – un par un, puis les deux ensemble.",
    "ODER: Eins reicht schon.": "OU : un seul suffit.",
    "An der Haustür drückt niemand": "Personne n’appuie à la porte d’entrée",
    "Jemand drückt an der Haustür": "Quelqu’un appuie à la porte d’entrée",
    "Haustür": "Porte d’entrée",
    "Am Gartentor drückt niemand": "Personne n’appuie au portail",
    "Jemand drückt am Gartentor": "Quelqu’un appuie au portail",
    "Gartentor": "Portail du jardin",
    # NAND
    "Die Fensterwarnung": "L’alerte fenêtre",
    "Im Klassenzimmer hat jedes der beiden Fenster einen Fühler. Im Winter soll keine Wärme verloren gehen: Die "
    "Warnlampe leuchtet, solange noch ein Fenster offen ist. Erst wenn beide Fenster zu sind, geht sie aus.":
        "Dans la salle de classe, chacune des deux fenêtres a un capteur. En hiver, il ne faut pas perdre de chaleur : "
        "le voyant d’alerte s’allume tant qu’une fenêtre est encore ouverte. Il ne s’éteint que lorsque les deux "
        "fenêtres sont fermées.",
    "Welche Kiste warnt richtig?": "Quelle boîte donne la bonne alerte ?",
    "Probier alle vier Stellungen. Wann darf die Warnlampe aus sein?":
        "Essaie les quatre positions. Quand le voyant d’alerte peut-il être éteint ?",
    "NAND heißt NICHT-UND: Nur wenn alle Eingänge 1 sind, ist der Ausgang 0.":
        "NON-ET veut dire : la sortie vaut 0 seulement si toutes les entrées valent 1.",
    "Fenster 1 offen": "Fenêtre 1 ouverte",
    "Fenster 1 zu": "Fenêtre 1 fermée",
    "Fenster 1": "Fenêtre 1",
    "Fenster 2 offen": "Fenêtre 2 ouverte",
    "Fenster 2 zu": "Fenêtre 2 fermée",
    "Fenster 2": "Fenêtre 2",
    "Warnlampe aus": "Voyant éteint",
    "Warnlampe leuchtet": "Voyant allumé",
    "Warnlampe": "Voyant d’alerte",
    # ALARM
    "Alarmanlage": "Alarme",
    "Die Alarmanlage": "L’alarme",
    "Jetzt bist du der Erfinder! Die Sirene soll heulen, wenn die Alarmanlage eingeschaltet ist und ein Fenster oder "
    "die Tür aufgeht. Tagsüber, wenn die Anlage aus ist, bleibt sie still.":
        "Maintenant, c’est toi l’inventeur ! La sirène doit hurler quand l’alarme est en marche et qu’une fenêtre ou "
        "la porte s’ouvre. Dans la journée, quand l’alarme est arrêtée, elle reste muette.",
    "Baue die Alarmanlage aus deinen Werkzeugen.": "Construis l’alarme avec tes outils.",
    "Fasse erst Fenster ODER Tür zusammen – und schicke das dann mit »Anlage an« durch ein UND.":
        "Réunis d’abord fenêtre OU porte – puis fais passer le résultat avec « Alarme en marche » dans un ET.",
    "Werkzeuge lassen sich kombinieren – so entstehen Erfindungen.":
        "On peut combiner les outils – c’est ainsi que naissent les inventions.",
    "Anlage aus": "Alarme arrêtée",
    "Anlage an": "Alarme en marche",
    "Anlage": "Alarme",
    "Fenster zu": "Fenêtre fermée",
    "Fenster offen": "Fenêtre ouverte",
    "Fenster": "Fenêtre",
    "Tür zu": "Porte fermée",
    "Tür offen": "Porte ouverte",
    "Tür": "Porte",
    "Sirene still": "Sirène muette",
    "Sirene heult": "La sirène hurle",
    "Sirene": "Sirène",
    # XODER
    "Treppenlicht-Schaltung": "Va-et-vient",
    "Das Treppenhauslicht": "La lumière de l’escalier",
    "Im Treppenhaus gibt es unten und oben einen Schalter. Mit jedem kann man das Licht an- und ausmachen. Es brennt, "
    "wenn die beiden Schalter verschieden stehen.":
        "Dans l’escalier, il y a un interrupteur en bas et un en haut. Chacun peut allumer et éteindre la lumière. "
        "Elle est allumée quand les deux interrupteurs sont dans des positions différentes.",
    "Wie merkt eine Schaltung, dass zwei Schalter verschieden stehen?":
        "Comment un circuit remarque-t-il que deux interrupteurs sont dans des positions différentes ?",
    "Verschieden heißt: einer ODER der andere – aber NICHT beide (UND).":
        "Différent veut dire : l’un OU l’autre – mais PAS les deux (ET).",
    "XODER: Verschieden ergibt 1.": "OU exclusif : différent donne 1.",
    "Schalter unten aus": "Interrupteur du bas éteint",
    "Schalter unten an": "Interrupteur du bas allumé",
    "Schalter unten": "Interrupteur du bas",
    "Schalter oben aus": "Interrupteur du haut éteint",
    "Schalter oben an": "Interrupteur du haut allumé",
    "Schalter oben": "Interrupteur du haut",
    "Treppenlicht aus": "Lumière éteinte",
    "Treppenlicht brennt": "Lumière allumée",
    "Treppenlicht": "Lumière de l’escalier",
    # NANDUND
    "Alles aus einem Baustein": "Tout avec une seule pièce",
    "Chipfabriken mögen den NAND-Baustein besonders: Aus ihm allein lässt sich jede andere Schaltung bauen. Probier es "
    "aus – baue ein UND nur aus NAND-Bausteinen.":
        "Les usines de puces adorent la pièce NON-ET : avec elle seule, on peut construire n’importe quel autre "
        "circuit. Essaie – construis un ET uniquement avec des pièces NON-ET.",
    "Geht alles mit einem einzigen Baustein?": "Peut-on tout faire avec une seule pièce ?",
    "Verbindest du beide Eingänge eines NAND, dreht er um wie NICHT. Und NICHT hinter NICHT-UND ergibt …?":
        "Si tu relies les deux entrées d’un NON-ET, il inverse comme un NON. Et NON après NON-ET donne … ?",
    "Ein NAND mit verbundenen Eingängen ist ein NICHT. Zwei NAND ergeben ein UND – aus NAND lässt sich alles bauen.":
        "Un NON-ET aux entrées reliées est un NON. Deux NON-ET font un ET – avec des NON-ET, on peut tout construire.",
    "Schalter A aus": "Interrupteur A éteint",
    "Schalter A an": "Interrupteur A allumé",
    "Schalter A": "Interrupteur A",
    "Schalter B aus": "Interrupteur B éteint",
    "Schalter B an": "Interrupteur B allumé",
    "Schalter B": "Interrupteur B",
    # ZAEHLEN
    "Vier Lampen": "Quatre lampes",
    "Mit einer Lampe kann man nur 0 und 1 zeigen. Mit vier Lampen nebeneinander geht viel mehr: Jede Lampe hat einen "
    "Wert – 8, 4, 2 und 1. Die Werte der leuchtenden Lampen werden zusammengezählt. So haben die ersten Heimcomputer "
    "Zahlen gezeigt: Schalter einstellen, Lampen ablesen.":
        "Avec une lampe, on ne peut montrer que 0 et 1. Avec quatre lampes côte à côte, on peut faire bien plus : "
        "chaque lampe a une valeur – 8, 4, 2 et 1. On additionne les valeurs des lampes allumées. C’est ainsi que les "
        "premiers ordinateurs familiaux affichaient les nombres : régler les interrupteurs, lire les lampes.",
    "Wie zählt ein Rechner mit vier Lampen?": "Comment un ordinateur compte-t-il avec quatre lampes ?",
    "Fang bei der größten Lampe an: Passt die 8 hinein? Dann Achter an, den Rest bildest du mit 4, 2 und 1. Beispiel: "
    "5 = 4 + 1.":
        "Commence par la plus grande lampe : est-ce que 8 rentre ? Alors allume le 8 et fais le reste avec 4, 2 et 1. "
        "Exemple : 5 = 4 + 1.",
    "Jede Stelle ist doppelt so viel wert wie die rechts daneben: 8, 4, 2, 1. Mit 4 Bit zählt man von 0 bis 15.":
        "Chaque position vaut le double de celle de droite : 8, 4, 2, 1. Avec 4 bits, on compte de 0 à 15.",
    "Achter": "Huit",
    "Vierer": "Quatre",
    "Zweier": "Deux",
    "Einer": "Un",
    "Lampe 8": "Lampe 8",
    "Lampe 4": "Lampe 4",
    "Lampe 2": "Lampe 2",
    "Lampe 1": "Lampe 1",
    # HA
    "Halbaddierer": "Demi-additionneur",
    "1 + 1 = 10": "1 + 1 = 10",
    "Rechner zählen nur mit 0 und 1. Kommen zwei Einsen zusammen, wird es eng: 1 + 1 ergibt »10« – eine 0 und ein "
    "Übertrag in die nächste Stelle, wie beim Zehnerübergang.":
        "Les ordinateurs ne comptent qu’avec 0 et 1. Quand deux 1 se rencontrent, il n’y a plus de place : 1 + 1 donne "
        "« 10 » – un 0 et une retenue dans la position suivante, comme quand on passe la dizaine.",
    "Baue eine Schaltung, die zwei Bits addiert.": "Construis un circuit qui additionne deux bits.",
    "Die Summe ist 1, wenn die Bits verschieden sind. Der Übertrag ist 1, wenn beide 1 sind.":
        "La somme vaut 1 quand les bits sont différents. La retenue vaut 1 quand les deux valent 1.",
    "Summe und Übertrag – so rechnet ein Computer.": "Somme et retenue – c’est ainsi que calcule un ordinateur.",
    "A ist 0": "A vaut 0",
    "A ist 1": "A vaut 1",
    "Bit A": "Bit A",
    "B ist 0": "B vaut 0",
    "B ist 1": "B vaut 1",
    "Bit B": "Bit B",
    "Summe 0": "Somme 0",
    "Summe 1": "Somme 1",
    "Summe": "Somme",
    "kein Übertrag": "pas de retenue",
    "Übertrag 1": "retenue 1",
    "Übertrag": "Retenue",
    # VA
    "Volladdierer": "Additionneur complet",
    "Drei Bits auf einmal": "Trois bits à la fois",
    "Beim schriftlichen Addieren kommt von rechts manchmal ein Übertrag dazu. Dann müssen drei Bits zusammengezählt "
    "werden: A, B und der Übertrag. Das schafft ein Halbaddierer allein nicht.":
        "Quand on additionne sur papier, une retenue arrive parfois de la droite. Il faut alors additionner trois bits : "
        "A, B et la retenue. Un demi-additionneur seul n’y arrive pas.",
    "Baue eine Schaltung, die drei Bits addiert.": "Construis un circuit qui additionne trois bits.",
    "Addiere erst A und B mit einem Halbaddierer, dann dessen Summe und den Übertrag mit einem zweiten. Liefert einer "
    "der beiden einen Übertrag, gibt es einen Übertrag raus – das ist ein ODER.":
        "Additionne d’abord A et B avec un demi-additionneur, puis sa somme et la retenue avec un deuxième. Si l’un des "
        "deux donne une retenue, il y a une retenue en sortie – c’est un OU.",
    "Ein Volladdierer addiert drei Bits: A, B und den Übertrag von rechts.":
        "Un additionneur complet additionne trois bits : A, B et la retenue venant de la droite.",
    "kein Übertrag rein": "pas de retenue en entrée",
    "Übertrag rein 1": "retenue en entrée 1",
    "Übertrag rein": "Retenue en entrée",
    "kein Übertrag raus": "pas de retenue en sortie",
    "Übertrag raus 1": "retenue en sortie 1",
    "Übertrag raus": "Retenue en sortie",
    # ADD4
    "4-Bit-Addierer": "Additionneur 4 bits",
    "Rechnen bis 15": "Calculer jusqu’à 15",
    "Jetzt rechnest du wie ein echtes Rechenwerk: Mit den oberen vier Schaltern stellst du die Zahl A ein, mit den "
    "unteren die Zahl B. Deine Schaltung zeigt A + B an den Lampen. Und wenn das Ergebnis größer als 15 wird? Dann muss "
    "die Übertrags-Lampe »Ü 16« helfen.":
        "Maintenant, tu calcules comme une vraie unité de calcul : les quatre interrupteurs du haut règlent le nombre A, "
        "ceux du bas le nombre B. Ton circuit affiche A + B sur les lampes. Et si le résultat dépasse 15 ? Alors la "
        "lampe de retenue « R 16 » doit aider.",
    "Was passiert, wenn das Ergebnis nicht mehr in vier Lampen passt?":
        "Que se passe-t-il quand le résultat ne tient plus sur quatre lampes ?",
    "Rechne Stelle für Stelle wie beim schriftlichen Addieren: Für die Einer reicht ein Halbaddierer. Sein Übertrag "
    "geht in den Volladdierer der Zweier, dessen Übertrag zu den Vierern und so weiter. Der letzte Übertrag leuchtet "
    "an »Ü 16«.":
        "Calcule position par position comme sur papier : pour les unités, un demi-additionneur suffit. Sa retenue va "
        "dans l’additionneur complet des 2, la retenue de celui-ci aux 4, et ainsi de suite. La dernière retenue "
        "allume « R 16 ».",
    "Ein 4-Bit-Addierer rechnet bis 15 + 15 = 30. Passt das Ergebnis nicht in vier Bit, zeigt es der Übertrag an – "
    "Rechner nennen das Überlauf.":
        "Un additionneur 4 bits calcule jusqu’à 15 + 15 = 30. Si le résultat ne tient pas sur quatre bits, la retenue "
        "le signale – les ordinateurs appellent cela un dépassement.",
    "A 8": "A 8",
    "A 4": "A 4",
    "A 2": "A 2",
    "A 1": "A 1",
    "B 8": "B 8",
    "B 4": "B 4",
    "B 2": "B 2",
    "B 1": "B 1",
    "Ü 16": "R 16",
    "8": "8",
    "4": "4",
    "3": "3",
    "2": "2",
    "1": "1",
    # MERKER
    "Merker": "Bascule",
    "Hat jemand geklingelt?": "Quelqu’un a-t-il sonné ?",
    "Du warst kurz im Garten. Hat in der Zeit jemand geklingelt? Eine normale Klingellampe leuchtet nur, solange "
    "jemand drückt. Deine Merklampe soll anbleiben, bis du auf »Löschen« drückst.":
        "Tu étais un moment au jardin. Quelqu’un a-t-il sonné entre-temps ? Une lampe de sonnette normale ne s’allume "
        "que tant que quelqu’un appuie. Ta lampe mémoire doit rester allumée jusqu’à ce que tu appuies sur « Effacer ».",
    "Wie kann sich eine Schaltung etwas merken?": "Comment un circuit peut-il se souvenir de quelque chose ?",
    "Die Lampe soll an bleiben, wenn sie schon an ist. Führe ihren Ausgang zurück: Lampe = (Klingel ODER Lampe) UND "
    "NICHT Löschen.":
        "La lampe doit rester allumée si elle l’est déjà. Ramène sa sortie en arrière : lampe = (sonnette OU lampe) ET "
        "NON effacer.",
    "Führt ein Ausgang zurück an den eigenen Eingang, kann sich die Schaltung etwas merken. Das ist ein Speicher für "
    "ein Bit.":
        "Quand une sortie revient sur sa propre entrée, le circuit peut se souvenir de quelque chose. C’est une mémoire "
        "pour un bit.",
    "Löschen": "Effacer",
    "Merklampe": "Lampe mémoire",
    "Einschalten: alles aus": "Mise en marche : tout éteint",
    "Jemand klingelt": "Quelqu’un sonne",
    "Klingelknopf loslassen": "Relâcher le bouton de sonnette",
    "Löschen drücken": "Appuyer sur Effacer",
    "Löschen loslassen": "Relâcher Effacer",
    "Noch einmal klingeln": "Sonner encore une fois",
    "Loslassen": "Relâcher",
    "Klingeln und Löschen zugleich": "Sonner et effacer en même temps",
    # SBIT
    "Speicherbit": "Bit mémoire",
    "Der Schnappschuss": "L’instantané",
    "Ein Rechner muss sich Zahlen merken – aber nur dann, wenn er es soll. Wie bei einem Foto: Erst beim Drücken auf "
    "»Merken« wird festgehalten, wie der Schalter gerade steht. Danach darfst du ihn umlegen, die Lampe bleibt, wie "
    "sie war.":
        "Un ordinateur doit retenir des nombres – mais seulement quand on le lui demande. Comme pour une photo : c’est "
        "seulement en appuyant sur « Mémoriser » que la position de l’interrupteur est capturée. Ensuite, tu peux le "
        "basculer, la lampe reste comme elle était.",
    "Wie merkt sich ein Rechner ein Bit genau dann, wenn er soll?":
        "Comment un ordinateur retient-il un bit exactement au bon moment ?",
    "Nimm deinen Merker. Beim Merken mit Schalter an soll er gesetzt werden (Schalter UND Merken), beim Merken mit "
    "Schalter aus gelöscht (NICHT Schalter UND Merken).":
        "Prends ta bascule. Mémoriser avec l’interrupteur allumé doit la mettre à 1 (interrupteur ET mémoriser), "
        "mémoriser avec l’interrupteur éteint doit l’effacer (NON interrupteur ET mémoriser).",
    "Ein Speicherbit übernimmt den Wert nur beim Merken. Sonst behält es, was es hat.":
        "Un bit mémoire ne prend la valeur qu’au moment de mémoriser. Sinon, il garde ce qu’il a.",
    "Schalter": "Interrupteur",
    "Merken": "Mémoriser",
    "Schalter an – noch nicht gemerkt": "Interrupteur allumé – pas encore mémorisé",
    "Merken drücken": "Appuyer sur Mémoriser",
    "Merken loslassen": "Relâcher Mémoriser",
    "Schalter aus – die Lampe bleibt": "Interrupteur éteint – la lampe reste",
    # TBIT
    "Taktbit": "Bit d’horloge",
    "Genau im Moment des Drückens": "Exactement au moment d’appuyer",
    "Dein Schnappschuss hat einen Haken: Solange »Merken« gedrückt ist, folgt die Lampe dem Schalter. Ein Rechner "
    "braucht Speicher, die nur im Moment des Drückens übernehmen – wie der Auslöser einer Kamera. Und einen Knopf zum "
    "Löschen.":
        "Ton instantané a un défaut : tant que « Mémoriser » est enfoncé, la lampe suit l’interrupteur. Un ordinateur "
        "a besoin de mémoires qui ne prennent la valeur qu’au moment où l’on appuie – comme le déclencheur d’un appareil "
        "photo. Et d’un bouton pour effacer.",
    "Wie übernimmt ein Speicher genau im Moment des Takts?":
        "Comment une mémoire prend-elle la valeur exactement au moment de l’horloge ?",
    "Zuerst ohne Löschen: zwei Speicherbits hintereinander. Das erste merkt, solange der Takt NICHT gedrückt ist, das "
    "zweite beim Drücken. Dann das Löschen: Es öffnet beide Speicherbits (ODER) und gibt beiden eine 0 (UND NICHT "
    "Löschen).":
        "D’abord sans effacer : deux bits mémoire l’un après l’autre. Le premier mémorise tant que l’horloge n’est PAS "
        "enfoncée, le second au moment d’appuyer. Ensuite l’effacement : il ouvre les deux bits mémoire (OU) et leur "
        "donne un 0 (ET NON effacer).",
    "Ein Taktbit übernimmt nur im Moment des Takts. Danach kann sich der Schalter ändern, die Lampe bleibt. Echte "
    "Speicher starten beim Einschalten zufällig – deshalb gibt es »Löschen«.":
        "Un bit d’horloge ne prend la valeur qu’au moment de l’horloge. Ensuite, l’interrupteur peut changer, la lampe "
        "reste. Les vraies mémoires démarrent au hasard à la mise en marche – c’est pourquoi il y a « Effacer ».",
    "Takt": "Horloge",
    "Schalter an – noch kein Takt": "Interrupteur allumé – pas encore d’horloge",
    "Takt drücken: übernommen": "Appuyer sur Horloge : valeur prise",
    "Schalter aus, Takt noch gedrückt – die Lampe bleibt": "Interrupteur éteint, horloge encore enfoncée – la lampe reste",
    "Takt loslassen": "Relâcher Horloge",
    "Schalter an, Takt": "Interrupteur allumé, horloge",
    "Löschen, obwohl der Schalter an ist": "Effacer, alors que l’interrupteur est allumé",
    # REG4
    "Register": "Registre",
    "Vier Bits merken": "Mémoriser quatre bits",
    "Mit vier Taktbits nebeneinander merkt sich ein Rechner eine ganze Zahl von 0 bis 15. So etwas heißt Register. "
    "Alle vier hören auf denselben Takt und denselben Löschknopf.":
        "Avec quatre bits d’horloge côte à côte, un ordinateur retient un nombre entier de 0 à 15. Cela s’appelle un "
        "registre. Les quatre obéissent à la même horloge et au même bouton Effacer.",
    "Wie merkt sich ein Rechner eine ganze Zahl?": "Comment un ordinateur retient-il un nombre entier ?",
    "Jeder Schalter bekommt sein eigenes Taktbit. Takt und Löschen verbindest du mit allen vier.":
        "Chaque interrupteur reçoit son propre bit d’horloge. Relie Horloge et Effacer aux quatre.",
    "Ein Register besteht aus mehreren Taktbits, die gemeinsam übernehmen. Eins mit 4 Bit hält eine Zahl von 0 bis 15.":
        "Un registre est fait de plusieurs bits d’horloge qui prennent la valeur ensemble. Un registre de 4 bits garde "
        "un nombre de 0 à 15.",
    "5 einstellen und merken": "Régler 5 et mémoriser",
    "12 einstellen – es bleibt 5": "Régler 12 – il reste 5",
    "Alle Schalter aus – es bleibt 12": "Tous les interrupteurs éteints – il reste 12",
    "9 einstellen und merken": "Régler 9 et mémoriser",
    # LAUF
    "Schieberegister": "Registre à décalage",
    "Das Lauflicht": "Le chenillard",
    "Bei einem Lauflicht wandert ein Licht von Lampe zu Lampe. Dahinter steckt ein Register, in dem jedes Bit bei "
    "jedem Takt einen Platz weiterrückt – ein Schieberegister.":
        "Dans un chenillard, une lumière se déplace de lampe en lampe. Derrière, il y a un registre dans lequel chaque "
        "bit avance d’une place à chaque coup d’horloge – un registre à décalage.",
    "Wie wandert ein Bit von Lampe zu Lampe?": "Comment un bit se déplace-t-il de lampe en lampe ?",
    "Vier Taktbits in einer Reihe: Das erste bekommt den Eingang, jedes weitere den Ausgang seines Vordermanns. Takt "
    "und Löschen gehen an alle.":
        "Quatre bits d’horloge en rang : le premier reçoit l’entrée, chacun des suivants la sortie de celui qui le "
        "précède. Horloge et Effacer vont à tous.",
    "In einem Schieberegister rückt jedes Bit bei jedem Takt einen Platz weiter. Schiebt man eine Zahl um eine Stelle, "
    "wird sie doppelt oder halb so groß.":
        "Dans un registre à décalage, chaque bit avance d’une place à chaque coup d’horloge. Si on décale un nombre "
        "d’une position, il devient deux fois plus grand ou deux fois plus petit.",
    "Eingang": "Entrée",
    "Eingang an, Takt: das erste Licht geht an": "Entrée allumée, horloge : la première lumière s’allume",
    "Eingang aus, Takt: das Licht wandert": "Entrée éteinte, horloge : la lumière avance",
    "Takt: es fällt hinten heraus": "Horloge : elle sort au bout",
    "Eingang an, Takt": "Entrée allumée, horloge",
    "Takt mit Eingang an": "Horloge avec l’entrée allumée",
    # TFF
    "Umschalter": "Bascule T",
    "Ein Knopf für An und Aus": "Un bouton pour allumer et éteindre",
    "Viele Geräte haben nur einen Knopf: Einmal drücken – an. Noch einmal drücken – aus. Dafür muss sich die Schaltung "
    "merken, wie sie gerade steht, und beim nächsten Takt das Gegenteil nehmen.":
        "Beaucoup d’appareils n’ont qu’un seul bouton : on appuie une fois – allumé. On appuie encore – éteint. Pour "
        "cela, le circuit doit retenir son état et prendre l’inverse au coup d’horloge suivant.",
    "Wie wechselt eine Lampe mit jedem Druck?": "Comment une lampe change-t-elle à chaque pression ?",
    "Ein Taktbit genügt: Gib ihm als Schalter das Gegenteil seiner eigenen Lampe (NICHT). Dann übernimmt es bei jedem "
    "Takt das Gegenteil.":
        "Un bit d’horloge suffit : donne-lui comme interrupteur l’inverse de sa propre lampe (NON). Il prend alors "
        "l’inverse à chaque coup d’horloge.",
    "Ein Taktbit, das bei jedem Takt sein eigenes Gegenteil übernimmt, ist ein Umschalter: Er wechselt bei jedem Druck "
    "genau einmal.":
        "Un bit d’horloge qui prend son propre inverse à chaque coup d’horloge est une bascule T : elle change exactement "
        "une fois à chaque pression.",
    "Knopf": "Bouton",
    "Ganz aus": "Tout éteint",
    "Einschalten: Lampe aus": "Mise en marche : lampe éteinte",
    "Drücken – an": "Appuyer – allumé",
    "Loslassen – bleibt an": "Relâcher – reste allumé",
    "Drücken – aus": "Appuyer – éteint",
    "Loslassen – bleibt aus": "Relâcher – reste éteint",
    # ZAEHLER
    "Zähler": "Compteur",
    "Der Zähler": "Le compteur",
    "Ein Rechner arbeitet im Takt: Bei jedem Takt geschieht ein Schritt. Damit er weiß, wo er gerade ist, zählt er die "
    "Takte mit. Hier bist du der Taktgeber – jeder Druck ist ein Takt.":
        "Un ordinateur travaille au rythme d’une horloge : à chaque coup, une étape se fait. Pour savoir où il en est, "
        "il compte les coups. Ici, c’est toi l’horloge – chaque pression est un coup d’horloge.",
    "Wie zählt ein Rechner mit jedem Takt weiter?": "Comment un ordinateur compte-t-il à chaque coup d’horloge ?",
    "Die Einerstelle wechselt bei jedem Takt. Die nächste Stelle wechselt immer dann, wenn die vorige von 1 auf 0 "
    "fällt – so wie beim Zählen die Zehner weiterspringen. Ein NICHT dazwischen macht aus dem Fallen ein Drücken. "
    "Löschen geht an alle.":
        "La position des unités change à chaque coup. La position suivante change chaque fois que la précédente passe "
        "de 1 à 0 – comme les dizaines qui avancent quand on compte. Un NON entre les deux transforme cette chute en "
        "pression. Effacer va à tous.",
    "Vier Umschalter hintereinander zählen von 0 bis 15. Nach 15 kommt wieder 0 – der Zähler läuft über.":
        "Quatre bascules T à la suite comptent de 0 à 15. Après 15 revient 0 – le compteur déborde.",
    "Takt – Überlauf": "Horloge – dépassement",
    "Takt nach dem Löschen": "Horloge après l’effacement",
    # TAKT
    "Teiler": "Diviseur",
    "Der Taktgeber": "Le générateur d’horloge",
    "Jetzt drückt niemand mehr: Links tickt ein Taktgeber von allein. Für eine Uhr ist er aber viel zu schnell. Bau "
    "einen Teiler: Die zweite Lampe soll halb so schnell blinken, die dritte noch einmal halb so schnell.":
        "Maintenant, plus personne n’appuie : à gauche, un générateur d’horloge bat tout seul. Mais il est bien trop "
        "rapide pour une montre. Construis un diviseur : la deuxième lampe doit clignoter deux fois moins vite, la "
        "troisième encore deux fois moins vite.",
    "Wie wird aus einem schnellen Takt ein langsamer?": "Comment une horloge rapide devient-elle lente ?",
    "Ein Umschalter am Taktgeber wechselt bei jedem Tick – er blinkt also halb so schnell. Ein zweiter Umschalter "
    "dahinter (mit NICHT wie beim Zähler) halbiert noch einmal.":
        "Une bascule T sur le générateur change à chaque tic – elle clignote donc deux fois moins vite. Une deuxième "
        "bascule derrière (avec un NON comme pour le compteur) divise encore par deux.",
    "Jeder Umschalter teilt den Takt durch 2. So entstehen aus einem schnellen Takt Sekunden, Minuten und Stunden.":
        "Chaque bascule T divise l’horloge par 2. C’est ainsi qu’une horloge rapide donne des secondes, des minutes et "
        "des heures.",
    "Taktgeber": "Générateur d’horloge",
    "halb": "moitié",
    "Viertel": "quart",
    "Tick": "Tic",
    "Tick nach dem Löschen": "Tic après l’effacement",
    # GLEICH
    "Vergleicher": "Comparateur",
    "Das Zahlenschloss": "Le cadenas à code",
    "Ein Zahlenschloss geht nur auf, wenn die eingestellte Zahl genau die Geheimzahl ist. Oben stellst du die Zahl A "
    "ein, unten die Geheimzahl B. Die Lampe »offen« soll nur leuchten, wenn beide gleich sind.":
        "Un cadenas à code ne s’ouvre que si le nombre réglé est exactement le code secret. En haut, tu règles le "
        "nombre A, en bas le code secret B. La lampe « ouvert » ne doit s’allumer que si les deux sont égaux.",
    "Woran erkennt ein Rechner, dass zwei Zahlen gleich sind?":
        "Comment un ordinateur reconnaît-il que deux nombres sont égaux ?",
    "Vergleiche Stelle für Stelle: Deine Treppenlicht-Schaltung (XODER) meldet 1, wenn zwei Bits verschieden sind. "
    "Fasse die vier Meldungen mit ODER zusammen – und dreh das Ergebnis mit NICHT um.":
        "Compare position par position : ton va-et-vient (OU exclusif) signale 1 quand deux bits sont différents. "
        "Réunis les quatre signaux avec un OU – et inverse le résultat avec un NON.",
    "Zwei Zahlen sind gleich, wenn sich keine Stelle unterscheidet. XODER findet Unterschiede, NICHT-ODER meldet "
    "»keiner«.":
        "Deux nombres sont égaux quand aucune position ne diffère. OU exclusif trouve les différences, NON-OU signale "
        "« aucune ».",
    "offen": "ouvert",
    # RW
    "Rechenwerk": "Unité de calcul",
    "Plus und Minus": "Plus et moins",
    "Ein Rechner kann eigentlich nur addieren – und zieht trotzdem ab. Der Trick: Alle Bits von B umdrehen und noch 1 "
    "dazuzählen. Mit dem Schalter »Minus« soll dein Rechenwerk wahlweise A + B oder A − B rechnen.":
        "Un ordinateur ne sait en fait qu’additionner – et pourtant il soustrait. L’astuce : inverser tous les bits de "
        "B et ajouter encore 1. Avec l’interrupteur « Moins », ton unité de calcul doit calculer au choix A + B ou "
        "A − B.",
    "Wie zieht ein Rechner ab, wenn er nur addieren kann?":
        "Comment un ordinateur soustrait-il s’il ne sait qu’additionner ?",
    "Nimm vier Volladdierer wie beim Rechnen bis 15. Jedes Bit von B geht durch ein XODER mit »Minus« – das dreht es "
    "um, wenn Minus an ist. Und »Minus« ist zugleich der Übertrag in die Einerstelle: das »+ 1«.":
        "Prends quatre additionneurs complets comme pour calculer jusqu’à 15. Chaque bit de B passe dans un OU exclusif "
        "avec « Moins » – cela l’inverse quand Moins est allumé. Et « Moins » est en même temps la retenue dans la "
        "position des unités : le « + 1 ».",
    "A − B = A + (B umgedreht) + 1. Ist dabei Ü = 1, ging die Rechnung auf; ist Ü = 0, war B größer als A.":
        "A − B = A + (B inversé) + 1. Si R = 1, le calcul est juste ; si R = 0, B était plus grand que A.",
    "Minus": "Moins",
    "Ü": "R",
    # STEUER
    "Du bist das Steuerwerk": "Tu es l’unité de commande",
    "Hier ist eine fertige Rechenmaschine aus deinen Erfindungen: dein Rechenwerk und ein Register, der Akku, der sich "
    "das Ergebnis merkt. Was sie rechnen soll, sagst du ihr Schritt für Schritt: Zahl einstellen, Plus oder Minus "
    "wählen, »Rechnen« drücken. Du bist das Steuerwerk!":
        "Voici une machine à calculer toute prête, faite de tes inventions : ton unité de calcul et un registre, "
        "l’accumulateur, qui retient le résultat. Tu lui dis étape par étape ce qu’elle doit calculer : régler un "
        "nombre, choisir plus ou moins, appuyer sur « Calculer ». C’est toi l’unité de commande !",
    "Wer sagt der Rechenmaschine, was sie tun soll?": "Qui dit à la machine à calculer ce qu’elle doit faire ?",
    "Der Akku zeigt immer das Zwischenergebnis. Jeder Druck auf »Rechnen« rechnet Akku + Zahl (oder Akku − Zahl, wenn "
    "Minus an ist) und merkt sich das Ergebnis.":
        "L’accumulateur montre toujours le résultat intermédiaire. Chaque pression sur « Calculer » calcule "
        "accumulateur + nombre (ou accumulateur − nombre si Moins est allumé) et retient le résultat.",
    "Eine Rechenmaschine braucht jemanden, der ihr die Schritte nacheinander gibt – das Steuerwerk. Konrad Zuses große "
    "Idee: Die Schritte selbst in einem Speicher ablegen, dann rechnet die Maschine von allein. Das ist ein Programm.":
        "Une machine à calculer a besoin de quelqu’un qui lui donne les étapes l’une après l’autre – l’unité de "
        "commande. La grande idée de Konrad Zuse : ranger les étapes elles-mêmes dans une mémoire, et la machine "
        "calcule toute seule. C’est un programme.",
    "Rechnen": "Calculer",
    "Akku 8": "Acc 8",
    "Akku 4": "Acc 4",
    "Akku 2": "Acc 2",
    "Akku 1": "Acc 1",
    "0 + 3: Lade 3 in den Akku": "0 + 3 : charger 3 dans l’accumulateur",
    "3 + 5": "3 + 5",
    "8 − 2": "8 − 2",
    "6 + 7": "6 + 7",
    "Geschafft: 3 + 5 − 2 + 7 = 13! Du hast die Maschine Schritt für Schritt gesteuert. Genau das macht in einem "
    "Rechner das Steuerwerk – von allein, nach einem Programm.":
        "Réussi : 3 + 5 − 2 + 7 = 13 ! Tu as commandé la machine étape par étape. C’est exactement ce que fait l’unité "
        "de commande dans un ordinateur – toute seule, en suivant un programme.",
    # FREI
    "Freies Labor": "Labo libre",
    "Was baust du heute?": "Que construis-tu aujourd’hui ?",
    "Hier gibt es keine Aufgabe. Zieh deine Werkzeuge auf die Werkbank, verbinde sie mit den Schaltern und Lampen und "
    "probier aus, was passiert. Dein Aufbau bleibt gespeichert.":
        "Ici, il n’y a pas d’exercice. Fais glisser tes outils sur l’établi, relie-les aux interrupteurs et aux lampes "
        "et regarde ce qui se passe. Ton montage reste enregistré.",
    "Im Werkzeugfach liegen alle Werkzeuge, die du schon entdeckt oder erfunden hast. Je weiter du im Lernpfad kommst, "
    "desto mehr werden es.":
        "Le bac à outils contient tous les outils que tu as déjà découverts ou inventés. Plus tu avances dans le "
        "parcours, plus il y en a.",
    "Taster": "Bouton-poussoir",
    # Lernstufen
    "Auftakt": "Mise en route",
    "Was versteht ein Rechner überhaupt?": "Que comprend un ordinateur, au fond ?",
    "Logik entdecken": "Découvrir la logique",
    "Wie entscheidet ein Rechner?": "Comment un ordinateur décide-t-il ?",
    "Erfinden mit Bausteinen": "Inventer avec des pièces",
    "Was kann ich aus meinen Bausteinen bauen?": "Que puis-je construire avec mes pièces ?",
    "Zahlen und Rechnen": "Nombres et calcul",
    "Wie rechnet ein Rechner mit 0 und 1?": "Comment un ordinateur calcule-t-il avec 0 et 1 ?",
    "Speichern und Zählen": "Mémoriser et compter",
    "Wie merkt sich ein Rechner etwas – und wie zählt er?":
        "Comment un ordinateur retient-il quelque chose – et comment compte-t-il ?",
    "Die Rechenmaschine": "La machine à calculer",
    "Wie rechnet eine Maschine – und wer sagt ihr, was sie tun soll?":
        "Comment une machine calcule-t-elle – et qui lui dit ce qu’elle doit faire ?",
    "Weiter in Zuse 2: Der Rechner bekommt einen Speicher für sein Programm und steuert sich selbst.":
        "La suite dans Zuse 2 : l’ordinateur reçoit une mémoire pour son programme et se commande lui-même.",
}

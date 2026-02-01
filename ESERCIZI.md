# Esercizi

Lista degli esercizi svolti. Gli esercizi seguono il tutorial in ordine di lettura e sono implementati su un singolo file
per poter essere confrontati con diff più semplicemente.

__Nota__ gli esercizi sono stati svolti in un momento successivo alla creazione del progetto principale e riutilizzano
parti di quel codice per essere più brevi e concisi.

## Esercizio 01

Hello window: una finestra vuota con glfw

## Esercizio 02

Il triangolo del tutorial (con un colore diverso)

## Esercizio 03

Il triangolo dell'esercizio 2 con l'uso di EBO (element buffer object) per specificare i vertici con il loro indice nel VBO

## Esercizio 04

L'esercizio proposto nel sito, ma ho specificato i vertici a mano ed il triangolo non è isoscele, e nemmeno equilatero... ma non erano requisiti espliciti.

## Esercizio 05

L'esercizio proposto nel sito: i due triangoli sono disegnati attraverso due diversi VBO e relativi VAO.

## Esercizio 06

L'esercizio proposto nel sito: i due triangoli sono disegnati come in 05, ma con due programmi diversi per colorarli in modo diverso.

## Esercizio 07

L'esercizio proposto nel sito: un triangolo con colore variabile nel tempo attraverso una uniform variable.

## Esercizio 08

Ogni vertice ha un colore diverso e il fragment interpolation colora il triangolo di conseguenza

## Esercizio 09

L'esercizio proposto nel sito: il triangolo di prima capovolto: inverto il componente y della posizione sullo shader
(l'esercizio prevede di non modificare VBO o VAO)

## Esercizio 10

L'esercizio proposto nel sito: un triangolo translato senza modificarne la geometria, ma usando solo il vertex shader

## Esercizio 11

Questo esercizio proposto prevede di usare le posizioni come dei vertici come colori, quindi nel fragment

```glsl
color = vec4(aPos, 1.0);
```

ho aggiunto inoltre un commento per rispondere alla domanda posta:

```c
// usando i colori come output del fragment shader
// il vertice in basso a sinistra sarà nero
// (-0.5f, -0.5f, 0.0f) perchè i componenti che sono negativi
// verranno clampati a 0 dal fragment shader, l'altro componente
// sarà 0.0f quindi il colore finale sarà (0.0, 0.0, 0.0) ovvero nero
```

## Esercizio 12

Segue il tutorial: carica una texture con stbi_image e usa le coordinate UV per il sampling della texture nel fragment shader.

## Esercizio 13

Segue il tutorial: mix di due texture caricate e usate come nell'esercizio 12. Il questo esempio l'immagine non è caricate "al contrario".

## Esercizio 14

Segue il tutorial: mix di due texture caricate e usate come nell'esercizio 12, ma in questo esempio l'immagine è caricate "al contrario": stbi_image la inverte nell'asse y.

## Esercizio 15

L'esercizio proposto nel sito: applicare la texture della faccia sorridente ruotata rispetto all'asse x (la faccia guarda nella direzione opposta) senza cambiare le coordinate UV nella geometria.

La soluzione è usare la funzione texture passando al primo argomento (U) 1.0 - u

## Esercizio 16

L'esercizio proposto nel sito: disegnare quattro facce con sampling impostato a GL_REPEAT e EDGE_CLAMP per l'altra texture.

## Esercizio 17

L'esercizio proposto nel sito: sampling del pixel centrale delle due texture

## Esercizio 18

L'esercizio proposto nel sito: usare le frecce per cambiare il coefficiente del mix delle due texture.

L'implementazione usa una uniform float.

## Esercizio 19

Segue il tutorial: ruota e trasla il textured quad degli esercizi precedenti

## Esercizio 20

L'esercizio proposto nel sito: applicare prima la traslazione e poi la rotazione.

L'esercizio richiede di spiegare il risultato visuale quindi ho lasciato un commento nel sorgente:

```c
// inverto le operazioni di rotazione e traslazione rispetto all'esercizio 19
// prima avviene la traslazione e poi la rotazione (l'ordine delle moltiplicazioni e' inverso)
// la rotazione avviene intorno all'origine degli assi, quindi l'oggetto ruota intorno a un punto,
// che NON e' più il suo centro, ma il punto che si trova all'origine degli assi
// DOPO la traslazione (che avviene per prima)
```

## Esercizio 21

L'esercizio proposto nel sito: aggiungere un secondo textured quad nell'angolo in alto a sinistra e scalarlo nel tempo.

uso glDrawElements due volte cambiando la matrice di trasformazione tra le due drawcall

## Esercizio 22

Segue il tutorial: si applica la trasformazione desiderata al textured quad usando le matrici M, V e P per un senso di 3D

## Esercizio 23

Segue il tutorial: il cubo texturizzato viene ruotato attraverso una matrice di trasformazione ottenuta usando `sin(t)`.

In questo tutorial viene anche introdotto lo z-buffer, infatti l'esercizio usa `glEnable(GL_DEPTH_TEST)`

## Esercizio 24

Segue il tutorial: disegna diverse istanze del cubo texturizzato con una drawcall per ogni cubo, cambiando la matrice di trasformazione
ad ogni drawcall come descritto nel tutorial.

## Esercizio 25

L'esercizio proposto nel sito: chiede di sperimentare con i parametri FoV e aspect-ratio modificando la matrice di proiezione (P).

Riporto qui il commento di spiegazione:

```c
// Cambiare il field of view non altera la dimensione relativa degli oggetti
// (le deformazioni non alterano la relazione tra gli assi) e i quadrati rimangono quadrati.
// Cambiare il rapporto d'aspetto invece deforma gli oggetti quando l'aspect ratio
// non è quello della finestra di visualizzazione.
// Questo e' dovuto al fatto che l'angolo di visuale verticale mantiene la sua
// proporzione con l'angolo di visuale orizzontale (e tale proporzione e'
// l'aspect ratio).
//
// Cambiando il field of view si ha l'effetto di zoomare avanti o indietro
// sull'intera scena, senza alterarne la prospettiva.
//
// Cambiando l'aspect ratio si ha l'effetto di deformare la scena orizzontalmente
// o verticalmente, come se si schiacciasse o allungasse la lente della camera.
```

## Esercizio 26

L'esercizio proposto nel sito: chiede di sperimentare con la view matrix (V).

Riporto qui il commento di spiegazione:

```c
// La view matrix simula una camera: applica una trasformazione ai vertici della scena:
// la traslazione appare "inversa" rispetto al movimento della camera,
// questo perchè spostare la camera a destra equivale a spostare
// tutta la scena a sinistra e lo stesso vale per le altre direzioni.
```

## Esercizio 27

L'esercizio proposto nel sito: chiede di ruotare ogni terzo cubo incluso il primo.

I cubi vengono disegnati con un for che esegue un drawcall ad ogni esecuzione dopo aver cambiato
la matrice del modello (M): la soluzione è cambiare la matrice se l'indice del for mod 3 è uguale a 0.

## Esercizio 28

Segue il tutorial: implementa una camera 3D attraverso la view matrix (V) usando le API per l'input di glfw.

## Esercizio 29

L'esercizio proposto nel sito: modifica il codice del tutorial per avere una camera ancorata al piano XZ:
l'implementazione calcola il movimento normalmente e rimuove il contributo nell'asse y.

## Esercizio 30

L'esercizio proposto nel sito: reimplementare glm::lookAt

## Esercizio 31

Segue il tutorial: prepara due cubi con solid color per il capito successivo sull'illuminazione

## Esercizio 32

Segue il tutorial: applica illuminazione ambientale, diretta e speculare al cubo più grande
trattando quello più piccolo come una fonte di luce puntiforme

## Esercizio 33

L'esercizio proposto nel sito: muovere la fonte di luce nel tempo usando le funzioni `cos(t)` e `sin(t)`.

Viene applicata una traslazione nella matrice del modello relative alla fonte di luce

## Esercizio 34

L'esercizio proposto nel sito: cambiare i parametri della luce e riportare i risultati:
Shininess factor: più e' piccolo e più l'area speculare e' ampia e meno intensa.
Ambient: non affetta dal colore della sorgente luminosa, più il coefficiente e' piccolo
e meno l'oggetto risalta.
Directional: affetta dal colore della sorgente luminosa, il coefficiente più è grande
e maggiore è il contributo della sorgente luminosa.

## Esercizio 35

L'esercizio proposto nel sito: implementare Phong shading in view space.

Basta moltiplicare i vettori e le posizioni già in worldspace con la view matrix.

Una ottimizzazione possibile è rimuovere la uniform relativa alla posizione dell'osservatore:
è sempre (0,0,0) in viewspace

## Esercizio 36

Come esercizio 35, con applicata l'ottimizzazione riguardo alla posizione della camera in viewspace

## Esercizio 37

L'esercizio proposto nel sito: implementare Gouraud shading.

Basta spostare il calcolo dell'illuminazione nel vertex shader e passare
le tre componenti nel fragment.

Riporto la spiegazione sull'effetto visivo:

```c
// La luce speculare non e' corretta:
// viene calcolata solo ai vertici dei cubi e interpolata sui frammenti,
// creando effetti come "cubo illuminato per metà faccia e metà no",
// inoltre è molto debole a meno che la parte più satura di colore non
// coincida esattamente con un vertice del cubo.
// La luce ambientale è corretta.
// La luce diffusa soffre degli stessi problemi della luce speculare,
// ma è meno evidente perchè la normale varia più dolcemente sui lati del cubo.
```

## Esercizio 38

Segue il tutorial: Parte dell'esercizio 34 e imposta le proprietà del materiale come specifici coefficienti di colori per i differenti tipi di effetti

## Esercizio 39

Segue il tutorial, ma risponde anche al primo quesito del capitolo: la luce varia nel tempo e il colore del cubo emissivo cambia di conseguenza.

Nel precedente capitolo era stato menzionato come il colore del cubo fosse da intendere come il colore della luce e tutti gli esercizi precedenti sono stati svolti con questa relazione resa effettiva tramite una uniform

## Esercizio 40

L'esercizio proposto nel sito: usare un materiale della tabella

## Esercizio 41

Segue il tutorial: applica al cubo (diventato una cassa) una diffuse lightmap e una specular lightmap

## Esercizio 42

L'esercizio proposto nel sito: invertire i colori della specular map.

La soluzione esegue 1.0 - texture(...) per ottenere il valore di specular nel fragment

## Esercizio 43

L'esercizio proposto nel sito: usare una specular lightmap colorata (non in scala di grigi).

Il risultato non è molto realistico e sicuramente non rispetta l'aspettativa de riflesso
speculare su un pezzo di metallo

## Esercizio 44

L'esercizio proposto nel sito: aggiungere una emission map (una texture che rappresenta la luce
emessa dall'oggetto stesso)

## Esercizio 45

Passo intermedio: applica la luce puntiforme (senza attenuazione della distanza) alle diverse casse
posizionate come nei precedenti esercizi

## Esercizio 46

Segue il tutorial: applica una luce direzionale alle diverse casse posizionate come nei precedenti
esercizi

## Esercizio 47

Segue il tutorial: circa uguale all'esercizio esercizio 45, ma pronto per inserire il calcolo della
distanza per implementare l'attenuazione

## Esercizio 48

Segue il tutorial: implementa l'attenuazione di una luce puntiforme basata sulla distanza.

Si può osservare chiaramente come l'attenuazione influenzi la cassa più distante dalla luce,
che nell'esempio precedente risaltava mentre in questo è quasi completamente oscusata

## Esercizio 49

Segue il tutorial: implementa una luce di tipo spotlight "la torcia del giocatore"

Questa prima versione non ha un cutoff graduale

## Esercizio 50

Segue il tutorial: come esercizio 49, ma implementa un alone di dissolvenza graduale attorno alla luce spotlight

## Esercizio 51

L'esercizio proposto nel sito: sperimentazioni varie.

Invertire la direzione della luce (da sola) è equivalente a disabilitare i dermini diffuse e specular: cosO diventa zero

Invertire la direzione della luce e la nomrmale mantiene la componente diffusa come in esercizio 50, percè è come calcolare
l'illuminazione in un differente spazio vettoriale, e sappiamo che il risultato è equivalente, mentre la luce speculare ha
l'intensità massima quando la camera è parallela e minima quando è perpendicolare; questo per come funziona reflect():
__ I - 2.0 * dot(N, I) * N __.

## Esercizio 52

Segue il tutorial: permette di avere diversi punti di luce, spotlight, luci direzionali e una luce ambientale globale.

## Esercizio 53

L'esercizio proposto nel sito: desert

## Esercizio 54

L'esercizio proposto nel sito: biochemical lab

## Esercizio 55

Segue il tutorial: dimostra il problema relativo all'applicare l'illuminazione con componenti in spazi vettoriali diversi,
in questo caso modelspace e tangent space.

Si può osservare come tutte le facce del cubo siano illuminate allo stesso modo nonostante siano orientate in modo differente
tra loro.

## Esercizio 56

Segue il tutorial: partendo da esercizio 55 esegue il calcolo dell'illuminazione in tangentspace così da rendere
tutti i componenti presenti nell'esercizio 55 nello stesso spazio vettoriale.

L'ulluminazione ora risulta corretta e le varie fecce del cubo non sono più illuminate allo stesso modo,
ma seguono l'orientamento della faccia.

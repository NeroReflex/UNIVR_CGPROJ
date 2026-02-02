# Shadow Mapping

Shadow mapping è una tecnica per generare ombre partendo da una sorgente luminosa e la geometria della scena e si compone principalmente di due passi: la generazione della shadowmap ed il suo utilizzo durante l'applicazione della luce sulla scena.

L'algoritmo di shadow mapping deve essere adattato allo specifico tipo di luce che si intende implementare e di seguito descrivo l'implementazione per luci direzionale e spotlight.

La shadowmap è di fatto una texture nella quale ogni pixel descrive la distanza in prospettiva tra un oggetto della scena e la sorgente luminosa: è la risoluzione del problema della visibilità dal punto di vista della luce.

## Generazione della shadowmap

Generare la shadowmap richiede risolvere il problema della visibilità, e in computer graphics esistono principalmente due modi: una rasterization pipeline o una raytracing/ray casting pipeline; l'implementazione qui descritta è la prima creata quindi usando rasterization.

Dal punto di vista delle API grafiche le operazioni necessarie sono creare il framebuffer con un depth attachment e nessun color attachment, impostare le matrici di view e projection per poi eseguire l'operazione di *draw* (nel caso di OpenGL glDraw o glDrawIndexed o equivalenti) su ogni oggetto della scena.

Il motivo dietro al non utilizzo di color attachment risiede nel fatto che la tecnica si interessa solo ed esclusivamente al depth buffer, che deve essere usato per il depth testing durante il fragment test stage: il fragment shader per questi tipi di pipeline non deve eseguire alcuna operazione ne emettere output perchè l'output è già stato generato quando il fragment shader viene eseguito.

Le matrici di view e projection dipendono dal tipo di luce in considerazione e dalla risoluzione desiderata: in questo progetto è stata implementata la versione più semplice di shadowmapping, con problematiche descritte in seguito, ma esistono tecniche di shadowmapping come *cascading shadowmapping* che mirano a migliorare la risoluzione in maniera adattiva sulla zona della scena considerata.

L'unico altro dato necessario alla generazione della shadowmap è la model matrix da applicare ai modelli della scena; di fatto la stessa pipeline grafica funziona sia per luci direzionali che per spotlight:

### depth_only.vert

```c
#version 320 es

precision highp float;

layout(location = 0) in vec3 a_Position;

layout(location = 0) uniform mat4 u_LightSpaceMatrix;

void main() {
    gl_Position = u_LightSpaceMatrix * vec4(a_Position, 1.0);
}
```

### depth_only.frag

```c
#version 320 es

precision highp float;

void main() {
    // nothing to be done here except to NOT discard the fragment.
}
```

### Shadowmap per una luce direzionale

![Directional Shadowmap](relazione/dir_shadowmap.png "Shadowmap per una luce direzionale")

In questa immagine possiamo chiaramente osservare la scena nella sua interezza vista dall'alto.

### Shadowmap per una spotlight

![Spotlight Shadowmap](relazione/spotlight_shadowmap.png "Shadowmap per una spotlight")

In questa immagine sono presenti dei pilastri presenti nella scena.

### MVP per luci direzionali

### MVP per luci spotlight

### Modifiche per luci puntiforme

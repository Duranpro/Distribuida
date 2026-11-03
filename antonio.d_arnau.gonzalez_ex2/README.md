# Exercici 2: exclusio mutua distribuida

Implementacio en C per a Linux/POSIX. Dos heavyweight creen tres lightweight cadascun amb `fork` i `execv`. Els vuit executables son programes independents i es comuniquen nomes per sockets TCP a 127.0.0.1.

## Compilacio i execucio

```sh
make
./ProcessA &
pid_a=$!
./ProcessB &
pid_b=$!
```

Executar des d'aquesta carpeta, on es troben els vuit executables. Es pot iniciar B abans que A si tots dos s'inicien dins del marge de connexio de 10 segons. A sempre te el testimoni inicial. L'ordre dels lightweight dins de cada grup depen de les peticions i les seves marques temporals; no es fixa artificialment A1, A2, A3.

Els bucles son infinits. Despres de l'arrencada, es poden aturar els dos heavyweight amb `kill "$pid_a" "$pid_b"`; els lightweight detecten el tancament dels canals. En terminals separats, Ctrl+C a cadascun. Els errors de comunicacio fan acabar els processos; no es recupera el testimoni.

## Fitxers i algoritmes

- `pesant.c`: ProcessA i ProcessB. Qui te el testimoni envia INICI als tres lightweight, espera un FI de cadascun i passa TESTIMONI a l'altre heavyweight.
- `lleuger_a.c`: Lamport amb REQUEST, ACK i RELEASE. La cua te una marca per proces, amb -1 per indicar que no hi ha peticio. La prioritat es el parell (temps, identificador).
- `lleuger_b.c`: Ricart-Agrawala amb REQUEST i ACK. Si no competeix, respon immediatament. Si competeix, cedeix a una peticio amb prioritat anterior; si esta escrivint o te prioritat, ajorna la resposta. En sortir respon les peticions ajornades, sense RELEASE. Un array fix de tres indicadors substitueix la llista de pendents de l'exemple Java.
- `rellotges.c`: rellotge escalar de Lamport i rellotge de dependencies directes. A utilitza el directe per competir i un escalar separat per coordinar-se amb el pesant. B utilitza un escalar per als missatges de coordinacio i de Ricart-Agrawala.
- `xarxa.c`: servidors, connexions i lectura/escriptura completa de missatges TCP.
- `comu.h`: tipus de missatge, rellotges i declaracions comunes.

Cada lightweight imprimeix el seu identificador deu vegades. Despres de cada impressio espera com a minim un segon, inclosa l'ultima. El bloc utilitza `snprintf`, `write` i `sleep(1)`. Durant les deu impressions, els missatges queden pendents als sockets; s'atenen en acabar el bloc.

## Protocol

Ports d'escolta: 8195 per al pesant A, 8196 per al pesant B, 8197 per a A1, 8198 per a A2 i 8199 per a B1. B2 reutilitza el 8196 despres que el pesant B tanqui el seu servidor: els canals entre B es preparen amb el primer INICI. A3 i B3 nomes inicien connexions i no obren servidor. Entre cada parella del mateix grup hi ha un sol canal TCP bidireccional FIFO. L'identificador major connecta amb el menor. IDENTITAT associa cada connexio al proces corresponent; els identificadors interns dels lightweight son 0, 1 i 2. El heavyweight B es presenta a A amb l'identificador 3.

Cada missatge transmet tres enters en ordre de xarxa: tipus, origen i temps. En l'entorn Linux comprovat cada enter ocupa 32 bits. Els bucles de `read` i `write` completen la trama encara que TCP la fragmenti; detecten desconnexions i repeteixen les operacions interrompudes per EINTR.

REQUEST i ACK s'utilitzen en tots dos grups. RELEASE nomes s'utilitza a Lamport. IDENTITAT, INICI, FI i TESTIMONI serveixen per a l'arrencada i la coordinacio.

No hi ha memoria dinamica: els arrays tenen mida fixa de tres processos. Els errors retornen fins a `main`, que tanca els sockets. Els heavyweight acaben i recullen els fills quan retornen per error.

## Requisits i decisions de disseny

L'enunciat exigeix els dos grups de tres processos, els programes independents, els sockets, el testimoni, Lamport als A, Ricart-Agrawala als B, les deu impressions amb pausa i un informe separat.

Els ports, les trames de tres enters, els noms dels missatges de coordinacio, el testimoni inicial a A i els reintents de connexio son decisions del disseny. Els enunciats d'altres practiques de `teoria` no s'han tractat com a requisits d'aquest exercici.

## Referencies de classe

- `../teoria/Forks, exec/forking5.c`: bucle de creacio de fills amb `fork`.
- `../teoria/Forks, exec/forking3.c`: `execv` amb array d'arguments acabat en NULL.
- `../teoria/Forks, exec/forking.c`: us de `_exit` en un fill.
- `../teoria/Sockets/client.c` i `server.c`: creacio de connexions i servidors TCP.
- `../teoria/Sockets/S4_client.c`: trames binaries i conversions htonl/ntohl. Aqui els camps son fixos i no cal un payload dinamic. Es conserven les lectures i escriptures completes, encara que alguns exemples no les implementin.
- `../teoria/Select/S5_sol.c`: bucle de select, macros FD_* i signal(SIGPIPE, SIG_IGN).
- Lecture 2 part 1, diapositiva 16: rellotge escalar de Lamport.
- Lecture 2 part 2, diapositives 17 i 22: rellotge de dependencies directes.
- Lecture 3 part 1, diapositives 11-14 i 23: Lamport i l'apendix LamportMutex.
- Lecture 3 part 2, diapositives 5-6 i 17: Ricart-Agrawala i l'apendix Java RAMutex. S'adapta la logica a arrays fixos en C, mantenint el tractament de la seccio critica i reinicialitzant les confirmacions a cada torn.

Les impressions utilitzen `sleep(1)` i `snprintf` amb `write`, presents als exemples de senyals i threads. `nanosleep` per als reintents de connexio i SO_REUSEADDR son adaptacions propies sense exemple directe als fonts C aportats.

## Verificacio

Les proves seguents corresponen a la versio anterior als canvis de `sleep`/`write` i als ports 8195-8199. Aquests ultims canvis no s'han testejat, per indicacio de l'usuari.

Verificacio anterior feta a Ubuntu sobre WSL2 amb GCC 15.2.0:

- Els vuit executables han compilat amb el Makefile, tots els seus avisos i `-Werror`, sense adaptacions de les crides POSIX.
- Han passat les proves dels rellotges, confirmacions i desempats de Lamport.
- Han passat la recepcio byte a byte, la desconnexio a mitja trama i les conversions d'ordre de xarxa sobre el codi C real.
- Amb A iniciat primer, s'han comprovat 121 linies: dos cicles A/B complets i l'inici del seguent torn.
- Amb B iniciat primer, s'han comprovat 61 linies: un cicle A/B complet i l'inici del seguent torn.
- En tots dos casos s'han comprovat blocs de deu linies sense intercalacions, les pauses mesurades en emetre les linies, l'alternanca dels grups i la recollida dels sis fills en provocar una desconnexio al grup actiu.

Els guions reproduibles son a `../proves`. Des de l'arrel del repositori, executar `make -C antonio.d_arnau.gonzalez_ex2` i `make -C proves test`. La instrumentacio de temps nomes s'utilitza a les proves i no modifica els fonts ni els executables de l'aplicacio.

## Limitacions acceptades

No es recuperen processos caiguts ni testimonis perduts. Si falla un fill del grup inactiu, el seu pesant pot detectar-ho quan recuperi el testimoni, perque mentrestant nomes espera l'altre pesant. Si falta un executable o una connexio no arriba durant l'arrencada, un accept pot quedar esperant i cal aturar manualment els processos. Una trama parcial d'un emissor congelat pot bloquejar la lectura. No es gestiona el desbordament dels rellotges en execucions molt llargues. No s'han afegit mecanismes de tolerancia a fallades que l'enunciat no demana.

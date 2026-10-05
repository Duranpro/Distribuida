# Exercici 2: primera fase

Implementacio en C per a Linux/POSIX. Els vuit executables son processos independents; tota la comunicacio de coordinacio passa per sockets TCP a 127.0.0.1. No s'utilitzen memoria compartida ni canonades.

## Compilacio i execucio

```sh
make
./ProcessA &
pid_a=$!
./ProcessB &
pid_b=$!
```

Executar des d'aquesta carpeta: cada heavyweight crea els seus tres lightweight amb `fork` i els substitueix pels programes independents amb `execl`. Tambe es pot iniciar cada heavyweight en un terminal diferent. L'ordre d'inici es indiferent si tots dos s'inicien dins dels 10 segons del marge de connexio.

Els bucles son infinits. Per aturar-los des del mateix terminal, executar `kill "$pid_a" "$pid_b"`; els lightweight detecten el tancament dels canals i acaben. Si s'han iniciat en terminals separats, Ctrl+C a cadascun. Els errors de connexio acaben el proces, sense recuperacio del testimoni ni tolerancia a fallades.

## Abast

- `pesant.c`: ProcessA i ProcessB. A te inicialment l'unic testimoni. Qui el posseeix envia INICI als seus tres lightweight, espera una confirmacio FI de cadascun i passa TESTIMONI a l'altre heavyweight. Els INICI s'envien tots abans de recollir les confirmacions.
- `lleuger_a.c`: ProcessLWA1, ProcessLWA2 i ProcessLWA3 competeixen amb Lamport. Cada un imprimeix el seu identificador deu vegades, amb una espera d'un segon despres de cada impressio, inclosa l'ultima. El bucle amb `select` aten els altres processos durant aquestes esperes.
- `lleuger_b.c`: ProcessLWB1, ProcessLWB2 i ProcessLWB3 reben INICI i responen FI. No imprimeixen ni implementen Ricart & Agrawala. Per tant, aquesta fase no genera els blocs B de l'exemple complet de l'enunciat.
- `rellotges.c`: rellotge escalar de Lamport i rellotge de dependencies directes de tres components. El primer s'utilitza en la coordinacio; el segon en la competicio de Lamport entre A.
- `xarxa.c`: servidors, connexions i lectura/escriptura completa de missatges TCP.

## Protocol i Lamport

Ports 5000/5001 per als heavyweight i 5100/5101/5102 per als canals entre lightweight A. Cada parella d'A te un sol canal TCP bidireccional FIFO. L'identificador major connecta amb el menor. La connexio s'identifica amb IDENTITAT; els identificadors interns dels lightweight son 0, 1 i 2. B es presenta a A amb identificador 3, per permetre qualsevol ordre d'arribada de les connexions.

Cada missatge ocupa tres enters de 32 bits en ordre de xarxa: tipus, origen i temps. REQUEST, ACK i RELEASE son exclusivament el protocol de Lamport; IDENTITAT, INICI, FI i TESTIMONI son coordinacio.

La cua de peticions es representa, com a l'apendix dels apunts, amb un array d'una marca temporal per proces; -1 indica ausencia de peticio. La prioritat es el parell (marca temporal, identificador). Un REQUEST difos conserva la mateixa marca temporal als dos destinataris. En rebre'l s'actualitza el rellotge directe, s'afegeix la peticio i s'envia ACK immediatament, encara que el receptor estigui dins la seccio critica. RELEASE elimina la peticio del remitent.

L'entrada exigeix peticio propia, ACK dels dos companys, cap peticio amb prioritat anterior i marques directes dels companys posteriors al parell propi, com a `okayCS` de l'apendix. En rebre una marca t del proces j: component[j] = max(component[j], t); component[propi] = max(component[propi], t) + 1. No es transmet un vector complet.

No hi ha reserva dinamica de memoria: totes les estructures tenen mida fixa de tres processos. `main` i els identificadors obligatoris de les biblioteques conserven els noms imposats per C/POSIX; els noms propis de variables, funcions i tipus son en catala.

## Referencies aportades

Lecture 2 part 1, diapositiva 16: regles del rellotge Lamport.
Lecture 2 part 2, diapositives 17 i 22: DirectClock i actualitzacio dels components.
Lecture 3 part 1, diapositives 11-14 i 23: REQUEST/ACK/RELEASE, cua i condicions d'entrada.
Tema5.4.Sockets: API de sockets TCP. Captures de l'enunciat: testimoni entre heavyweight, inici/confirmacions i deu impressions amb pausa.

## Verificacio realitzada

Els vuit programes han compilat amb GCC, els avisos del Makefile i `-Werror`, mitjancant una adaptacio temporal de les crides POSIX a Windows que no forma part dels fonts lliurats. Les proves han verificat els rellotges, les condicions d'entrada i els desempats; la recepcio TCP fragmentada byte a byte i la desconnexio a mitja trama; i dos torns complets amb els vuit processos, B iniciat primer, blocs de deu impressions sense intercalacions i pauses d'un segon. Tambe s'ha observat l'inici del tercer torn.

L'entorn no disposa de Linux/WSL: el banc de proves ha llancat els lightweight per separat, substituint nomes la creacio i recollida de fills dels heavyweight. Per tant, l'arrencada real amb `fork`/`execl` i el Makefile POSIX queden pendents de comprovar en Linux.

Refactoritzacio posterior: eliminats tots els `goto`. Les funcions d'execucio d'A i dels heavyweight retornen en cas d'error, i `main` tanca els recursos; B utilitza un indicador enter per acabar el bucle. Verificacio sintactica de les vuit variants amb GCC i `-Werror`, amb capcaleres temporals de compatibilitat Windows. La prova d'integracio anterior no s'ha repetit despres d'aquest canvi de control de flux.

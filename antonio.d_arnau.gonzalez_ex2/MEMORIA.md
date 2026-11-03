# Exercici 2 Exclusió mútua distribuïda

Memòria de Sistemes Distribuïts · Curs 2026-2027

Autors: antonio.d i arnau.gonzalez

## Objectiu i abast

La implementació coordina vuit programes independents en C per a Linux. Els dos processos pesants, ProcessA i ProcessB, creen tres processos lleugers cadascun. Tots comparteixen la pantalla i tota la comunicació de l'aplicació passa per sockets TCP locals.

L'exclusió es resol a dos nivells: un testimoni alterna els grups A i B; dins d'A s'utilitza Lamport i dins de B s'utilitza Ricart-Agrawala. Cada lleuger mostra deu vegades el seu identificador i espera almenys un segon després de cada línia, també després de l'última.

## Organització del codi

| Fitxer | Responsabilitat |
| pesant.c | Creació dels fills i traspàs del testimoni entre grups |
| lleuger_a.c | Secció crítica amb l'algoritme de Lamport |
| lleuger_b.c | Secció crítica amb Ricart-Agrawala |
| rellotges.c | Rellotge escalar i de dependències directes |
| xarxa.c | Connexions TCP i transmissió completa de missatges |
| comu.h i Makefile | Declaracions comunes i vuit executables |

Cada pesant utilitza fork i execv per iniciar els seus tres programes. El procés fill acaba amb _exit(1) si execv falla. Els identificadors de cada lleuger es fixen en compilar; es mostren com a A1, A2, A3, B1, B2 i B3.

## Coordinació entre grups

A comença amb l'únic testimoni. Qui el té envia INICI als tres fills abans d'esperar cap resposta. Després rep un FI de cadascun i envia TESTIMONI a l'altre pesant. Esperar els FI en ordre d'identificador no obliga els lleugers a entrar en aquest ordre.

```
ProcessA                         ProcessB
    |-- INICI a A1, A2, A3          |
    |<- FI de tots tres             |
    |---------- TESTIMONI ---------> |
    |                               |-- INICI a B1, B2, B3
    |                               |<- FI de tots tres
    |<--------- TESTIMONI -----------|
    |-- nou torn d'A                 |
```

<!-- pagina -->

## Rellotges i ordre de les peticions

El rellotge escalar s'incrementa abans d'enviar un esdeveniment del protocol. En rebre una marca t, es calcula max(local, t) + 1. B utilitza aquest rellotge tant per coordinar-se amb el pesant com per competir amb els companys.

A utilitza un rellotge de dependències directes per a Lamport. En rebre una marca t del procés j, actualitza component[j] = max(component[j], t) i component[propi] = max(component[propi], t) + 1. Només es transmet una marca escalar, no un vector complet. El rellotge de coordinació dels A és separat.

La prioritat es compara amb el parell (temps de la petició, identificador). Si els temps coincideixen, té prioritat l'identificador menor. La marca de la petició es conserva mentre s'espera; no se substitueix pel rellotge actual.

## Lamport als processos A

Quan rep INICI, cada A incrementa el rellotge, registra la seva petició i difon REQUEST amb la mateixa marca als dos companys. La cua és un array fix de tres marques; -1 indica absència de petició.

En rebre REQUEST, registra la petició del remitent i respon ACK, encara que estigui escrivint. En rebre RELEASE, elimina la petició corresponent. Pot entrar quan ha demanat accés, té les dues confirmacions i cap petició té prioritat anterior. Es conserva també la comparació de les marques conegudes dels companys de l'apèndix LamportMutex.

En acabar les deu impressions i l'última espera, elimina la petició pròpia, difon RELEASE i notifica FI al pesant.

## Ricart Agrawala als processos B

Cada B registra la marca de la seva petició i envia REQUEST als dos companys. En rebre una petició, respon immediatament si no competeix. Si competeix però encara no escriu, respon si la petició rebuda té prioritat anterior. En els altres casos ajorna la resposta, inclòs quan ja és dins de la secció crítica.

Les respostes ajornades es representen amb tres indicadors, un per procés. Amb una sola petició activa per procés no cal una llista dinàmica. Quan rep els dos ACK, entra a la secció crítica. En sortir, respon les peticions ajornades, buida els indicadors i envia FI al pesant. No envia RELEASE. Les confirmacions es reinicialitzen en cada nova petició.

```
Exemple conceptual amb dues peticions de temps 10
B1 -> B2: REQUEST(10)      B2 -> B1: REQUEST(10)
B2 -> B1: ACK             B1 ajorna la resposta a B2
B1 entra quan te tambe l'ACK de B3
B1 surt -> B2: ACK        B2 entra quan te els dos ACK
```

Aquest exemple il·lustra el desempat; no imposa un ordre fix als torns reals. En tots dos algoritmes, les esperes d'impressió es combinen amb select per continuar atenent els sockets.

<!-- pagina -->

## Sockets i format dels missatges

Els ports 5000 i 5001 corresponen als pesants. Els A escolten als ports 5100, 5101 i 5102; els B als ports 5200, 5201 i 5202. Entre cada parella de lleugers del mateix grup hi ha un únic socket bidireccional: el de més identificador inicia la connexió amb el menor.

IDENTITAT permet associar el socket al seu procés sense dependre de l'ordre d'arribada. Els identificadors interns dels lleugers són 0, 1 i 2. B es presenta al pesant A amb l'identificador 3.

Cada trama té tres enters en ordre de xarxa: tipus, origen i temps. Els tipus són IDENTITAT, INICI, FI, TESTIMONI, REQUEST, ACK i RELEASE. En el Linux comprovat cada enter ocupa 32 bits. htonl i ntohl fan les conversions corresponents.

TCP és un flux de bytes. Les funcions enviar i rebre repeteixen write o read fins a completar la trama. No es pressuposa que una sola lectura recuperi un missatge sencer. També detecten el tancament del canal i repeteixen les operacions interrompudes amb EINTR.

## Recursos i decisions de disseny

No es reserva memòria dinàmica: la quantitat de processos és fixa i s'utilitzen arrays de tres elements. Els descriptors s'inicialitzen a -1 i s'assignen a l'estructura de recursos quan s'obren. El socket acceptat temporal es conserva fins a validar la identificació, per poder-lo tancar si es produeix un error.

Els errors retornen fins a main, que tanca els sockets oberts. Si un pesant retorna per error, envia SIGTERM als fills creats i els recull amb waitpid. SIGPIPE s'ignora perquè una escriptura sobre un canal tancat pugui retornar un error.

Els ports, el format de trama, els missatges de coordinació, el testimoni inicial d'A i el marge aproximat de deu segons per connectar són decisions del disseny. L'enunciat exigeix els processos, els dos algoritmes, el testimoni, els sockets i les impressions, però no aquests detalls de protocol.

## Compilació i ús

```
make
./ProcessA &
pid_a=$!
./ProcessB &
pid_b=$!
# Despres de l'arrencada, per aturar-los:
kill "$pid_a" "$pid_b"
```

Cal executar els pesants des de la carpeta on es troben els vuit executables. També es pot iniciar B abans d'A dins del marge de connexió. La sortida alterna trenta línies del grup A i trenta del grup B, repartides en blocs indivisibles de deu. L'ordre dels tres identificadors dins del grup depèn de les peticions.

<!-- pagina -->

## Verificació de la implementació

Els vuit executables s'han compilat en Ubuntu sobre WSL2 amb GCC 15.2.0 i les opcions -std=c99 -Wall -Wextra -Wpedantic -Wdeclaration-after-statement -Werror. No s'han substituït fork, execv ni les crides de sockets per adaptacions de Windows.

Les proves sobre el codi C real han comprovat les actualitzacions dels dos rellotges, la necessitat de confirmacions i els desempats de Lamport. També han passat la recepció d'una trama byte a byte, la detecció d'EOF a mitja trama i les conversions d'ordre de xarxa.

La integració ha passat amb A iniciat primer (121 línies, dos cicles A/B) i amb B primer (61 línies, un cicle). S'han comprovat blocs de deu sense intercalacions, els identificadors, els torns i les pauses. Una biblioteca només de prova registra el moment d'emissió.

En tots dos casos, una desconnexió al grup actiu ha fet acabar els pesants i s'han recollit els sis fills. Els guions reproduïbles són a proves, fora del codi de l'aplicació.

## Limitacions acceptades

La pràctica no implementa recuperació de processos caiguts ni regeneració del testimoni. Una fallada de comunicació fa acabar els processos quan es detecta. Si falla un fill del grup inactiu, el pesant pot detectar-ho al torn següent perquè mentrestant espera el testimoni. Si falta un executable durant l'arrencada, un accept pot quedar esperant i cal aturar manualment el programa.

Si un emissor es congela a mitja trama, la lectura completa pot bloquejar-se. Tampoc es gestiona el desbordament dels rellotges en execucions molt llargues. Són casos excepcionals que es deixen fora d'aquesta implementació.

La temporització utilitza CLOCK_MONOTONIC i select amb sondeig de 100 ms. Això evita que un canvi de l'hora civil afecti les pauses i permet atendre missatges mentre s'escriu. Les pauses poden ser lleugerament superiors a un segon.

## Relació amb els exemples de classe

La creació dels fills segueix forking5.c i l'execució dels programes segueix execv de forking3.c. forking.c mostra _exit en un fill. Els clients i servidors de Sockets aporten el patró de connexió; S4_client.c mostra les conversions de xarxa. S5_sol.c aporta el bucle de select i l'ús de SIGPIPE.

Lamport i el rellotge directe es basen en els apèndixs dels apunts. Ricart-Agrawala adapta la lògica de RAMutex en Java a arrays fixos en C; es conserva l'exclusió durant la secció crítica i es reinicialitzen les confirmacions a cada petició. No es traslladen les classes ni les reserves dinàmiques de Java.

clock_gettime, nanosleep i SO_REUSEADDR són adaptacions pròpies sense exemple directe als fonts C aportats. No s'han substituït els protocols de sockets per semàfors, memòria compartida, pipes o cues de missatges.

## Referències

- 2026 Exercise 2 - DS, enunciat complet, pàgines 1 a 3.
- Lecture 2 part 1, diapositiva 16, rellotge escalar.
- Lecture 2 part 2, diapositives 17 i 22, dependències directes.
- Lecture 3 part 1, diapositives 11 a 14 i 23, Lamport.
- Lecture 3 part 2, diapositives 5, 6 i 17, Ricart-Agrawala.
- Exemples de teoria: Forks, exec; Sockets; Select; Signals.

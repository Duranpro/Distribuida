# Executar l’exercici 3 a Montserrat

## Arrencar les rèpliques i el monitor

Des d’una primera terminal de PowerShell, connecta’t a Montserrat amb un túnel per al monitor. Substitueix `EL_TEU_USUARI` pel teu usuari de Montserrat.

```powershell
ssh -L 8080:127.0.0.1:8080 EL_TEU_USUARI@montserrat.salle.url.edu
```

Un cop dins de Montserrat, substitueix `RUTA_EXERCICI` per la ruta de la carpeta on tens el `Makefile`, compila i arrenca el sistema.

```bash
cd "RUTA_EXERCICI"
make
./iniciar
```

Deixa aquesta terminal oberta. Quan les set rèpliques indiquin que estan preparades i aparegui el missatge del monitor, obre el monitor al navegador del teu ordinador [http://127.0.0.1:8080](http://127.0.0.1:8080).

## Executar el client

Obre una segona terminal de PowerShell i connecta’t a Montserrat.

```powershell
ssh EL_TEU_USUARI@montserrat.salle.url.edu
```

Des de la carpeta de la pràctica, executa el fitxer de transaccions contra A2.

```bash
cd "RUTA_EXERCICI"
./client transaccions.txt A2
```

Amb el sistema acabat d’arrencar, la primera execució mostra que la clau `49` passa de `0` a `53` dins de la transacció d’escriptura a A2. Al monitor pots veure el nucli a la versió 1 mentre B i C encara són a la versió 0. Si executes el client quatre vegades més i esperes que acabi la propagació, les set rèpliques mostraran la versió 5.

Per provar la propagació d’un lot de deu escriptures a partir d’un sistema acabat d’arrencar, executa el fitxer `deu.txt`.

```bash
./client deu.txt A2
```

## Aturar el sistema

Per aturar el programa, prem **Ctrl+C** a la primera terminal, on funciona `./iniciar`.

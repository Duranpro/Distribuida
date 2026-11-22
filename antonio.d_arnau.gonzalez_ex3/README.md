# Executar l’exercici 3 a Montserrat

A la primera terminal PowerShell:

```powershell
ssh -L 8080:127.0.0.1:8080 arnau.gonzalez@montserrat.salle.url.edu
```

Un cop dins de Montserrat:

```bash
cd ~/distr/pr1/antonio.d_arnau.gonzalez_ex3
make
./iniciar
```

Deixa aquesta terminal oberta i obre **http://localhost:8080** al navegador del teu ordinador.

A la segona terminal PowerShell:

```powershell
ssh arnau.gonzalez@montserrat.salle.url.edu
```

Un cop dins de Montserrat:

```bash
cd ~/distr/pr1/antonio.d_arnau.gonzalez_ex3
./client transaccions.txt A2
```

Per executar les altres proves:

```bash
./client prova.txt A3
./client deu.txt A2
```

Els fitxers de prova han d’estar a la carpeta del projecte.

Per aturar el programa, prem **Ctrl+C** a la primera terminal, on funciona `./iniciar`.

# Exercici 2

## Compilar i executar

Obre una terminal Linux a la carpeta de l'exercici i executa:

```bash
make
./ProcessA & ./ProcessB
```

Els dos processos pesants creen els sis lleugers automàticament. A la terminal veuràs les impressions dels dos grups.

## Aturar el programa

Prem **Ctrl+C** a la mateixa terminal per aturar `ProcessB`. Quan torni el prompt, executa `kill %1` per aturar `ProcessA`, que ha quedat en segon pla.

# Pràctica 1: sistemes distribuïts

## Compilació

Des de la carpeta del projecte a matagalls, montserrat o puigpedros:

```sh
make
```

Genera l'executable `distribuida`. Per eliminar l'executable i els fitxers objecte:

```sh
make clean
```

## Execució

Executa cada node en un terminal diferent. Arrenca primer el coordinador.

**Coordinador (ID 0):** indica les seves dades i després les de tots els participants.

```text
./distribuida 0 IP_PROPIA PORT_PROPI MODE ID_1 IP_1 PORT_1 ID_2 IP_2 PORT_2 ...
```

**Participant (ID diferent de 0):** indica les seves dades i després les del coordinador.

```text
./distribuida ID_PROPI IP_PROPIA PORT_PROPI MODE 0 IP_COORDINADOR PORT_COORDINADOR
```

- `MODE`: `READ_ONLY` (lector) o `READ_WRITE` (lector i escriptor).
- Cada node ha de tenir un ID enter no negatiu i una parella IP/port únics.
- Al mateix ordinador, utilitza `127.0.0.1` i ports diferents. Entre ordinadors, utilitza les seves IPv4 accessibles.
- Les dades de cada node han de coincidir entre les comandes. El programa comença quan tots s'han connectat.

### Exemple amb tres nodes al mateix ordinador

Terminal 1 — coordinador lector (arrenca'l primer):

```sh
./distribuida 0 127.0.0.1 8195 READ_ONLY 1 127.0.0.1 8196 2 127.0.0.1 8197
```

Terminal 2 — escriptor:

```sh
./distribuida 1 127.0.0.1 8196 READ_WRITE 0 127.0.0.1 8195
```

Terminal 3 — escriptor:

```sh
./distribuida 2 127.0.0.1 8197 READ_WRITE 0 127.0.0.1 8195
```

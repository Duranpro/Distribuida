"""Regressio del canvi de torns fixos a peticions independents.
Dos participants simulats permeten decidir exactament qui demana permis.
"""
from pathlib import Path
import select
import socket
import struct
import subprocess
import sys
import tempfile
import time
from integracio import ports_lliures


def provar(exe):
    ports = ports_lliures(3)
    sockets, clocks, copies = {}, {1: 0, 2: 0}, {1: 0, 2: 0}
    with tempfile.TemporaryFile() as log:
        args = [str(exe), '0', '127.0.0.1', str(ports[0]), 'READ_ONLY']
        for node in (1, 2):
            args += [str(node), '127.0.0.1', str(ports[node])]
        p = subprocess.Popen(args, stdout=log, stderr=log)
        try:
            def send(node, kind, value=0):
                clocks[node] += 1
                sockets[node].sendall(struct.pack('!III', kind, value, clocks[node]))

            def receive(node):
                data = b''
                while len(data) < 12:
                    block = sockets[node].recv(12-len(data))
                    assert block, 'Connexio tancada inesperadament'
                    data += block
                kind, value, clock = struct.unpack('!III', data)
                clocks[node] = max(clocks[node], clock) + 1
                return kind, value

            def wait(node, expected):
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline:
                    ready, _, _ = select.select(list(sockets.values()), [], [], 1)
                    for s in ready:
                        origin = next(n for n, conn in sockets.items() if conn == s)
                        kind, value = receive(origin)
                        if kind == 6:  # Serveix repliques mentre espera o despres de DONE.
                            copies[origin] = value
                            send(origin, 7)
                        else:
                            assert origin == node and kind == expected, (origin, kind, expected)
                            return value
                raise AssertionError('Timeout esperant resposta')

            for node in (1, 2):
                for _ in range(50):
                    try:
                        sockets[node] = socket.create_connection(('127.0.0.1', ports[0]), timeout=2)
                        break
                    except OSError:
                        time.sleep(.1)
                else:
                    raise AssertionError('Coordinador no disponible')
                send(node, 1, node)
            assert receive(1)[0] == 2
            assert receive(2)[0] == 2

            # El node 2 arriba abans que l'1 tot i figurar despres a la llista.
            send(2, 10, 1)
            assert wait(2, 3) == 1
            send(2, 4)
            assert wait(2, 5) == 0
            # REQUEST del node 1 creua la replicacio del node 2.
            send(1, 10, 1)
            send(2, 6, 1)
            wait(2, 7)
            assert copies[1] == 1
            ready, _, _ = select.select([sockets[1]], [], [], .15)
            assert not ready, 'Segona concessio abans del RELEASE'
            send(2, 8)
            assert wait(1, 3) == 1
            send(1, 4)
            assert wait(1, 5) == 1
            send(1, 6, 2)
            wait(1, 7)
            send(1, 8)

            # L'1 no demana la segona iteracio encara. El 2 pot continuar
            # moltes vegades, sense una barrera artificial entre rondes.
            for iteration in range(2, 11):
                send(2, 10, iteration)
                assert wait(2, 3) == iteration
                send(2, 4)
                value = wait(2, 5)
                assert value == iteration
                send(2, 6, value+1)
                wait(2, 7)
                send(2, 8)
            send(2, 11)
            # Encara que el 2 ha acabat, ha de continuar rebent repliques.
            for iteration in range(2, 11):
                send(1, 10, iteration)
                assert wait(1, 3) == iteration
                send(1, 4)
                value = wait(1, 5)
                assert value == iteration+9
                send(1, 6, value+1)
                wait(1, 7)
                send(1, 8)
            send(1, 11)
            assert copies[2] == 20
            # Cada STOP es confirma abans que el coordinador enviï el seguent.
            wait(1, 9)
            send(1, 7)
            wait(2, 9)
            send(2, 7)
            assert p.wait(timeout=5) == 0
            print('OK ordre 2 abans d\'1, REQUEST durant replicacio, exclusio, iteracions independents i replicas despres de DONE')
        finally:
            for s in sockets.values():
                s.close()
            if p.poll() is None:
                p.kill()
            p.wait()
            if p.returncode:
                log.seek(0)
                print(log.read().decode(errors='replace'))


if __name__ == '__main__':
    provar(Path(sys.argv[1]).resolve())

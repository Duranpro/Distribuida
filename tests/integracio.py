"""Proves reals amb processos i TCP; nomes biblioteca estandard de Python."""
import argparse
import os
from collections import defaultdict
from pathlib import Path
import re
import socket
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def ports_lliures(n):
    sockets = [socket.socket() for _ in range(n)]
    try:
        for s in sockets:
            s.bind(('127.0.0.1', 0))
        return [s.getsockname()[1] for s in sockets]
    finally:
        for s in sockets:
            s.close()


def executar(exe, modes, nom, traces_activades=True):
    ports = ports_lliures(len(modes))
    entorn = os.environ.copy()
    entorn["DISTRIBUIDA_TRACES"] = "1" if traces_activades else "0"
    processos = []
    logs = []
    out = ROOT / 'output' / nom
    out.mkdir(parents=True, exist_ok=True)
    try:
        # Clients primer: comprova que la connexio inicial es reintenta.
        for node in list(range(1, len(modes))) + [0]:
            args = [str(exe), str(node), '127.0.0.1', str(ports[node]), modes[node]]
            for peer in range(len(modes)):
                if peer != node:
                    args += [str(peer), '127.0.0.1', str(ports[peer])]
            f = (out / f'node-{node}.log').open('w')
            logs.append(f)
            processos.append(subprocess.Popen(args, stdout=f, stderr=subprocess.STDOUT, env=entorn))
        for p in processos:
            assert p.wait(timeout=30) == 0, f'{nom}: proces amb error; revisa {out}'
    finally:
        for p in processos:
            if p.poll() is None:
                p.kill()
            p.wait()
        for f in logs:
            f.close()
    esperat = 10 * modes.count('READ_WRITE')
    traces = []
    for node in range(len(modes)):
        text = (out / f'node-{node}.log').read_text()
        assert f'Node {node}: execucio completada.' in text
        assert f'Iteracions: 10/10\nValor final compartit: {esperat}\n' in text
        if not traces_activades:
            assert 'TRACE,' not in text
        lectures = re.findall(r'Iteracio\s+(\d+)/10 \| Llegit:\s+(\d+)', text)
        assert len(lectures) == 10
        assert [int(ronda) for ronda, _ in lectures] == list(range(1, 11))
        clock = 0
        for line in text.splitlines():
            if not line.startswith('TRACE,'):
                continue
            _, nid, ms, lamport, event, peer, tipus, valor = line.split(',')
            lamport = int(lamport)
            assert lamport > clock, 'Lamport local no creix estrictament'
            clock = lamport
            traces.append((int(nid), int(ms), lamport, event, int(peer), tipus, int(valor)))
    if not traces_activades:
        print(f'OK {nom}: sortida llegible, 10 iteracions per node i valor final {esperat}, sense TRACE')
        return
    # Reproduim l'historial serial real, sense pressuposar cap ordre de nodes.
    valor_actual = 0
    for node, ms, clock, event, peer, tipus, valor in sorted(traces, key=lambda x: (x[2], x[0])):
        if event == 'LOCAL' and tipus == 'READ':
            assert valor == valor_actual, 'Lectura desactualitzada'
        if node == 0 and event == 'LOCAL' and tipus == 'UPDATE':
            assert valor == valor_actual + 1, 'Increment perdut'
            valor_actual = valor
    assert valor_actual == esperat
    cua, actiu, concessions = [], None, 0
    for node, ms, clock, event, peer, tipus, valor in traces:
        if node != 0 or event != 'LOCAL':
            continue
        if tipus == 'ENCUAR':
            assert valor not in cua
            cua.append(valor)
        elif tipus == 'CONCEDIR':
            assert actiu is None and cua.pop(0) == valor, 'Violacio de FIFO/exclusio'
            actiu = valor
            concessions += 1
        elif tipus == 'ALLIBERAR':
            assert actiu == valor
            actiu = None
    assert not cua and actiu is None and concessions == 10 * len(modes)
    for node in range(len(modes)):
        alliberat = None
        peticions = 0
        for nid, ms, clock, event, peer, tipus, valor in traces:
            if nid != node:
                continue
            if ((node == 0 and event == 'LOCAL' and tipus == 'ALLIBERAR' and valor == 0) or
                (node != 0 and event == 'SEND' and tipus == 'RELEASE')):
                alliberat = ms
            if tipus == 'REQUEST' and event == ('LOCAL' if node == 0 else 'SEND'):
                peticions += 1
                if alliberat is not None:
                    assert ms - alliberat >= 1000, 'Pausa individual inferior a 1 segon'
        assert peticions == 10
    sends, recvs = defaultdict(list), defaultdict(list)
    for node, ms, clock, event, peer, tipus, valor in traces:
        if event == 'SEND':
            sends[node, peer].append((clock, tipus, valor))
        elif event == 'RECV':
            if peer == -1 and tipus == 'READY':
                peer = valor
            recvs[peer, node].append((clock, tipus, valor))
    assert sends.keys() == recvs.keys()
    for key in sends:
        assert len(sends[key]) == len(recvs[key])
        for sent, recv in zip(sends[key], recvs[key]):
            assert sent[1:] == recv[1:] and sent[0] < recv[0]
    with (out / 'traces.csv').open('w') as f:
        f.write('node,ms,lamport,event,peer,tipus,valor\n')
        for row in sorted(traces, key=lambda x: (x[2], x[0])):
            f.write(','.join(map(str, row)) + '\n')
    print(f'OK {nom}: {len(modes)} nodes, valor final {esperat}, lectures i Lamport correctes')


def invalids(exe):
    cases = [[], ['abc', '127.0.0.1', '5000', 'READ_ONLY'],
             ['0', '127.0.0.1', '70000', 'READ_ONLY'],
             ['0', '999.0.0.1', '5000', 'READ_ONLY'],
             ['0', '127.0.0.1', '5000', 'INVALID'],
             ['1', '127.0.0.1', '5000', 'READ_ONLY'],
             ['0', '127.0.0.1', '5000', 'READ_ONLY', '0', '127.0.0.1', '5001'],
             ['0', '127.0.0.1', '5000', 'READ_ONLY', '1', '127.0.0.1']]
    for args in cases:
        p = subprocess.run([str(exe)] + args, capture_output=True, timeout=3)
        assert p.returncode != 0
    print('OK arguments invalids (8 casos)')


def fragmentacio(exe):
    # Coordinator simulat: entrega cadascuna de les trames byte a byte.
    listener = socket.socket()
    listener.bind(('127.0.0.1', 0))
    listener.listen()
    listener.settimeout(5)
    port = listener.getsockname()[1]
    clock = 0
    with tempfile.TemporaryFile() as log:
        p = subprocess.Popen([str(exe), '1', '127.0.0.1', str(port+1 if port < 65535 else port-1),
                              'READ_WRITE', '0', '127.0.0.1', str(port)],
                             stdout=log, stderr=log)
        try:
            conn, _ = listener.accept()
            with conn:
                conn.settimeout(5)
                def recv(expected):
                    nonlocal clock
                    data = b''
                    while len(data) < 12:
                        part = conn.recv(12-len(data))
                        assert part
                        data += part
                    kind, value, remote = struct.unpack('!III', data)
                    assert kind == expected
                    clock = max(clock, remote) + 1
                    return value
                def send(kind, value=0):
                    nonlocal clock
                    clock += 1
                    for b in struct.pack('!III', kind, value, clock):
                        conn.sendall(bytes([b]))
                        time.sleep(0.001)
                assert recv(1) == 1  # READY
                send(2)            # START
                for i in range(10):
                    assert recv(10) == i+1  # REQUEST independent
                    send(3, i+1)   # GRANT
                    recv(4)        # READ
                    send(5, i)     # VALUE
                    assert recv(6) == i+1  # UPDATE absolut
                    send(7)        # ACK
                    recv(8)        # RELEASE
                recv(11)           # DONE despres de la darrera pausa
                send(9)            # STOP
                recv(7)
            assert p.wait(timeout=5) == 0
        finally:
            if p.poll() is None:
                p.kill()
            p.wait()
            listener.close()
    print('OK trames TCP fragmentades byte a byte')


def desconnexio(exe):
    port, peer_port = ports_lliures(2)
    with tempfile.TemporaryFile() as log:
        p = subprocess.Popen([str(exe), '0', '127.0.0.1', str(port), 'READ_ONLY',
                              '1', '127.0.0.1', str(peer_port)], stdout=log, stderr=log)
        try:
            for _ in range(50):
                try:
                    conn = socket.create_connection(('127.0.0.1', port), timeout=1)
                    break
                except OSError:
                    time.sleep(0.1)
            else:
                raise AssertionError('Coordinador no disponible')
            conn.sendall(struct.pack('!III', 1, 1, 1))
            conn.close()
            assert p.wait(timeout=5) != 0
        finally:
            if p.poll() is None:
                p.kill()
            p.wait()
    print('OK desconnexio detectada sense anunciar exit')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('executable', type=Path)
    args = parser.parse_args()
    exe = args.executable.resolve()
    invalids(exe)
    fragmentacio(exe)
    desconnexio(exe)
    executar(exe, ['READ_WRITE'], 'solitari')
    executar(exe, ['READ_ONLY'] * 3, 'lectors')
    executar(exe, ['READ_ONLY', 'READ_WRITE', 'READ_WRITE'], 'mixt')
    executar(exe, ['READ_WRITE'] * 6, 'escriptors')
    executar(exe, ['READ_ONLY'] * 34, 'memoria_dinamica')
    executar(exe, ['READ_ONLY', 'READ_WRITE', 'READ_WRITE'], 'sortida_llegible', False)

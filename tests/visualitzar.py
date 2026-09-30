"""Genera un SVG de les traces amb nomes la biblioteca estandard."""
import argparse
import csv
from collections import Counter
from html import escape
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('csv', type=Path)
args = parser.parse_args()
with args.csv.open() as f:
    rows = list(csv.DictReader(f))
reads = [r for r in rows if r['event'] == 'LOCAL' and r['tipus'] == 'READ']
if not reads:
    parser.error('No hi ha lectures locals en aquest CSV')
svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="780" viewBox="0 0 1000 780">',
       '<rect width="1000" height="780" fill="white"/>',
       '<g font-family="Arial, sans-serif" fill="#172b4d">']
def text(x, y, value, size=14, anchor='start', color='#172b4d'):
    svg.append(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" fill="{color}">{escape(str(value))}</text>')
def line(x1, y1, x2, y2, color='#d8e0ea'):
    svg.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}"/>')
text(70, 40, 'Valor llegit a cada iteració', 24)
text(70, 65, 'Execució real · ' + args.csv.parent.name)
max_clock = max(int(r['lamport']) for r in reads) or 1
max_value = max(int(r['valor']) for r in reads) or 1
for tick in range(6):
    y = 350 - tick * 48
    line(70, y, 920, y)
    text(55, y + 5, round(max_value * tick / 5, 1), anchor='end')
    x = 70 + tick * 170
    text(x, 378, round(max_clock * tick / 5), anchor='middle')
text(490, 410, 'Rellotge de Lamport (ordre lògic; no mesura durades)', anchor='middle')
colors = ['#2166ac', '#b2182b', '#16865d', '#8055ad', '#a66a00', '#227d8d']
for i, node in enumerate(sorted({int(r['node']) for r in reads})):
    data = [r for r in reads if int(r['node']) == node]
    coords = [(70 + 850 * int(r['lamport']) / max_clock,
               350 - 240 * int(r['valor']) / max_value) for r in data]
    color = colors[i % len(colors)]
    points = ' '.join(f'{x:.1f},{y:.1f}' for x, y in coords)
    svg.append(f'<polyline points="{points}" fill="none" stroke="{color}" stroke-width="2"/>')
    for x, y in coords:
        svg.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4" fill="{color}"/>')
    # Una etiqueta a cada serie evita una llegenda de mida dependent de N.
    x, y = coords[-1]
    text(x + 8, y + 4, node, 12, color=color)
text(70, 95, 'Valor; etiqueta final = ID del node', 12)
text(70, 460, 'Cost de comunicació: trames enviades', 24)
counts = Counter(r['tipus'] for r in rows if r['event'] == 'SEND')
maximum = max(counts.values(), default=1)
width = 850 / max(len(counts), 1)
line(70, 690, 920, 690)
for i, (kind, count) in enumerate(counts.items()):
    height = 170 * count / maximum
    x = 70 + i * width + width * .15
    svg.append(f'<rect x="{x:.1f}" y="{690-height:.1f}" width="{width*.7:.1f}" height="{height:.1f}" fill="#2166ac"/>')
    text(x + width * .35, 680-height, count, anchor='middle')
    text(x + width * .35, 715, kind, 12, anchor='middle')
if not counts:
    text(70, 600, 'Sense trames: execució amb un únic node.')
text(70, 755, 'Cada trama ocupa 12 bytes. UPDATE i ACK mostren el cost de replicació.', 13)
svg.append('</g></svg>')
output = args.csv.with_suffix('.svg')
output.write_text('\n'.join(svg), encoding='utf-8')
print(output)

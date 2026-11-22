const layers = [
  ['Capa 0 · Nucli', ['A1', 'A2', 'A3']],
  ['Capa 1', ['B1', 'B2']],
  ['Capa 2', ['C1', 'C2']]
];
const status = document.querySelector('#status');
const container = document.querySelector('#layers');
function element(tag, text, parent) {
  const node = document.createElement(tag);
  node.textContent = text;
  parent.append(node);
  return node;
}
function connect() {
  const ws = new WebSocket('ws://' + location.host + '/versions');
  ws.onmessage = ({data}) => {
    const state = JSON.parse(data);
    if (state.error) {
      status.textContent = state.error;
      return;
    }
    status.textContent = 'Connectat';
    container.replaceChildren();
    for (const [title, names] of layers) {
      const section = element('section', '', container);
      section.className = 'layer';
      element('h2', title, section);
      const nodes = element('div', '', section);
      nodes.className = 'nodes';
      for (const name of names) {
        const card = element('article', '', nodes);
        element('h3', name, card);
        element('p', 'Versió ' + state[name].version, card);
        const pairs = Object.entries(state[name].data);
        element('pre', pairs.length ? pairs.map(([k, v]) => k + ' → ' + v).join('\n')
                                    : 'Valors inicials: 0', card);
      }
    }
  };
  ws.onclose = () => {
    status.textContent = 'Desconnectat. Reconnectant…';
    setTimeout(connect, 2000);
  };
}
connect();

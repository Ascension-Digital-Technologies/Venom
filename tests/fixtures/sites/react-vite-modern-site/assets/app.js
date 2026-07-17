// @venom: browser
const root = document.querySelector('#root');
let count = 0;
function render(){ root.innerHTML = `<main data-framework="react"><h1>Modern React/Vite pattern</h1><p id="count">${count}</p><button id="increment">Increment</button></main>`; root.querySelector('#increment').addEventListener('click',()=>{count += 1; render();}); }
queueMicrotask(render);

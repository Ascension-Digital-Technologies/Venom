// @venom: browser
const state = new Proxy({message:'ready', count:0},{set(target,key,value){target[key]=value; update(); return true;}});
const app=document.querySelector('#app');
function update(){app.innerHTML=`<main data-framework="vue"><h1>Modern Vue/Vite pattern</h1><p id="message">${state.message}</p><p id="count">${state.count}</p><button id="increment">Increment</button></main>`;app.querySelector('#increment').onclick=()=>state.count++;}
Promise.resolve().then(update);

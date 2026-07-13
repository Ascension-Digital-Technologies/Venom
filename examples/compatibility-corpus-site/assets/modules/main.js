import {sum} from './math.js';import {state} from './state.js';
state.ready=true;globalThis.__corpusModule=sum(10,20,12);globalThis.__corpusState=state.phase+':'+state.ready;
fetch('../data/config.json').then(r=>r.json()).then(data=>{globalThis.__corpusFetch=data.name+':'+data.version;}).catch(error=>{globalThis.__corpusFetch='error:'+error.message;});

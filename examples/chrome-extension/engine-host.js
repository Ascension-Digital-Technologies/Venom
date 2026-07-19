// @venom: browser
'use strict';
const ENGINE_TARGET='velocity-offscreen-engine';
function normalizeError(error){return error instanceof Error?error.message:String(error||'Protected engine failure');}
chrome.runtime.onMessage.addListener((message,_sender,sendResponse)=>{if(!message||message.target!==ENGINE_TARGET)return false;if(message.type==='PING'){sendResponse({ok:true});return false;}if(message.type!=='ANALYZE')return false;(async()=>{const api=globalThis.venom;if(!api||typeof api.ready!=='function'||!api.exports)throw new Error('Venom protected runtime is unavailable in the offscreen engine host');await api.ready();if(typeof api.exports.analyzeVelocity!=='function')throw new Error('Protected export analyzeVelocity is unavailable');return api.exports.analyzeVelocity(message.payload,{timeout:20000});})().then(result=>sendResponse({ok:true,result}),error=>sendResponse({ok:false,error:normalizeError(error)}));return true;});

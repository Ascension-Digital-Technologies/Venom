async function ensure(){
 const url=chrome.runtime.getURL('offscreen.html');
 const contexts=chrome.runtime.getContexts?await chrome.runtime.getContexts({contextTypes:['OFFSCREEN_DOCUMENT'],documentUrls:[url]}):[];
 if(!contexts.length) await chrome.offscreen.createDocument({url:'offscreen.html',reasons:['WORKERS'],justification:'compatibility fixture'});
}
chrome.runtime.onMessage.addListener((m,s,send)=>{if(m&&m.type==='PING'){ensure().then(()=>chrome.runtime.sendMessage({type:'OFFSCREEN_PING'})).then(r=>send(r)).catch(e=>send({ok:false,error:String(e)})); return true;}});
'use strict';
const canvas=document.querySelector('#game'),ctx=canvas.getContext('2d',{alpha:false});
const stage=document.querySelector('#stage'),start=document.querySelector('#start'),saveStatus=document.querySelector('#save-status');
const mobile=()=>matchMedia('(pointer:coarse)').matches||innerWidth<760||(innerHeight<500&&innerWidth<1100);
const languageKey='terminal-velocity.language',pendingLanguageKey='terminal-velocity.pending-language';
let pendingBootLanguage=null;
let preferredLanguage=(navigator.languages?.[0]||navigator.language||'en').toLowerCase().split('-')[0]==='pl'?'pl':'en';
try{const saved=localStorage.getItem(languageKey);if(saved==='pl'||saved==='en')preferredLanguage=saved;const pending=localStorage.getItem(pendingLanguageKey);if(pending==='pl'||pending==='en')preferredLanguage=pendingBootLanguage=pending;}catch{}
let ready=false,started=false,held=0,pressed=0,cols=110,rows=34,previous=new Uint32Array(0),last=0,focused=true,syncing=false,lastUI='',lastBlocked=false,uiFlags=0,english=preferredLanguage==='en',bootStatus='',explicitBootLanguage=pendingBootLanguage!==null;
const tr=(pl,en)=>english?en:pl;
const pointers=new Map(),keyboard=new Set(),tiles=new Map();
let cellPixels=10;
const colour=n=>'#'+n.toString(16).padStart(6,'0');
function tile(code,fg,bg,bold){
 const key=[code,fg,bg,bold].join(':');if(tiles.has(key))return tiles.get(key);
 const c=document.createElement('canvas');c.width=cellPixels;c.height=cellPixels*2;const p=c.getContext('2d');p.scale(cellPixels/10,cellPixels/10);p.fillStyle=colour(bg);p.fillRect(0,0,10,20);p.fillStyle=colour(fg);
 if(code>=0x2581&&code<=0x2588){const h=Math.floor(20*(code-0x2580)/8);p.fillRect(0,20-h,10,h);}
 else if(code===0x2580)p.fillRect(0,0,10,10);
 else if(code>=0x2589&&code<=0x258f)p.fillRect(0,0,Math.floor(10*(0x2590-code)/8),20);
 else if(code===0x2590)p.fillRect(5,0,5,20);
 else if(code===0x2594)p.fillRect(0,0,10,2);
 else if(code===0x2595)p.fillRect(9,0,1,20);
 else if(code>=0x2591&&code<=0x2593){for(let y=0;y<20;y+=2)for(let x=0;x<10;x+=2)if((x/2+y/2*3)%4<code-0x2590)p.fillRect(x,y,1,1);}
 else if(code>=0x2596&&code<=0x259f){const mask=[4,8,1,13,9,7,11,2,6,14][code-0x2596];for(let y=0;y<2;y++)for(let x=0;x<2;x++)if(mask&(1<<(y*2+x)))p.fillRect(x*5,y*10,5,10);}
 else if(code>=0x2800&&code<=0x28ff){const dots=[[1,8],[2,16],[4,32],[64,128]];for(let y=0;y<4;y++)for(let x=0;x<2;x++)if((code-0x2800)&dots[y][x])p.fillRect(x*5+1,y*5+1,2,2);}
 else{p.font=(bold?'bold ':'')+'17px Cell,monospace';p.textBaseline='alphabetic';p.fillText(String.fromCodePoint(code),0,16,10);}
 const cacheLimit=Math.max(512,Math.floor(24*1024*1024/(cellPixels*cellPixels*8)));
 if(tiles.size>cacheLimit)tiles.clear();tiles.set(key,c);return c;
}
window.tvFrame=(data,w,h)=>{
 const scale=Math.min(stage.clientWidth/(w*10),stage.clientHeight/(h*20));canvas.style.width=w*10*scale+'px';canvas.style.height=h*20*scale+'px';
 // Rasterize glyphs for the actual display density instead of enlarging a
 // fixed 10x20 bitmap. Whole-pixel cells keep adjacent block art seamless.
 const density=window.devicePixelRatio||1;
 // Two samples per display pixel soften diagonal glyph edges. Bound the
 // extra canvas memory on large screens, while never going below native DPI.
 const displayArea=Math.max(1,w*10*scale*h*20*scale);
 const samples=Math.max(density,Math.min(density*2,Math.sqrt(12000000/displayArea)));
 const pixels=Math.max(1,Math.ceil(10*scale*samples));
 if(cellPixels!==pixels){cellPixels=pixels;tiles.clear();}
 if(canvas.width!==w*pixels||canvas.height!==h*pixels*2){canvas.width=w*pixels;canvas.height=h*pixels*2;previous=new Uint32Array(0);}
 for(let i=0;i<w*h;i++){const n=i*4;if(previous.length===data.length&&data[n]===previous[n]&&data[n+1]===previous[n+1]&&data[n+2]===previous[n+2]&&data[n+3]===previous[n+3])continue;ctx.drawImage(tile(data[n],data[n+1],data[n+2],data[n+3]),(i%w)*pixels,Math.floor(i/w)*pixels*2);}
 previous=new Uint32Array(data);
};
const tap=(...keys)=>{for(const k of keys)pressed|=1<<k;};
function translateUI(){
 document.documentElement.lang=english?'en':'pl';
 document.title='Terminal Velocity — '+tr('Pod powierzchnią','Below the surface');
 for(const b of document.querySelectorAll('[data-pl]'))b.textContent=tr(b.dataset.pl,b.dataset.en);
 for(const b of document.querySelectorAll('[data-aria-pl]'))b.setAttribute('aria-label',tr(b.dataset.ariaPl,b.dataset.ariaEn));
 for(const b of document.querySelectorAll('[data-language]'))b.setAttribute('aria-pressed',String(b.dataset.language===(english?'en':'pl')));
 document.querySelector('#hint').textContent=tr('A/D ruch · Spacja skok · X/Shift zryw · E użyj · B obejrzyj · P pauza','A/D move · Space jump · X/Shift dash · E use · B inspect · P pause');
 document.querySelector('#touch-use').textContent=lastBlocked?'↵':tr('Użyj','Use');
 if(!started){start.textContent=ready?tr('Uruchom','Start'):tr('Ładowanie…','Loading…');saveStatus.textContent=ready?tr('Gotowe','Ready'):tr('Ładowanie gry…','Loading game…');}
}
function selectLanguage(language){
 english=language==='en';explicitBootLanguage=true;
 try{localStorage.setItem(languageKey,english?'en':'pl');if(!started)localStorage.setItem(pendingLanguageKey,english?'en':'pl');}catch{}
 translateUI();
 if(started){Module._web_set_language(english?1:0);syncSave();canvas.focus();}
 else drawBoot();
}
for(const b of document.querySelectorAll('[data-language]'))b.onclick=()=>selectLanguage(b.dataset.language);
for(const b of document.querySelectorAll('[data-language-toggle]'))b.onclick=()=>selectLanguage(english?'pl':'en');
window.tvUI=u=>{
 const modalFlags=1|2|4|8|64|128;
 // A doorway still reads held movement while its shutter opens.
 if(!!(u.flags&modalFlags)!==!!(uiFlags&modalFlags))clearInput();
 lastBlocked=!!(u.flags&(modalFlags|1024));
 const before=uiFlags;uiFlags=u.flags;
 if((before&(255|1024))!==(uiFlags&(255|1024)))resize();
 const nextEnglish=!!(u.flags&512);
 if(english!==nextEnglish){english=nextEnglish;try{localStorage.setItem(languageKey,english?'en':'pl');}catch{}}
 document.querySelector('#settings-keys').hidden=!(u.flags&16);
 translateUI();
};
const keyMap={ArrowLeft:0,KeyA:0,ArrowRight:1,KeyD:1,ArrowUp:2,KeyW:2,ArrowDown:3,KeyS:3,Space:4,KeyX:5,ShiftLeft:5,ShiftRight:5,KeyE:6,KeyF:6,KeyB:7,Enter:8,Escape:9,KeyP:9,Tab:10,KeyJ:11,KeyM:12,KeyR:13,KeyN:14,KeyL:15,KeyU:16,Digit1:20,Digit2:21,Digit3:22,Digit4:23,Digit5:24,Digit6:25};
function recompute(){
 held=0;
 for(const code of keyboard)held|=1<<keyMap[code];
 for(const {key} of pointers.values())held|=1<<key;
}
addEventListener('keydown',e=>{
 if(e.target instanceof HTMLButtonElement&&['Tab','Enter','Space'].includes(e.code))return;
 if(accounts.active||!started||keyMap[e.code]===undefined||e.ctrlKey||e.metaKey||e.altKey)return;
 e.preventDefault();
 if(!e.repeat)tap(keyMap[e.code]);
 keyboard.add(e.code);
 recompute();
});
addEventListener('keyup',e=>{if(keyboard.delete(e.code))recompute();});
for(const button of document.querySelectorAll('[data-key]')){
 button.addEventListener('pointerdown',e=>{
  if(accounts.active||!started)return;
  e.preventDefault();
  const key=button.id==='touch-use'&&lastBlocked?((uiFlags&64)?6:8):+button.dataset.key;
  button.setPointerCapture(e.pointerId);
  pointers.set(e.pointerId,{key,button});
  button.classList.add('held');tap(key);recompute();
 });
 for(const type of ['pointerup','pointercancel','lostpointercapture'])button.addEventListener(type,e=>{
  pointers.delete(e.pointerId);
  if(![...pointers.values()].some(pointer=>pointer.button===button))button.classList.remove('held');
  recompute();
 });
}
for(const b of document.querySelectorAll('[data-tap]'))b.onclick=()=>{tap(+b.dataset.tap);canvas.focus();};
function clearInput(){keyboard.clear();pointers.clear();held=pressed=0;document.querySelectorAll('.held').forEach(e=>e.classList.remove('held'));}
function syncSave(){if(!started||syncing)return;Module._web_save();syncing=true;Module.FS.syncfs(false,error=>{syncing=false;if(!error)try{localStorage.removeItem(pendingLanguageKey);}catch{}saveStatus.textContent=error?(english?'Save failed — download a copy':'Błąd zapisu — pobierz kopię'):(english?'Saved in this browser ✓':'Zapis w tej przeglądarce ✓');});}
addEventListener('blur',()=>{focused=false;clearInput();syncSave();});addEventListener('focus',()=>{focused=true;last=performance.now();});
document.addEventListener('visibilitychange',()=>{focused=!document.hidden;if(!focused){clearInput();syncSave();}last=performance.now();});
function resize(){
 const compact=mobile()&&!!(uiFlags&(255|1024)),landscape=innerHeight<500&&stage.clientWidth>560;
 cols=compact?(landscape?60:38):Math.max(76,Math.min(160,Math.floor(stage.clientWidth/10)));
 rows=compact?(landscape?20:28):Math.max(24,Math.min(48,Math.floor(stage.clientHeight/(stage.clientWidth/cols*2))));
 previous=new Uint32Array(0);if(!started)drawBoot();
}addEventListener('resize',resize);
function drawBoot(){
 const w=38,h=24,cells=new Uint32Array(w*h*4);for(let i=0;i<w*h;i++){cells[i*4]=32;cells[i*4+1]=0xc8bbb8;cells[i*4+2]=0x101923;}
 const line=(y,text,fg=0xc8bbb8)=>{let x=2;for(const ch of text){if(x>=w-2)break;const i=(y*w+x++)*4;cells[i]=ch.codePointAt(0);cells[i+1]=fg;}};
 line(3,'T E R M I N A L',0xe4ad68);line(5,'V E L O C I T Y',0xe4ad68);
 line(9,tr('POD POWIERZCHNIĄ','BELOW THE SURFACE'));line(12,tr('Miasto wewnątrz góry lodowej.','A city inside an iceberg.'));
 line(16,ready?tr('Dotknij / ENTER — rozpocznij','Tap / ENTER — start'):(bootStatus||tr('Ładowanie gry…','Loading game…')),0x9dd4dd);
 line(19,tr('Klawiatura lub ekran dotykowy.','Keyboard or touch controls.'));line(20,tr('Zapis zostaje w przeglądarce.','Progress stays in this browser.'));
 window.tvFrame(cells,w,h);
}
const playfield=document.querySelector('#playfield');
const expanded=()=>!!document.fullscreenElement||playfield.classList.contains('expanded');
function fullscreenLayout(){clearInput();resize();canvas.focus();document.querySelector('#fullscreen').setAttribute('aria-pressed',String(expanded()));}
async function toggleFullscreen(){
 if(document.fullscreenElement){await document.exitFullscreen();return;}
 if(playfield.classList.contains('expanded')){playfield.classList.remove('expanded');fullscreenLayout();return;}
 try {await playfield.requestFullscreen();}
 catch(error){
  // Embedded browsers and some mobile browsers refuse the native API. The
  // game still occupies the entire viewport, with an explicit way back out.
  console.info('Native fullscreen unavailable; using game viewport',error.name);
  playfield.classList.add('expanded');fullscreenLayout();
 }
}
addEventListener('keydown',e=>{
 if((e.code==='Enter'&&e.altKey)||(e.code==='Escape'&&expanded()&&!accounts.active)){
  e.preventDefault();e.stopImmediatePropagation();toggleFullscreen();
 }
},{capture:true});
document.querySelector('#fullscreen').onclick=toggleFullscreen;
document.querySelector('#leave-fullscreen').onclick=toggleFullscreen;
document.addEventListener('fullscreenchange',fullscreenLayout);
new ResizeObserver(resize).observe(stage);
document.querySelector('#export-save').onclick=()=>{if(!started)return;Module._web_save();try{const bytes=Module.FS.readFile('/persist/progress.dat.iceberg');const url=URL.createObjectURL(new Blob([bytes],{type:'application/octet-stream'}));const a=document.createElement('a');a.href=url;a.download='terminal-velocity.iceberg';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}catch{saveStatus.textContent=tr('Brak zapisu do pobrania','No save to download');}};
function frame(now){if(accounts.active)accounts.draw();else if(started){Module._web_tick(Math.min(.1,Math.max(0,(now-last)/1000)),held,pressed,cols,rows,focused?1:0);pressed=0;}last=now;requestAnimationFrame(frame);}
start.onclick=()=>{
 if(!ready||started)return;
 try {
  Module._web_start(Date.now()>>>0,english?1:0);
  Module._web_enter_campaign();
  started=true;start.hidden=true;syncSave();accounts.start();resize();last=performance.now();canvas.focus();requestAnimationFrame(frame);
 } catch(error) {
  console.error(error);started=false;ready=false;start.disabled=true;
  bootStatus=tr('Błąd uruchamiania — odśwież stronę','Startup failed — reload the page');
  saveStatus.textContent=bootStatus;drawBoot();
 }
};
canvas.onclick=()=>{if(!started)start.click();else if(!accounts.active&&(uiFlags&(2|4|1024)))tap(8);};
addEventListener('keydown',e=>{if(!started&&e.code==='Enter')start.click();});
var Module={onRuntimeInitialized(){try{Module.FS.mkdir('/persist');Module.FS.mount(Module.IDBFS,{},'/persist');Module.FS.syncfs(true,error=>{
 // A valid game save is authoritative even before the player presses Start.
 // An explicit selection made while loading takes precedence over that save.
 if(!explicitBootLanguage){const saved=Module._web_saved_language();if(saved===0||saved===1)english=saved===1;}
 try{localStorage.setItem(languageKey,english?'en':'pl');}catch{}
 ready=true;start.disabled=false;translateUI();
 if(error)saveStatus.textContent=tr('Zapis przeglądarki niedostępny — pobieraj kopię','Browser storage unavailable — download backups');
 drawBoot();document.fonts.load('17px Cell').then(()=>{tiles.clear();previous=new Uint32Array(0);});
 });}catch(e){bootStatus=tr('Błąd uruchamiania: ','Startup failed: ')+String(e);drawBoot();}},setStatus(text){if(!text)bootStatus='';drawBoot();},onAbort(error){bootStatus=tr('Błąd uruchamiania: ','Startup failed: ')+error;drawBoot();}};
const accounts=new TVAccounts({english:()=>english,ready:()=>started,clear:clearInput,frame:window.tvFrame});
document.querySelector('#account').onclick=()=>accounts.open();
document.querySelector('#account-fullscreen').onclick=()=>accounts.open();
setInterval(syncSave,5000);
setInterval(()=>accounts.sync(),15000);

translateUI();resize();

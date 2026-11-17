'use strict';
const canvas=document.querySelector('#game'),ctx=canvas.getContext('2d',{alpha:false});
const stage=document.querySelector('#stage'),start=document.querySelector('#start'),saveStatus=document.querySelector('#save-status');
const mobile=()=>matchMedia('(pointer:coarse)').matches||innerWidth<760||(innerHeight<500&&innerWidth<1100);
const languageKey='terminal-velocity.language',pendingLanguageKey='terminal-velocity.pending-language';
let pendingBootLanguage=null;
let hasStoredLanguage=false;
let preferredLanguage=(navigator.languages?.[0]||navigator.language||'en').toLowerCase().split('-')[0]==='pl'?'pl':'en';
try{const saved=localStorage.getItem(languageKey);if(saved==='pl'||saved==='en'){preferredLanguage=saved;hasStoredLanguage=true;}const pending=localStorage.getItem(pendingLanguageKey);if(pending==='pl'||pending==='en')preferredLanguage=pendingBootLanguage=pending;}catch{}
let ready=false,started=false,held=0,pressed=0,cols=110,rows=34,previous=new Uint32Array(0),last=0,focused=true,syncing=false,lastBlocked=false,uiFlags=0,english=preferredLanguage==='en',bootStatus='',explicitBootLanguage=pendingBootLanguage!==null;
const tr=(pl,en)=>english?en:pl;
async function suggestLanguage(){
 if(hasStoredLanguage||explicitBootLanguage)return null;
 const controller=new AbortController();
 const timeout=setTimeout(()=>controller.abort(),1200);
 try {
  const response=await fetch('language.php',{cache:'no-store',signal:controller.signal});
  const result=response.ok?await response.json():null;
  return ['pl','en'].includes(result?.language)?result.language:null;
 } catch {return null;}
 finally {clearTimeout(timeout);}
}
const locationLanguage=suggestLanguage();
const pointers=new Map(),keyboard=new Set(),tiles=new Map();
let dashShift='';
let frameWasStalled=false;
let storageFlushQueued=false;
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
 const sameGrid=previous.length===data.length;
 for(let i=0;i<w*h;i++){
  const n=i*4;
  if(sameGrid&&data[n]===previous[n]&&data[n+1]===previous[n+1]&&data[n+2]===previous[n+2]&&data[n+3]===previous[n+3])continue;
  ctx.drawImage(tile(data[n],data[n+1],data[n+2],data[n+3]),(i%w)*pixels,Math.floor(i/w)*pixels*2);
 }
 if(!sameGrid)previous=new Uint32Array(data.length);
 previous.set(data);
};
let completedPresses=0;
const tap=(...keys)=>{for(const k of keys){completedPresses|=1<<k;pressed|=1<<k;}};
function pointerPress(pointer){
 pointer.pending|=1<<pointer.key;
 pressed|=1<<pointer.key;
}
function clearPresses(){
 pressed=completedPresses=0;
 for(const pointer of pointers.values())pointer.pending=0;
}
function translateUI(){
 document.documentElement.lang=english?'en':'pl';
 document.title='Terminal Velocity — '+tr('Pod powierzchnią','Below the surface');
 for(const b of document.querySelectorAll('[data-pl]'))b.textContent=tr(b.dataset.pl,b.dataset.en);
 for(const b of document.querySelectorAll('[data-aria-pl]'))b.setAttribute('aria-label',tr(b.dataset.ariaPl,b.dataset.ariaEn));
 for(const b of document.querySelectorAll('[data-language]'))b.setAttribute('aria-pressed',String(b.dataset.language===(english?'en':'pl')));
 document.querySelector('#hint').textContent=tr('A/D ruch · Spacja skok · X lub wciśnij i puść Shift, by wykonać zryw · E użyj · B obejrzyj · P pauza · Tab zadania · Shift+Tab sterowanie przeglądarki','A/D move · Space jump · X or tap Shift to dash · E use · B inspect · P pause · Tab quests · Shift+Tab browser controls');
 const touchUse=document.querySelector('#touch-use');
 touchUse.textContent=lastBlocked?'↵':tr('Użyj','Use');
 touchUse.setAttribute('aria-label',lastBlocked?tr('Zatwierdź','Confirm'):tr('Użyj','Use'));
 const districtControl=!!(uiFlags&16384);
 for(const button of document.querySelectorAll('[data-tap="11"]')){
  button.textContent=districtControl?tr('Dzielnica','District'):tr('Mapa','Map');
  button.setAttribute('aria-label',districtControl?tr('Zmień dzielnicę','Change district'):button.textContent);
 }
 const touchInspect=document.querySelector('#touch-inspect');
 touchInspect.hidden=!!((uiFlags&128)&&(uiFlags&8192));
 if(uiFlags&128)touchInspect.textContent=uiFlags&2048?tr('Dzielnice','Districts'):tr('Trasa','Route');
 document.querySelector('#focus-pause').setAttribute('aria-pressed',String(!!(uiFlags&4096)));
 if(!started){start.textContent=ready?tr('Uruchom','Start'):tr('Ładowanie…','Loading…');saveStatus.textContent=ready?tr('Gotowe','Ready'):tr('Ładowanie gry…','Loading game…');}
}
function selectLanguage(language){
 english=language==='en';explicitBootLanguage=true;
 try{localStorage.setItem(languageKey,english?'en':'pl');localStorage.setItem(pendingLanguageKey,english?'en':'pl');}catch{}
 translateUI();
 if(started){Module._web_set_language(english?1:0);flushBrowserStorage();canvas.focus();}
 else drawBoot();
}
for(const b of document.querySelectorAll('[data-language]'))b.onclick=()=>selectLanguage(b.dataset.language);
for(const b of document.querySelectorAll('[data-language-toggle]'))b.onclick=()=>selectLanguage(english?'pl':'en');
window.tvUI=u=>{
 const nextEnglish=!!(u.flags&512);
 // The game sends UI state every frame. Only a transition changes the controls.
 if(u.flags===uiFlags&&english===nextEnglish)return;
 const modalFlags=1|2|4|8|64|128;
 // A doorway still reads held movement while its shutter opens.
 if(!!(u.flags&modalFlags)!==!!(uiFlags&modalFlags))clearInput();
 lastBlocked=!!(u.flags&(modalFlags|1024));
 const before=uiFlags;uiFlags=u.flags;
 if((before&255)!==(uiFlags&255))resize();
 if(english!==nextEnglish){english=nextEnglish;try{localStorage.setItem(languageKey,english?'en':'pl');}catch{}}
 settingsKeys.hidden=!(u.flags&16);
 translateUI();
};
const keyMap={ArrowLeft:0,KeyA:0,ArrowRight:1,KeyD:1,ArrowUp:2,KeyW:2,ArrowDown:3,KeyS:3,Space:4,KeyX:5,ShiftLeft:5,ShiftRight:5,KeyE:6,KeyF:6,KeyB:7,Enter:8,Escape:9,KeyP:9,Tab:10,KeyJ:11,KeyM:12,KeyR:13,KeyN:14,KeyL:15,KeyU:16,Digit1:20,Digit2:21,Digit3:22,Digit4:23,Digit5:24,Digit6:25};
function recompute(){
 held=0;
 for(const code of keyboard)held|=1<<keyMap[code];
 for(const {key} of pointers.values())if(key!==null)held|=1<<key;
}
addEventListener('keydown',e=>{
 const shiftKey=e.code==='ShiftLeft'||e.code==='ShiftRight';
 if(!shiftKey&&!e.repeat)dashShift='';
 if(e.code==='Tab'&&(e.shiftKey||e.target!==canvas||(uiFlags&2)))return;
 if(e.target instanceof Element&&e.target.closest('button,a,input,select,textarea,[contenteditable]'))return;
 if(accounts.active||!started||keyMap[e.code]===undefined||e.ctrlKey||e.metaKey||e.altKey)return;
 e.preventDefault();
 if(e.repeat)return;
 // Shift also starts browser shortcuts. Dash only if it was tapped alone.
 if(shiftKey){dashShift=e.code;return;}
 tap(keyMap[e.code]);
 keyboard.add(e.code);
 recompute();
});
canvas.addEventListener('blur',clearInput);
addEventListener('keyup',e=>{
 if(e.code===dashShift){dashShift='';tap(5);}
 if(keyboard.delete(e.code))recompute();
});
const horizontalTouchControls='#touch .directions :is(button[data-key="0"], button[data-key="1"])';
function releaseHighlight(button){
 if(button&&![...pointers.values()].some(pointer=>pointer.button===button))button.classList.remove('held');
}
function finishPointer(id,completed){
 const pointer=pointers.get(id);
 if(!pointer)return;
 pointers.delete(id);
 if(completed)completedPresses|=pointer.pending;
 // A cancelled contact must not erase other inputs of the same action.
 pressed=completedPresses;
 for(const active of pointers.values())pressed|=active.pending;
 releaseHighlight(pointer.button);
 recompute();
}
for(const button of document.querySelectorAll('[data-key]')){
 const action=()=>button.id==='touch-use'&&lastBlocked?((uiFlags&64)?6:8):+button.dataset.key;
 button.addEventListener('click',e=>{
  // Pointer clicks already sent their input on pointerdown.
  if(e.detail!==0||accounts.active||!started)return;
  canvas.focus();
  tap(action());
 });
 button.addEventListener('pointerdown',e=>{
  if(e.button!==0||accounts.active||!started)return;
  e.preventDefault();
  canvas.focus({preventScroll:true});
  const key=action();
  button.setPointerCapture(e.pointerId);
  const canSlide=e.pointerType==='touch'&&button.matches(horizontalTouchControls);
  const pointer={key,button,canSlide,pending:0};
  pointers.set(e.pointerId,pointer);
  button.classList.add('held');pointerPress(pointer);recompute();
 });
 button.addEventListener('pointermove',e=>{
  const pointer=pointers.get(e.pointerId);
  if(!pointer)return;
  // Mouse chords report the primary release as a move until every button is up.
  if(e.pointerType==='mouse'&&!(e.buttons&1)){
   finishPointer(e.pointerId,true);
   if(button.hasPointerCapture(e.pointerId))button.releasePointerCapture(e.pointerId);
   return;
  }
  if(!pointer.canSlide)return;
  // Passing over the jump button while sliding horizontally leaves movement neutral.
  const next=document.elementFromPoint(e.clientX,e.clientY)?.closest(horizontalTouchControls)??null;
  if(pointer.button===next)return;
  const previous=pointer.button;
  pointer.button=next;
  pointer.key=next?+next.dataset.key:null;
  releaseHighlight(previous);
  if(next){next.classList.add('held');pointerPress(pointer);}
  recompute();
 });
 for(const type of ['pointerup','pointercancel','lostpointercapture'])
  button.addEventListener(type,e=>finishPointer(e.pointerId,type==='pointerup'));
}
for(const b of document.querySelectorAll('[data-tap]'))b.onclick=()=>{tap(+b.dataset.tap);canvas.focus();};
function clearInput(){keyboard.clear();pointers.clear();dashShift='';held=0;clearPresses();document.querySelectorAll('.held').forEach(e=>e.classList.remove('held'));}
function flushBrowserStorage(){
 if(syncing){storageFlushQueued=true;return;}
 syncing=true;
 Module.FS.syncfs(false,error=>{
  syncing=false;
  if(storageFlushQueued){
   storageFlushQueued=false;
   flushBrowserStorage();
   return;
  }
  if(!error)try{localStorage.removeItem(pendingLanguageKey);}catch{}
  saveStatus.textContent=error
   ?tr('Błąd zapisu — pobierz kopię','Save failed — download a copy')
   :tr('Zapis w tej przeglądarce ✓','Saved in this browser ✓');
 });
}
function syncSave(){
 if(!started)return;
 Module._web_save();
 flushBrowserStorage();
}
function updateFocus(hasFocus){
 const next=hasFocus&&!document.hidden;
 if(focused===next)return;
 focused=next;
 last=performance.now();
 if(focused)return;
 clearInput();
 // A hidden tab can lose every animation frame between blur and focus.
 // Deliver the focus edge without advancing the game clock.
 if(started&&!accounts.active)Module._web_tick(0,0,0,cols,rows,0);
 syncSave();
}
addEventListener('blur',()=>updateFocus(false));
addEventListener('focus',()=>updateFocus(true));
document.addEventListener('visibilitychange',()=>updateFocus(document.hasFocus()));
function resize(){
 const compact=mobile()&&!!(uiFlags&255),landscape=innerHeight<500&&stage.clientWidth>560;
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
const settingsKeys=document.querySelector('#settings-keys');
const expanded=()=>!!document.fullscreenElement||playfield.classList.contains('expanded');
function fullscreenLayout(){
 const active=expanded();
 if(active)document.querySelector('#fullscreen-tools').append(settingsKeys);
 else document.querySelector('#hint').before(settingsKeys);
 clearInput();
 resize();
 canvas.focus();
 document.querySelector('#fullscreen').setAttribute('aria-pressed',String(active));
}
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
const exportSave=document.querySelector('#export-save');
const importSave=document.querySelector('#import-save');
const exportPreviousSave=document.querySelector('#export-previous-save');
const saveFile=document.querySelector('#save-file');
const previousImportPath='/persist/before-manual-import.iceberg';
let importPending=false;

function downloadSave(path,name) {
 try {
  const bytes=Module.FS.readFile(path);
  const url=URL.createObjectURL(new Blob([bytes],{type:'application/octet-stream'}));
  const link=document.createElement('a');
  link.href=url;link.download=name;link.click();
  setTimeout(()=>URL.revokeObjectURL(url),1000);
 } catch {
  saveStatus.textContent=tr('Brak zapisu do pobrania','No save to download');
 }
}
exportSave.onclick=()=>{
 Module._web_save();
 downloadSave('/persist/progress.dat.iceberg','terminal-velocity.iceberg');
};
exportPreviousSave.onclick=()=>downloadSave(previousImportPath,'terminal-velocity-previous.iceberg');
function finishImport(){
 importPending=false;
 importSave.disabled=false;
 clearInput();
 last=performance.now();
 if(!accounts.active)canvas.focus();
}
importSave.onclick=()=>{
 importPending=true;
 importSave.disabled=true;
 clearInput();
 saveFile.click();
};
saveFile.oncancel=finishImport;
saveFile.onchange=async()=>{
 const file=saveFile.files[0];
 saveFile.value='';
 try {
  if(!file)return;
  if(!file.size||file.size>65536) {
   saveStatus.textContent=tr('Nieprawidłowy plik zapisu','Invalid save file');
   return;
  }
  const bytes=new Uint8Array(await file.arrayBuffer());
  if(accounts.active||accounts.busy||accounts.mode==='choose'){
   saveStatus.textContent=tr('Zakończ obsługę konta przed wczytaniem zapisu','Finish the account action before importing a save');
   return;
  }
  const question=accounts.user
   ?tr('Zastąpić obecny zapis? Nowy zapis będzie synchronizowany z kontem.','Replace your current save? The imported save will sync to your account.')
   :tr('Zastąpić obecny zapis w tej przeglądarce?','Replace the current save in this browser?');
  if(!window.confirm(question))return;
  const fs=Module.FS;
  Module._web_save();
  const previous=fs.readFile('/persist/progress.dat.iceberg');
  const incoming='/persist/import.iceberg';
  fs.writeFile(incoming,bytes);
  try {
   if(!Module._web_import_save()){
    saveStatus.textContent=tr('Nieprawidłowy plik zapisu','Invalid save file');
    return;
   }
  } finally {fs.unlink(incoming);}
  fs.writeFile(previousImportPath,previous);
  exportPreviousSave.hidden=false;
  Module._web_enter_campaign();
  flushBrowserStorage();
 } catch {
  saveStatus.textContent=tr('Nie udało się wczytać zapisu','Could not import save');
 } finally {
  finishImport();
 }
};
function frame(now){
 const elapsed=Math.max(0,(now-last)/1000);
 const stalled=elapsed>.5;
 // Resume with a normal step after an isolated stall so rapid motion stays visible.
 const dt=stalled&&!frameWasStalled?1/60:Math.min(.1,elapsed);
 frameWasStalled=stalled;
 if(accounts.active)accounts.draw();
 else if(started&&!importPending){
  Module._web_tick(dt,held,pressed,cols,rows,focused?1:0);
  clearPresses();
 }
 last=now;
 requestAnimationFrame(frame);
}
start.onclick=()=>{
 if(!ready||started)return;
 try {
  Module._web_start(Date.now()>>>0,english?1:0);
  Module._web_enter_campaign();
  started=true;start.hidden=true;exportSave.disabled=importSave.disabled=false;
  exportPreviousSave.hidden=!Module.FS.analyzePath(previousImportPath).exists;
  syncSave();accounts.start();resize();last=performance.now();canvas.focus();requestAnimationFrame(frame);
 } catch(error) {
  console.error(error);started=false;ready=false;start.disabled=true;
  bootStatus=tr('Błąd uruchamiania — odśwież stronę','Startup failed — reload the page');
  saveStatus.textContent=bootStatus;drawBoot();
 }
};
canvas.onclick=()=>{if(!started)start.click();else if(!accounts.active&&(uiFlags&(2|4|1024)))tap(8);};
addEventListener('keydown',e=>{
 if(!started&&e.code==='Enter'&&!e.defaultPrevented&&(e.target===canvas||e.target===document.body))start.click();
});
var Module={onRuntimeInitialized(){try{Module.FS.mkdir('/persist');Module.FS.mount(Module.IDBFS,{},'/persist');Module.FS.syncfs(true,async error=>{
 // A valid game save is authoritative even before the player presses Start.
 // An explicit selection made while loading takes precedence over that save.
 if(!explicitBootLanguage){
  const saved=Module._web_saved_language();
  if(saved===0||saved===1)english=saved===1;
  else {
   const nearbyLanguage=await locationLanguage;
   if(!explicitBootLanguage&&nearbyLanguage)english=nearbyLanguage==='en';
  }
 }
 try{localStorage.setItem(languageKey,english?'en':'pl');}catch{}
 ready=true;start.disabled=false;translateUI();
 if(error)saveStatus.textContent=tr('Zapis przeglądarki niedostępny — pobieraj kopię','Browser storage unavailable — download backups');
 drawBoot();document.fonts.load('17px Cell').then(()=>{tiles.clear();previous=new Uint32Array(0);});
 });}catch(e){bootStatus=tr('Błąd uruchamiania: ','Startup failed: ')+String(e);drawBoot();}},setStatus(text){if(!text)bootStatus='';drawBoot();},onAbort(error){bootStatus=tr('Błąd uruchamiania: ','Startup failed: ')+error;drawBoot();}};
const accounts=new TVAccounts({english:()=>english,ready:()=>started,clear:clearInput,frame:window.tvFrame,needsRedraw:()=>previous.length===0,flush:flushBrowserStorage});
document.querySelector('#account').onclick=()=>accounts.open();
document.querySelector('#account-fullscreen').onclick=()=>accounts.open();
setInterval(syncSave,5000);
setInterval(()=>accounts.sync(),15000);

translateUI();resize();

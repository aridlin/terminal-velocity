'use strict';
const canvas=document.querySelector('#game'),ctx=canvas.getContext('2d',{alpha:false});
const stage=document.querySelector('#stage'),start=document.querySelector('#start'),saveStatus=document.querySelector('#save-status');
const gameButtons=document.querySelectorAll('[data-tap],[data-key],#account,#account-fullscreen,#export-save,#import-save');
function setGameButtons(enabled){
 for(const button of gameButtons)button.disabled=!enabled;
}
setGameButtons(false);
const mobile=()=>matchMedia('(pointer:coarse)').matches||innerWidth<760||(innerHeight<500&&innerWidth<1100);
const languageKey='terminal-velocity.language',pendingLanguageKey='terminal-velocity.pending-language';
let pendingBootLanguage=null;
let hasStoredLanguage=false;
let preferredLanguage=(navigator.languages?.[0]||navigator.language||'en').toLowerCase().split('-')[0]==='pl'?'pl':'en';
try{const saved=localStorage.getItem(languageKey);if(saved==='pl'||saved==='en'){preferredLanguage=saved;hasStoredLanguage=true;}const pending=localStorage.getItem(pendingLanguageKey);if(pending==='pl'||pending==='en')preferredLanguage=pendingBootLanguage=pending;}catch{}
let ready=false,started=false,held=0,pressed=0,cols=110,rows=34,previous=new Uint32Array(0),last=0,focused=true,syncing=false,lastBlocked=false,uiFlags=0,english=preferredLanguage==='en',bootFailed=false,explicitBootLanguage=pendingBootLanguage!==null;
const tr=(pl,en)=>english?en:pl;
const soundMutedFlag=1<<15,mutePreferenceKey='terminal-velocity.muted';
const localSaveFailedFlag=1<<16,tourFlag=1<<22,profilePickerFlag=1<<23,smoothCameraFlag=1<<24;
const selectedSlotKey='terminal-velocity.save-slot';
let selectedSlot=1,accountStarted=false,accountBooting=false;
try {
 const value=localStorage.getItem(selectedSlotKey);
 if(value==='1'||value==='2'||value==='3')selectedSlot=Number(value);
}catch{}
const inTour=()=>!!(uiFlags&tourFlag);
const persistentSaveDirectory='/persist';
let saveDirectory=persistentSaveDirectory;
const accountSlot=()=>started&&selectedSlot===1&&!inTour()&&saveDirectory===persistentSaveDirectory;
const startupError=()=>tr('Błąd uruchamiania — odśwież stronę','Startup failed — reload the page');
let browserSaveStatus=['Gotowe','Ready'],saveActionError=null;
function renderSaveStatus(){
 const text=bootFailed?startupError():ready
  ?tr(...(saveActionError??((uiFlags&localSaveFailedFlag)?['Błąd zapisu','Save failed']:browserSaveStatus)))
  :tr('Ładowanie gry…','Loading game…');
 if(saveStatus.textContent!==text)saveStatus.textContent=text;
}
function setSaveActionError(message){
 saveActionError=message;
 renderSaveStatus();
}
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
const shiftKeys=new Set();
let dashShift='';
let frameWasStalled=false;
let storageFlushQueued=false;
let browserStorageWritable=false;
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
 dashShift='';
 pointer.pending|=1<<pointer.key;
 pressed|=1<<pointer.key;
}
function clearPresses(){
 pressed=completedPresses=0;
 for(const pointer of pointers.values())pointer.pending=0;
}
const overlayFlags=()=>uiFlags&1?0:uiFlags;
const confirmSaveFlag=1<<5;
const roomMapFlag=1<<17;
const forkDoorFlag=1<<18;
const replaceRunFlag=1<<19;
const districtTravelFlag=1<<25;
const readOnlyMapOpen=()=>!!((overlayFlags()&128)&&!(overlayFlags()&districtTravelFlag));
const questRecordOpen=()=>(overlayFlags()&(4|64))===(4|64);
function translateUI(){
 document.documentElement.lang=english?'en':'pl';
 document.title='Terminal Velocity — '+tr('Pod powierzchnią','Below the surface');
 for(const b of document.querySelectorAll('[data-pl]'))b.textContent=tr(b.dataset.pl,b.dataset.en);
 for(const b of document.querySelectorAll('[data-aria-pl]'))b.setAttribute('aria-label',tr(b.dataset.ariaPl,b.dataset.ariaEn));
 for(const b of document.querySelectorAll('[data-language]'))b.setAttribute('aria-pressed',String(b.dataset.language===(english?'en':'pl')));
 document.querySelector('#hint').textContent=tr('A/D ruch · Spacja skok · X lub wciśnij i puść Shift, by wykonać zryw · E użyj · B obejrzyj · P pauza · Tab zadania · Shift+Tab sterowanie przeglądarki','A/D move · Space jump · X or tap Shift to dash · E use · B inspect · P pause · Tab quests · Shift+Tab browser controls');
 const touchUse=document.querySelector('#touch-use');
 const replacingSave=!!(uiFlags&confirmSaveFlag);
 if(replacingSave){
  touchUse.textContent=tr('Zastąp','Replace');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(uiFlags&forkDoorFlag){
  touchUse.textContent=tr('Przejście B','Passage B');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(readOnlyMapOpen()){
  touchUse.textContent=tr('Wróć','Back');
  touchUse.setAttribute('aria-label',tr('Zamknij mapę','Close map'));
 }else if(overlayFlags()&districtTravelFlag){
  touchUse.textContent=tr('Podróżuj','Travel');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else{
  touchUse.textContent=lastBlocked?'↵':tr('Użyj','Use');
  touchUse.setAttribute('aria-label',lastBlocked?tr('Zatwierdź','Confirm'):tr('Użyj','Use'));
 }
 const districtControl=!!(uiFlags&16384);
 for(const button of document.querySelectorAll('[data-tap="11"]')){
  button.textContent=districtControl?tr('Dzielnica','District'):tr('Mapa','Map');
  button.setAttribute('aria-label',districtControl?tr('Zmień dzielnicę','Change district'):button.textContent);
 }
 const touchInspect=document.querySelector('#touch-inspect');
 const overlay=overlayFlags();
 touchInspect.hidden=!!((overlay&128)&&(overlay&tourFlag));
 let inspectLabel;
 if(replacingSave){
  touchInspect.textContent=tr('Anuluj','Cancel');
 }else if(overlay&forkDoorFlag){
  touchInspect.textContent=tr('Główne drzwi','Main door');
 }else if(questRecordOpen()){
  touchInspect.textContent=tr('Wróć','Back');
  inspectLabel=tr('Wróć do księgi zadań','Back to quest book');
 }else if(overlay&8){
  touchInspect.textContent=tr('Zamknij','Close');
  inspectLabel=tr('Zamknij podgląd','Close inspection');
 }else if(overlay&64){
  touchInspect.textContent=tr('Zakładka','Tab');
  inspectLabel=tr('Następna zakładka','Next quest tab');
 }else if(overlay&128){
  if((overlay&roomMapFlag)&&!(overlay&8192))touchInspect.textContent=tr('Dzielnice','Districts');
  else if((overlay&2048)&&!(overlay&roomMapFlag)){
   touchInspect.textContent=tr('Pokój','Room');
   inspectLabel=tr('Pokaż plan pokoju','Show room map');
  }else {
   touchInspect.textContent=tr('Trasa','Route');
   if(overlay&roomMapFlag)inspectLabel=tr('Pokaż trasę','Show route map');
  }
 }
 touchInspect.setAttribute('aria-label',inspectLabel||touchInspect.textContent);
 document.querySelector('#focus-pause').setAttribute('aria-pressed',String(!!(uiFlags&4096)));
 document.querySelector('#smooth-camera').setAttribute('aria-pressed',String(!!(uiFlags&smoothCameraFlag)));
 soundButton.setAttribute('aria-pressed',String(started&&!(uiFlags&soundMutedFlag)));
 if(!started)start.textContent=ready||bootFailed?tr('Uruchom','Start'):tr('Ładowanie…','Loading…');
 renderSaveStatus();
}
function selectLanguage(language){
 english=language==='en';explicitBootLanguage=true;
 try{localStorage.setItem(languageKey,english?'en':'pl');localStorage.setItem(pendingLanguageKey,english?'en':'pl');}catch{}
 translateUI();
 if(started){Module._web_set_language(english?1:0);flushBrowserStorage();canvas.focus();}
 else drawBoot();
}
for(const b of document.querySelectorAll('[data-language]'))b.onclick=()=>selectLanguage(b.dataset.language);
const languageToggle=document.querySelector('[data-language-toggle]');
languageToggle.onclick=()=>selectLanguage(english?'pl':'en');
window.tvUI=u=>{
 const nextEnglish=!!(u.flags&512);
 // The game sends UI state every frame. Only a transition changes the controls.
 if(u.flags===uiFlags&&english===nextEnglish)return;
 const previousSlot=selectedSlot;
 if(started)selectedSlot=1+((u.flags>>>20)&3);
 const blockingFlags=1|2|4|8|64|128;
 const inputContextFlags=blockingFlags|2048|confirmSaveFlag|roomMapFlag|profilePickerFlag|tourFlag;
 // A doorway still reads held movement while its shutter opens.
 if((u.flags&inputContextFlags)!==(uiFlags&inputContextFlags))clearInput();
 lastBlocked=!!(u.flags&(blockingFlags|1024));
 const before=uiFlags;uiFlags=u.flags;
 if((before&255)!==(uiFlags&255))resize();
 if((before^uiFlags)&soundMutedFlag)try{localStorage.setItem(mutePreferenceKey,uiFlags&soundMutedFlag?'1':'0');}catch{}
 if(english!==nextEnglish){english=nextEnglish;try{localStorage.setItem(languageKey,english?'en':'pl');}catch{}}
 settingsKeys.hidden=!(u.flags&16);
 if(previousSlot!==selectedSlot){
  try{localStorage.setItem(selectedSlotKey,String(selectedSlot));}catch{}
  setSaveActionError(null);
 }
 if(previousSlot!==selectedSlot||((before^uiFlags)&tourFlag)){
  refreshSaveControls();
  maybeStartAccount();
 }
 translateUI();
};
const keyMap={ArrowLeft:0,KeyA:0,ArrowRight:1,KeyD:1,ArrowUp:2,KeyW:2,ArrowDown:3,KeyS:3,Space:4,KeyX:5,ShiftLeft:5,ShiftRight:5,KeyE:6,KeyF:6,KeyB:7,Enter:8,Escape:9,KeyP:9,Tab:10,KeyJ:11,KeyM:12,KeyR:13,KeyN:14,KeyL:15,KeyU:16,Digit1:20,Digit2:21,Digit3:22,Digit4:23,Digit5:24,Digit6:25,Digit7:26};
function recompute(){
 held=0;
 for(const code of keyboard)held|=1<<keyMap[code];
 for(const {key} of pointers.values())if(key!==null)held|=1<<key;
}
addEventListener('keydown',e=>{
 const shiftKey=e.code==='ShiftLeft'||e.code==='ShiftRight';
 if(!e.repeat){
  if(shiftKey)shiftKeys.add(e.code);
  if(!shiftKey||shiftKeys.size>1)dashShift='';
 }
 const pauseScreen=(uiFlags&(2|64|128))===2;
 if(e.code==='Tab'&&(e.shiftKey||e.target!==canvas||pauseScreen))return;
 if(e.target instanceof Element&&e.target.closest('button,a,input,select,textarea,[contenteditable]'))return;
 if(accounts?.active||!started||keyMap[e.code]===undefined||e.ctrlKey||e.metaKey||e.altKey)return;
 e.preventDefault();
 if(e.repeat)return;
 // Shift also starts browser shortcuts. Dash only if it was tapped alone.
 if(shiftKey){
  if(shiftKeys.size===1)dashShift=e.code;
  return;
 }
 tap(keyMap[e.code]);
 keyboard.add(e.code);
 recompute();
});
canvas.addEventListener('blur',clearInput);
addEventListener('keyup',e=>{
 shiftKeys.delete(e.code);
 if(e.code===dashShift){
  dashShift='';
  if(!e.shiftKey)tap(5);
 }
 if(keyboard.delete(e.code))recompute();
});
// A modified click or scroll also uses Shift, even without another keydown.
const cancelShiftDash=()=>{dashShift='';};
addEventListener('pointerdown',cancelShiftDash,{capture:true});
addEventListener('wheel',cancelShiftDash,{capture:true,passive:true});
const horizontalTouchControls='#touch .directions :is(button[data-key="0"], button[data-key="1"])';
function releaseHighlight(button){
 if(button&&![...pointers.values()].some(pointer=>pointer.button===button))button.classList.remove('held');
}
function recomputePresses(){
 pressed=completedPresses;
 for(const pointer of pointers.values())pressed|=pointer.pending;
}
function finishPointer(id,completed){
 const pointer=pointers.get(id);
 if(!pointer)return;
 pointers.delete(id);
 if(completed)completedPresses|=pointer.pending;
 // A cancelled contact must not erase other inputs of the same action.
 recomputePresses();
 releaseHighlight(pointer.button);
 recompute();
}
for(const button of document.querySelectorAll('[data-key]')){
 const action=()=>{
  if(uiFlags&forkDoorFlag){
   if(button.id==='touch-use')return 6;
   if(button.id==='touch-inspect')return 8;
  }
  if(button.id==='touch-inspect'&&((uiFlags&confirmSaveFlag)||questRecordOpen()))return 9;
  if(button.id!=='touch-use'||!lastBlocked)return +button.dataset.key;
  if(readOnlyMapOpen())return 11;
  return (overlayFlags()&64)||(uiFlags&replaceRunFlag)?6:8;
 };
 button.addEventListener('click',e=>{
  // Pointer clicks already sent their input on pointerdown.
  if(e.detail!==0||accounts?.active||!started)return;
  canvas.focus();
  tap(action());
 });
 button.addEventListener('pointerdown',e=>{
  if(e.button!==0||accounts?.active||!started)return;
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
  // A slide replaces this contact's unconsumed direction.
  pointer.pending=0;
  releaseHighlight(previous);
  if(next){next.classList.add('held');pointerPress(pointer);}
  recomputePresses();
  recompute();
 });
 for(const type of ['pointerup','pointercancel','lostpointercapture'])
  button.addEventListener(type,e=>finishPointer(e.pointerId,type==='pointerup'));
}
for(const b of document.querySelectorAll('[data-tap]'))b.onclick=()=>{
 if(!started||accounts?.active||importPending)return;
 canvas.focus({preventScroll:true});
 tap(+b.dataset.tap);
};
function clearInput(){keyboard.clear();pointers.clear();shiftKeys.clear();dashShift='';held=0;clearPresses();document.querySelectorAll('.held').forEach(e=>e.classList.remove('held'));}
function flushBrowserStorage(){
 // A failed restore or a second tab cannot safely replace the IndexedDB contents.
 if(!browserStorageWritable)return;
 if(syncing){storageFlushQueued=true;return;}
 syncing=true;
 Module.FS.syncfs(false,error=>{
  syncing=false;
  if(bootFailed){storageFlushQueued=false;return;}
  if(storageFlushQueued){
   storageFlushQueued=false;
   flushBrowserStorage();
   return;
  }
  if(!error)try{localStorage.removeItem(pendingLanguageKey);}catch{}
  browserSaveStatus=error
   ?['Błąd zapisu — pobierz kopię','Save failed — download a copy']
   :['Zapis w tej przeglądarce ✓','Saved in this browser ✓'];
  renderSaveStatus();
 });
}
window.tvSaveReady=()=>{
 refreshUnreadableSaveControls();
 flushBrowserStorage();
};
function syncSave(){
 if(!started||inTour())return;
 Module._web_save();
 window.tvSaveReady();
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
 if(started&&!accounts?.active)Module._web_tick(0,0,0,cols,rows,0);
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
 line(16,bootFailed?startupError():ready?tr('Dotknij / ENTER — rozpocznij','Tap / ENTER — start'):tr('Ładowanie gry…','Loading game…'),0x9dd4dd);
 line(19,tr('Klawiatura lub ekran dotykowy.','Keyboard or touch controls.'));line(20,tr('Zapis zostaje w przeglądarce.','Progress stays in this browser.'));
 window.tvFrame(cells,w,h);
}
const playfield=document.querySelector('#playfield');
const settingsKeys=document.querySelector('#settings-keys');
const retryButton=document.querySelector('#toolbar [data-tap="13"]');
const soundButton=document.querySelector('#sound');
const soundAnchor=soundButton.nextElementSibling;
const toolbar=document.querySelector('#toolbar'),fullscreenTools=document.querySelector('#fullscreen-tools');
const saveControls=document.querySelector('#save-controls'),fullscreenButton=document.querySelector('#fullscreen');
const leaveFullscreen=document.querySelector('#leave-fullscreen');
const pageControls=document.querySelectorAll('body > header, body > footer');
const expanded=()=>!!document.fullscreenElement||playfield.classList.contains('expanded');
function fullscreenLayout(){
 const active=expanded();
 const fallback=playfield.classList.contains('expanded');
 for(const controls of pageControls)controls.inert=fallback;
 if(active){
  fullscreenTools.prepend(start);
  document.querySelector('#account-fullscreen').before(retryButton,soundButton);
  leaveFullscreen.before(saveControls,languageToggle);
  fullscreenTools.append(saveStatus,settingsKeys);
 }else{
  toolbar.prepend(start);
  soundAnchor.before(retryButton,soundButton);
  toolbar.append(saveControls,languageToggle);
  fullscreenButton.before(saveStatus);
  document.querySelector('#hint').before(settingsKeys);
 }
 clearInput();
 resize();
 const field=document.activeElement;
 if(!accounts?.active||(field!==accounts.name&&field!==accounts.pass))canvas.focus();
 fullscreenButton.setAttribute('aria-pressed',String(active));
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
 if((e.code==='Enter'&&e.altKey)||(e.code==='Escape'&&expanded()&&!accounts?.active)){
  e.preventDefault();e.stopImmediatePropagation();
  if(!e.repeat)toggleFullscreen();
 }
},{capture:true});
fullscreenButton.onclick=toggleFullscreen;
leaveFullscreen.onclick=toggleFullscreen;
document.addEventListener('fullscreenchange',fullscreenLayout);
new ResizeObserver(resize).observe(stage);
const exportSave=document.querySelector('#export-save');
const importSave=document.querySelector('#import-save');
const exportPreviousSave=document.querySelector('#export-previous-save');
const unreadableSaveButtons=[
 document.querySelector('#export-unreadable-primary'),
 document.querySelector('#export-unreadable-backup')
];
let unreadableSaveNames=[null,null];
const saveFile=document.querySelector('#save-file');
const slotPath=base=>selectedSlot===1?base:base+'.slot'+selectedSlot;
const currentSavePath=()=>slotPath(saveDirectory+'/progress.dat.iceberg');
const previousImportPath=()=>slotPath(saveDirectory+'/before-manual-import.iceberg');
let importPending=false;

function refreshUnreadableSaveControls(){
 unreadableSaveNames=[null,null];
 if(started&&!inTour()){
  const base=currentSavePath().split('/').at(-1);
  const prefixes=[base+'.unreadable',base+'.bak.unreadable'];
  const names=Module.FS.readdir(saveDirectory);
  for(const [i,prefix] of prefixes.entries()){
   let latest=-1;
   for(const name of names){
    const version=name===prefix?0:name.startsWith(prefix+'.')?Number(name.slice(prefix.length+1)):NaN;
    if(!Number.isSafeInteger(version)||version<=latest)continue;
    latest=version;
    unreadableSaveNames[i]=name;
   }
  }
 }
 unreadableSaveButtons.forEach((button,i)=>{button.hidden=unreadableSaveNames[i]===null;});
}
function refreshSaveControls(){
 for(const button of document.querySelectorAll('#account,#account-fullscreen'))
  button.hidden=!accounts||!accountSlot();
 exportSave.disabled=!started||inTour();
 importSave.disabled=!started||inTour()||importPending;
 exportPreviousSave.hidden=!started||inTour()||!Module.FS.analyzePath(previousImportPath()).exists;
 refreshUnreadableSaveControls();
}
function maybeStartAccount(){
 if(!accounts||!accountSlot()||!browserStorageWritable||accountStarted)return;
 accountStarted=accountBooting=true;
 accounts.start().finally(()=>{accountBooting=false;});
}

function readCurrentSave(forAccount=false){
 if(Module._web_save())return Module.FS.readFile(currentSavePath());
 if(forAccount&&!Module._web_primary_usable_for_account())throw Error('save_write_failed');
 if(!Module._web_export_recovery())throw Error('save_write_failed');
 const recovery='/tmp/terminal-velocity-export.iceberg';
 try{return Module.FS.readFile(recovery);}
 finally{Module.FS.unlink(recovery);}
}
function downloadSave(name,read){
 try {
  const url=URL.createObjectURL(new Blob([read()],{type:'application/octet-stream'}));
  const link=document.createElement('a');
  link.href=url;link.download=name;link.click();
  setTimeout(()=>URL.revokeObjectURL(url),1000);
  setSaveActionError(null);
 } catch {
  setSaveActionError(['Brak zapisu do pobrania','No save to download']);
 }
}
exportSave.onclick=()=>downloadSave('terminal-velocity.iceberg',readCurrentSave);
exportPreviousSave.onclick=()=>downloadSave('terminal-velocity-previous.iceberg',()=>Module.FS.readFile(previousImportPath()));
unreadableSaveButtons.forEach((button,i)=>{
 button.onclick=()=>{
  const name=unreadableSaveNames[i];
  if(name)downloadSave(name,()=>Module.FS.readFile(saveDirectory+'/'+name));
 };
});
function finishImport(){
 importPending=false;
 refreshSaveControls();
 clearInput();
 last=performance.now();
 if(!accounts?.active)canvas.focus({preventScroll:true});
}
importSave.onclick=()=>{
 clearInput();
 canvas.focus({preventScroll:true});
 saveFile.click();
};
saveFile.oncancel=()=>{if(!importPending)finishImport();};
saveFile.onchange=async()=>{
 const file=saveFile.files[0];
 saveFile.value='';
 // Only an actual selection starts an import. A picker dismissal may never
 // reach the page, so opening the picker cannot suspend the game loop.
 if(!file||importPending||inTour())return;
 importPending=true;
 importSave.disabled=true;
 setSaveActionError(null);
 clearInput();
 try {
  if(!file.size||file.size>65536) {
   setSaveActionError(['Nieprawidłowy plik zapisu','Invalid save file']);
   return;
  }
  const bytes=new Uint8Array(await file.arrayBuffer());
  if(accountSlot()&&(accountBooting||accounts?.active||accounts?.busy||accounts?.mode==='choose')){
   setSaveActionError(['Zakończ obsługę konta przed wczytaniem zapisu','Finish the account action before importing a save']);
   return;
  }
  const question=accountSlot()&&accounts?.user
   ?tr('Zastąpić obecny zapis? Nowy zapis będzie synchronizowany z kontem.','Replace your current save? The imported save will sync to your account.')
   :tr('Zastąpić obecny zapis w tej przeglądarce?','Replace the current save in this browser?');
  if(!window.confirm(question))return;
  const fs=Module.FS;
  if(!Module._web_save()){
   setSaveActionError(['Błąd zapisu — pobierz kopię','Save failed — download a copy']);
   return;
  }
  const previous=fs.readFile(currentSavePath());
  const incoming=saveDirectory+'/import.iceberg';
  fs.writeFile(incoming,bytes);
  try {
   const imported=Module._web_import_save();
   if(imported!==1){
    setSaveActionError(imported===0
     ?['Nieprawidłowy plik zapisu','Invalid save file']
     :['Nie udało się zapisać wczytywanej gry','Could not write the imported save']);
    return;
   }
  } finally {fs.unlink(incoming);}
  fs.writeFile(previousImportPath(),previous);
  exportPreviousSave.hidden=false;
  Module._web_enter_campaign();
  flushBrowserStorage();
 } catch {
  setSaveActionError(['Nie udało się wczytać zapisu','Could not import save']);
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
 if(started&&accounts?.active)accounts.draw();
 else if(started&&!importPending){
  Module._web_profile_action_busy(accountBooting||accounts?.busy?1:0);
  Module._web_tick(dt,held,pressed,cols,rows,focused?1:0);
  clearPresses();
 }
 last=now;
 requestAnimationFrame(frame);
}
function failStartup(error){
 if(bootFailed)return;
 if(error)console.error(error);
 bootFailed=true;started=false;ready=false;
 clearInput();
 start.hidden=false;start.disabled=true;
 setGameButtons(false);exportPreviousSave.hidden=true;
 refreshUnreadableSaveControls();
 translateUI();drawBoot();
}
addEventListener('error',event=>{
 if(event.target.id==='game-engine'||(!ready&&event.filename&&event.filename===document.querySelector('#game-engine')?.src))
  failStartup(event.error);
},{capture:true});
// The engine's data preloader can reject before it calls Module.onAbort.
addEventListener('unhandledrejection',event=>{if(!ready)failStartup(event.reason);});
start.onclick=()=>{
 if(!ready||started)return;
 try {
  clearInput();
  let muted=false;
  try{muted=localStorage.getItem(mutePreferenceKey)==='1';}catch{}
  Module._web_start(Date.now()>>>0,english?1:0,muted?1:0);
  Module._web_enter_campaign();
  started=true;
  syncSave();
  maybeStartAccount();
  resize();
  start.hidden=true;
  setGameButtons(true);
  refreshSaveControls();
  last=performance.now();canvas.focus();requestAnimationFrame(frame);
 } catch(error) {
  failStartup(error);
 }
};
canvas.onclick=()=>{
 if(!started){start.click();return;}
 if(accounts?.active||importPending)return;
 const overlay=overlayFlags();
 if((overlay&4)||(!(overlay&(8|64|128))&&(overlay&(2|1024))))tap(8);
};
addEventListener('keydown',e=>{
 if(!started&&e.code==='Enter'&&!e.defaultPrevented&&(e.target===canvas||e.target===document.body))start.click();
});
async function finishBoot(storageError,canWrite=true){
 if(bootFailed)return;
 try {
  Module._web_set_start_slot(selectedSlot);
  // The save's language takes precedence over approximate location, unless
  // the player already made an explicit choice while the game was loading.
  if(!explicitBootLanguage){
   const saved=Module._web_saved_language();
   if(saved===0||saved===1)english=saved===1;
   else {
    const nearbyLanguage=await locationLanguage;
    if(!explicitBootLanguage&&nearbyLanguage)english=nearbyLanguage==='en';
   }
  }
  if(bootFailed)return;
  try{localStorage.setItem(languageKey,english?'en':'pl');}catch{}
  browserStorageWritable=!storageError&&canWrite;
  if(storageError)browserSaveStatus=['Zapis przeglądarki niedostępny — pobieraj kopię','Browser storage unavailable — download backups'];
  else if(!canWrite)browserSaveStatus=['Inna karta zapisuje — pobierz kopię','Another tab is saving — download a copy'];
  ready=true;start.disabled=false;translateUI();
  drawBoot();document.fonts.load('17px Cell').then(()=>{tiles.clear();previous=new Uint32Array(0);});
 }catch(error){failStartup(error);}
}
async function restoreBrowserStorage(){
 let expired=false,deadline;
 const timeout=new Promise(resolve=>{
  deadline=setTimeout(()=>{
   expired=true;
   resolve();
   if(bootFailed)return;
   try {
    // A pending IndexedDB read can still populate /persist after this point.
    // Keep the running game and its exports in a different memory directory.
    saveDirectory='/session-progress';
    Module.FS.mkdir(saveDirectory);
    Module._web_use_temporary_saves();
    finishBoot(new Error('Browser storage restore timed out'),false);
   } catch(error){failStartup(error);}
  },6000);
 });
 const restore=async(canWrite,lockError)=>{
  if(expired)return;
  const storageError=await Promise.race([
   timeout,new Promise(resolve=>Module.FS.syncfs(true,resolve)).catch(error=>error),
  ]);
  if(expired)return;
  clearTimeout(deadline);
  await finishBoot(storageError||lockError,canWrite);
 };
 if(!navigator.locks){await restore(true);return;}
 await navigator.locks.request('terminal-velocity-browser-save',{ifAvailable:true},async lock=>{
  await restore(!!lock);
  // Keep the writer until this document closes. A timed-out restore releases it.
  if(browserStorageWritable)await new Promise(()=>{});
 }).catch(error=>restore(false,error));
}
var Module={
 onRuntimeInitialized(){
  try {
   Module.FS.mkdir(persistentSaveDirectory);
   Module.FS.mount(Module.IDBFS,{},persistentSaveDirectory);
   restoreBrowserStorage().catch(failStartup);
  }catch(error){failStartup(error);}
 },
 setStatus(){if(!started)drawBoot();},
 onAbort:failStartup
};
const accounts=typeof window.TVAccounts==='function'?new window.TVAccounts({
 english:()=>english,ready:()=>accountSlot()&&!accountBooting&&!importPending,clear:clearInput,frame:window.tvFrame,
 needsRedraw:()=>previous.length===0,flush:flushBrowserStorage,
 currentSave:()=>new TextDecoder().decode(readCurrentSave(true)),
 recoveryCopy:()=>unreadableSaveNames[0]
}):null;
for(const button of document.querySelectorAll('#account,#account-fullscreen')){
 button.hidden=true;
 button.onclick=()=>{if(!accountBooting)accounts?.open();};
}
setInterval(syncSave,5000);
if(accounts)setInterval(()=>{if(accountSlot()&&!accountBooting&&!importPending)accounts.sync();},15000);

translateUI();resize();

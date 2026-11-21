'use strict';
const canvas=document.querySelector('#game'),ctx=canvas.getContext('2d',{alpha:false});
const stage=document.querySelector('#stage'),start=document.querySelector('#start'),saveStatus=document.querySelector('#save-status');
const tourControls=document.querySelector('#tour-controls');
const menuSelection=document.querySelector('#menu-selection-actions');
const saveReplacement=document.querySelector('#save-replacement-actions');
const isMenuSelectionControl=element=>menuSelection.contains(element)||
 element?.matches('#touch-use,#touch-inspect,#touch .directions button');
const isSaveReplacementControl=element=>saveReplacement.contains(element)||
 element?.matches('#touch-use,#touch-inspect');
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
let engineLoadTimer=null;
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
let routeLegs=0;
let travelDistrict=-1;
let tourPicker=false;
let awardsOpen=false;
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
let frameWasStalled=false;
let storageFlushQueued=false;
let storageStatusTimers=null;
let browserStorageWritable=false;
let cellPixels=10;
let stageWidth=0,stageHeight=0,canvasFit=null;
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
function fitCanvas(w,h){
 const density=window.devicePixelRatio||1;
 if(canvasFit?.w===w&&canvasFit.h===h&&canvasFit.density===density)return;
 canvasFit={w,h,density};
 const scale=Math.min(stageWidth/(w*10),stageHeight/(h*20));
 canvas.style.width=w*10*scale+'px';
 canvas.style.height=h*20*scale+'px';
 // Rasterize glyphs for the actual display density instead of enlarging a
 // fixed 10x20 bitmap. Whole-pixel cells keep adjacent block art seamless.
 // Two samples per display pixel soften diagonal glyph edges. Bound the
 // extra canvas memory on large screens, while never going below native DPI.
 const displayArea=Math.max(1,w*10*scale*h*20*scale);
 const samples=Math.max(density,Math.min(density*2,Math.sqrt(12000000/displayArea)));
 const pixels=Math.max(1,Math.ceil(10*scale*samples));
 if(cellPixels!==pixels){cellPixels=pixels;tiles.clear();}
 if(canvas.width!==w*pixels||canvas.height!==h*pixels*2){canvas.width=w*pixels;canvas.height=h*pixels*2;previous=new Uint32Array(0);}
}
window.tvFrame=(data,w,h)=>{
 fitCanvas(w,h);
 const pixels=cellPixels;
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
let canvasPress=null;
let replyFirstRow=-1;
window.tvReplyRows=firstRow=>{replyFirstRow=firstRow;};
let tourRows=null;
window.tvTourRows=(first,count,row,left,right)=>{
 tourRows=count?{first,count,row,left,right}:null;
};
let expeditionNodes=null;
window.tvExpeditionNodes=(first,count,row,left,stride,width)=>{
 expeditionNodes=count?{first,count,row,left,stride,width}:null;
};
let dashEdgeOrder=0,completedDash=null;
function dashEdge(){
 const axis=(negative,positive)=>{
  const mask=(1<<negative)|(1<<positive);
  const direction=(held&mask)||(pressed&mask);
  return Number(!!(direction&(1<<positive)))-Number(!!(direction&(1<<negative)));
 };
 // Encode the nine aim combinations as 1–9. Zero is an unspecified aim.
 return {aim:1+(axis(0,1)+1)+3*(axis(2,3)+1),order:++dashEdgeOrder};
}
function queuedDashAim(){
 if(!(pressed&(1<<5)))return 0;
 let latest=completedDash;
 for(const pointer of pointers.values())
  if(pointer.dash&&(!latest||pointer.dash.order>latest.order))latest=pointer.dash;
 return latest?.aim??0;
}
const tap=(...keys)=>{
 for(const key of keys){
  if(key===5)completedDash=dashEdge();
  completedPresses|=1<<key;
  pressed|=1<<key;
 }
};
function pointerPress(pointer){
 if(pointer.key===5)pointer.dash=dashEdge();
 pointer.pending|=1<<pointer.key;
 pressed|=1<<pointer.key;
}
function clearPresses(){
 pressed=completedPresses=0;
 completedDash=null;
 for(const pointer of pointers.values()){
  pointer.pending=0;
  pointer.dash=null;
 }
}
let awardToast=false,journalAwards=false;
const overlayFlags=()=>uiFlags&1?0:uiFlags;
const pauseScreenOpen=()=>(overlayFlags()&(2|8|64|128))===2;
const settingsFlag=1<<4;
const confirmSaveFlag=1<<5;
const doorReadyFlag=1<<10;
const expeditionFlag=1<<13;
const roomMapFlag=1<<17;
const forkDoorFlag=1<<18;
const blockingFlags=1|2|4|8|64|128;
const doorwayContext=flags=>flags&blockingFlags?0:flags&(doorReadyFlag|forkDoorFlag);
const replaceRunFlag=1<<19;
const routeReturnFlag=1<<25,replyChoiceFlag=1<<26;
const previousInspectionPageFlag=1<<27,nextInspectionPageFlag=1<<28;
const previousRouteRoomFlag=1<<29,nextRouteRoomFlag=1<<30;
const routeDirectionSelector='.route-browse [data-key],#touch .directions [data-key]';
const expeditionMapOpen=()=>(overlayFlags()&(128|expeditionFlag|tourFlag))===(128|expeditionFlag);
const expeditionTimelineOpen=()=>expeditionMapOpen()&&!(overlayFlags()&roomMapFlag);
const canSwitchMap=()=>(overlayFlags()&(128|tourFlag))===128;
const isMapViewAction=element=>canSwitchMap()&&element instanceof Element&&
 element.matches('.map-view-switch,#touch-inspect,#touch-use');
const isTimelineDirection=element=>expeditionTimelineOpen()&&element instanceof Element&&element.matches(routeDirectionSelector);
const timelineShortcuts=new Set(['KeyB','KeyJ','KeyP','Escape']);
const readOnlyMapOpen=()=>!!((overlayFlags()&128)&&travelDistrict<0);
const questRecordOpen=()=>(overlayFlags()&(4|64))===(4|64);
function nameTouchDirections(overlay){
 let directions;
 if((uiFlags&1)&&!(uiFlags&(settingsFlag|confirmSaveFlag))){
  directions=tr(['Poprzednia opcja','Następna opcja'],['Previous option','Next option']);
 }else if(overlay&128){
  if(overlay&roomMapFlag)directions=tr(['W górę','W dół'],['Up','Down']);
  else if(overlay&expeditionFlag)directions=tr(['Poprzedni pokój','Następny pokój'],['Previous room','Next room']);
  else if(overlay&2048)directions=tr(['Poprzedni odcinek','Następny odcinek'],['Previous leg','Next leg']);
  else directions=tr(['Poprzednia dzielnica','Następna dzielnica'],['Previous district','Next district']);
 }else if(overlay&replyChoiceFlag){
  directions=tr(['Poprzednia odpowiedź','Następna odpowiedź'],['Previous reply','Next reply']);
 }else if(overlay&8){
  directions=tr(['Poprzednia strona','Następna strona'],['Previous page','Next page']);
 }else if((overlay&64)&&!(overlay&4)){
  directions=tr(['Poprzedni wpis','Następny wpis'],['Previous entry','Next entry']);
 }else if((overlay&2)&&(overlay&tourFlag)&&!(overlay&4)){
  directions=tr(['Poprzedni pokój','Następny pokój'],['Previous room','Next room']);
 }
 if(!directions)return;
 document.querySelector('#touch [data-key="2"]').setAttribute('aria-label',directions[0]);
 document.querySelector('#touch [data-key="3"]').setAttribute('aria-label',directions[1]);
}
function translateUI(){
 document.documentElement.lang=english?'en':'pl';
 document.title='Terminal Velocity — '+tr('Pod powierzchnią','Below the surface');
 for(const b of document.querySelectorAll('[data-pl]'))b.textContent=tr(b.dataset.pl,b.dataset.en);
 for(const b of document.querySelectorAll('[data-aria-pl]'))b.setAttribute('aria-label',tr(b.dataset.ariaPl,b.dataset.ariaEn));
 for(const b of document.querySelectorAll('[data-language]'))b.setAttribute('aria-pressed',String(b.dataset.language===(english?'en':'pl')));
 document.querySelector('#hint').textContent=tr('A/D ruch · Spacja skok · X / Shift zryw · E użyj · B obejrzyj · P pauza · Tab zadania · Pauza + Tab sterowanie przeglądarki','A/D move · Space jump · X / Shift dash · E use · B inspect · P pause · Tab quests · Pause + Tab for browser controls');
 const touchUse=document.querySelector('#touch-use');
 const inMenu=!!(uiFlags&1);
 const replacingSave=!!(uiFlags&confirmSaveFlag);
 const replacingRun=!!(uiFlags&replaceRunFlag);
 const choosingProfile=!!(uiFlags&profilePickerFlag);
 const inSettings=!!(uiFlags&settingsFlag);
 const overlay=overlayFlags();
 const showingPause=pauseScreenOpen();
 const reading=!!(overlay&4);
 const choosingReply=!!(overlay&replyChoiceFlag);
 const inspecting=!!(overlay&8);
 const closingOverlay=inspecting||!!(overlay&(64|128));
 const navigationBlocked=inspecting||(reading&&!questRecordOpen());
 const hasPreviousPage=!!(overlay&previousInspectionPageFlag);
 const hasNextPage=!!(overlay&nextInspectionPageFlag);
 const expeditionMap=expeditionMapOpen();
 const showRoute=!!(overlay&roomMapFlag);
 const timeline=expeditionTimelineOpen();
 const legControls=timeline&&routeLegs!==0;
 const focusedDirection=isTimelineDirection(document.activeElement)?document.activeElement:null;
 document.querySelector('#touch').toggleAttribute('data-leg-browse',legControls);
 for(const button of document.querySelectorAll('.route-browse [data-leg-control]'))button.hidden=!legControls;
 if(legControls){
  document.querySelector('#touch [data-key="0"]').setAttribute('aria-label',tr('Poprzedni odcinek','Previous leg'));
  document.querySelector('#touch [data-key="1"]').setAttribute('aria-label',tr('Następny odcinek','Next leg'));
 }
 for(const [key,available] of [
  [0,!legControls||!!(routeLegs&1)],
  [1,!legControls||!!(routeLegs&2)],
  [2,timeline?!!(overlay&previousRouteRoomFlag):!inspecting||hasPreviousPage],
  [3,timeline?!!(overlay&nextRouteRoomFlag):!inspecting||hasNextPage],
 ]){
  for(const button of document.querySelectorAll('#touch [data-key="'+key+'"],.route-browse [data-key="'+key+'"]')){
   if(button.disabled===!available)continue;
   if(!available)for(const [id,pointer] of pointers)if(pointer.button===button)finishPointer(id,false);
   button.disabled=!available;
  }
 }
 if(focusedDirection?.disabled){
  const reverseKey=Number(focusedDirection.dataset.key)^1;
  const reverse=focusedDirection.parentElement.querySelector('[data-key="'+reverseKey+'"]');
  (reverse&&!reverse.disabled&&!reverse.hidden?reverse:canvas).focus({preventScroll:true});
 }
 const browsingQuests=!!(overlay&64)&&!reading;
 touchUse.hidden=inspecting;
 if(inSettings||closingOverlay||showingPause){
  for(const button of document.querySelectorAll('#toolbar > [data-tap="9"],#fullscreen-buttons > [data-tap="9"]'))
   button.textContent=inSettings||closingOverlay?tr('Wróć','Back'):tr('Wznów','Resume');
 }
 saveReplacement.hidden=!replacingSave;
 tourControls.hidden=!tourPicker;
 menuSelection.hidden=!inMenu||inSettings||replacingSave;
 menuSelection.querySelector('[data-tap="9"]').hidden=!choosingProfile&&!awardsOpen;
 menuSelection.querySelector('[data-tap="8"]').hidden=awardsOpen;
 document.querySelector('#open-achievements').hidden=choosingProfile||awardsOpen;
 document.querySelector('#filter-achievements').hidden=!awardsOpen;
 if(replacingSave){
  const replace=document.querySelector('#replace-save');
  replace.dataset.tap=replacingRun?'6':'8';
  replace.setAttribute('aria-label',replacingRun
   ?tr('Zastąp zapisaną rozgrywkę','Replace saved run')
   :tr('Zastąp zapisaną podróż','Replace saved journey'));
  touchUse.textContent=tr('Zastąp','Replace');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(inMenu&&!inSettings){
  touchUse.textContent=awardsOpen?tr('Wróć','Back'):tr('Wybierz','Select');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(inSettings){
  touchUse.textContent=tr('Wróć','Back');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(showingPause){
  touchUse.textContent=tr('Wznów','Resume');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(choosingReply){
  touchUse.textContent=tr('Wybierz','Select');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(reading){
  touchUse.textContent=tr('Dalej','Continue');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(browsingQuests){
  touchUse.textContent=tr('Wybierz','Select');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(uiFlags&forkDoorFlag){
  touchUse.textContent=tr('Przejście B','Passage B');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(uiFlags&routeReturnFlag){
  touchUse.textContent=tr('Teraz','Now');
  touchUse.setAttribute('aria-label',overlay&expeditionFlag
   ?tr('Wróć do obecnego pokoju','Return to current room')
   :tr('Wróć do obecnego odcinka','Return to current leg'));
 }else if(readOnlyMapOpen()){
  touchUse.textContent=tr('Wróć','Back');
  touchUse.setAttribute('aria-label',tr('Zamknij mapę','Close map'));
 }else if(travelDistrict>=0){
  touchUse.textContent=tr('Podróżuj','Travel');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else if(overlay&doorReadyFlag){
  touchUse.textContent=tr('Wejdź','Enter');
  touchUse.setAttribute('aria-label',touchUse.textContent);
 }else{
  touchUse.textContent=lastBlocked?'↵':tr('Użyj','Use');
  touchUse.setAttribute('aria-label',lastBlocked?tr('Zatwierdź','Confirm'):tr('Użyj','Use'));
 }
 const districtControl=!!(uiFlags&16384);
 // Keep the initiating button's space so the confirmation does not move under it.
 retryButton.hidden=!!(uiFlags&1)&&!districtControl&&!replacingRun;
 retryButton.disabled=!started||reading||closingOverlay;
 retryButton.style.visibility=replacingRun?'hidden':'';
 if(districtControl||replacingRun){
  retryButton.textContent=tr('Od nowa','New run');
  retryButton.setAttribute('aria-label',tr('Rozpocznij od nowa','Start a new run'));
 }else retryButton.removeAttribute('aria-label');
 for(const button of document.querySelectorAll('[data-tap="11"]')){
  button.textContent=districtControl?tr('Dzielnica','District'):tr('Mapa','Map');
  button.setAttribute('aria-label',districtControl?tr('Zmień dzielnicę','Change district'):button.textContent);
  button.disabled=!started||navigationBlocked;
 }
 for(const button of document.querySelectorAll('[data-tap="10"]'))button.disabled=!started||navigationBlocked;
 const touchInspect=document.querySelector('#touch-inspect');
 const touchControls=document.querySelector('#touch');
 if(inMenu||overlay&128||showingPause||reading||inspecting||browsingQuests){
  const browseTour=!!(overlay&tourFlag)&&!(overlay&4);
  const actionsOnly=inMenu?(inSettings||replacingSave):showingPause?!browseTour:
   (reading&&!choosingReply)||!!(overlay&roomMapFlag)||(inspecting&&!hasPreviousPage&&!hasNextPage);
  touchControls.dataset.compactControls=actionsOnly?'actions':'browse';
 }else delete touchControls.dataset.compactControls;
 nameTouchDirections(overlay);
 for(const controls of document.querySelectorAll('.route-browse'))controls.hidden=!timeline;
 const switchingMap=canSwitchMap();
 const [mapViewLabel,mapViewAction]=expeditionMap
  ?showRoute?tr(['Oś trasy','Pokaż oś trasy'],['Timeline','Show route timeline'])
            :tr(['Plan pokoju','Pokaż plan pokoju'],['Room map','Show room map'])
  :showRoute?tr(['Dzielnice','Pokaż mapę dzielnic'],['District map','Show district map'])
  :overlay&2048?tr(['Plan pokoju','Pokaż plan pokoju'],['Room map','Show room map'])
               :tr(['Trasa','Pokaż trasę podróży'],['Journey route','Show campaign route']);
 for(const button of document.querySelectorAll('.map-view-switch')){
  button.hidden=!switchingMap;
  if(!switchingMap)continue;
  button.textContent='B · '+mapViewLabel;
  button.setAttribute('aria-label',mapViewAction);
 }
 touchInspect.hidden=showingPause||(reading&&!questRecordOpen())||inSettings||!!((overlay&128)&&(overlay&tourFlag));
 let inspectLabel;
 if(replacingSave){
  touchInspect.textContent=tr('Anuluj','Cancel');
 }else if(choosingProfile){
  touchInspect.textContent=tr('Wróć','Back');
 }else if(inMenu){
  touchInspect.textContent=awardsOpen?tr('Grupa','Group'):tr('Osiągnięcia','Achievements');
  if(awardsOpen)inspectLabel=tr('Zmień grupę','Change group');
 }else if(overlay&forkDoorFlag){
  touchInspect.textContent=tr('Główne drzwi','Main door');
 }else if(questRecordOpen()){
  touchInspect.textContent=tr('Wróć','Back');
  inspectLabel=tr('Wróć do księgi zadań','Back to quest book');
 }else if(inspecting){
  touchInspect.textContent=tr('Zamknij','Close');
  inspectLabel=tr('Zamknij podgląd','Close inspection');
 }else if(browsingQuests){
  touchInspect.textContent=tr('Zakładka','Tab');
  inspectLabel=tr('Następna zakładka','Next quest tab');
 }else if(switchingMap){
  touchInspect.textContent='B · '+mapViewLabel;
  inspectLabel=mapViewAction;
 }else{
  touchInspect.textContent=awardToast?tr('Opis','Details'):tr('Obejrzyj','Inspect');
  if(awardToast)inspectLabel=tr('Opis osiągnięcia','Achievement details');
 }
 touchInspect.setAttribute('aria-label',inspectLabel||touchInspect.textContent);
 document.querySelector('#focus-pause').setAttribute('aria-pressed',String(!!(uiFlags&4096)));
 document.querySelector('#smooth-camera').setAttribute('aria-pressed',String(!!(uiFlags&smoothCameraFlag)));
 soundButton.setAttribute('aria-pressed',String(started&&!(uiFlags&soundMutedFlag)));
 if(!started)start.textContent=bootFailed?tr('Odśwież','Reload'):ready?tr('Uruchom','Start'):tr('Ładowanie…','Loading…');
 renderSaveStatus();
 renderSteamSoundSettings();
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
 const nextRouteLegs=u.routeLegs??0;
 const nextTravelDistrict=u.travelDistrict??-1;
 const travelChanged=nextTravelDistrict!==travelDistrict;
 // The game sends UI state every frame. Only a transition changes the controls.
 if(u.flags===uiFlags&&english===nextEnglish&&routeLegs===nextRouteLegs&&!travelChanged&&tourPicker===!!u.tourPicker&&awardsOpen===!!u.awardsOpen&&awardToast===!!u.awardToast&&journalAwards===!!u.journalAwards)return;
 awardToast=!!u.awardToast;
 if(tourPicker!==!!u.tourPicker||awardsOpen!==!!u.awardsOpen||journalAwards!==!!u.journalAwards)clearInput();
 journalAwards=!!u.journalAwards;
 awardsOpen=!!u.awardsOpen;
 tourPicker=!!u.tourPicker;
 const hadTimelineFocus=isTimelineDirection(document.activeElement);
 const hadMapActionFocus=isMapViewAction(document.activeElement);
 const hadSettingsFocus=settingsKeys.contains(document.activeElement);
 const hadTourFocus=tourControls.contains(document.activeElement);
 const hadMenuFocus=(uiFlags&1)&&!(uiFlags&(settingsFlag|confirmSaveFlag))&&
  isMenuSelectionControl(document.activeElement);
 const hadReplacementFocus=(uiFlags&confirmSaveFlag)&&isSaveReplacementControl(document.activeElement);
 routeLegs=nextRouteLegs;
 const previousSlot=selectedSlot;
 if(started)selectedSlot=1+((u.flags>>>20)&3);
 const inputContextFlags=blockingFlags|2048|settingsFlag|confirmSaveFlag|roomMapFlag|profilePickerFlag|tourFlag|replyChoiceFlag;
 // A doorway still reads held movement while its shutter opens.
 if((u.flags&inputContextFlags)!==(uiFlags&inputContextFlags))clearInput();
 const doorChanged=doorwayContext(u.flags)!==doorwayContext(uiFlags);
 const routeReturnChanged=!!((u.flags^uiFlags)&routeReturnFlag);
 if(doorChanged||travelChanged||routeReturnChanged){
  // Cancel only contacts for obsolete destinations; independent inputs stay active.
  for(const [id,pointer] of pointers)
   if((doorChanged&&pointer.doorConfirm)||(travelChanged&&pointer.travelConfirm)||
      (routeReturnChanged&&pointer.mapConfirm))finishPointer(id,false);
 }
 travelDistrict=nextTravelDistrict;
 lastBlocked=!!(u.flags&(blockingFlags|doorReadyFlag));
 const before=uiFlags;uiFlags=u.flags;
 if((before&255)!==(uiFlags&255))resize();
 if((before^uiFlags)&soundMutedFlag)try{localStorage.setItem(mutePreferenceKey,uiFlags&soundMutedFlag?'1':'0');}catch{}
 if(english!==nextEnglish){english=nextEnglish;try{localStorage.setItem(languageKey,english?'en':'pl');}catch{}}
 settingsKeys.hidden=!(u.flags&settingsFlag);
 if(previousSlot!==selectedSlot){
  // A temporary session or another tab's read-only session chooses only
  // where it plays now. The writer owns the slot used at the next launch.
  if(browserStorageWritable)try{localStorage.setItem(selectedSlotKey,String(selectedSlot));}catch{}
  setSaveActionError(null);
 }
 if(previousSlot!==selectedSlot||((before^uiFlags)&tourFlag)){
  refreshSaveControls();
  maybeStartAccount();
 }
 translateUI();
 if(!accounts?.active&&((hadTimelineFocus&&!expeditionTimelineOpen())||
    (hadMapActionFocus&&!canSwitchMap())||
    (hadSettingsFocus&&settingsKeys.hidden)||(hadTourFocus&&tourControls.hidden)||
    (hadMenuFocus&&menuSelection.hidden)||(hadReplacementFocus&&saveReplacement.hidden)))
  canvas.focus({preventScroll:true});
};
const composingKey=e=>e.isComposing||e.keyCode===229;
const confirmKey=e=>e.code==='Enter'||e.code==='NumpadEnter';
const keyMap={ArrowLeft:0,KeyA:0,ArrowRight:1,KeyD:1,ArrowUp:2,KeyW:2,ArrowDown:3,KeyS:3,Space:4,KeyX:5,ShiftLeft:5,ShiftRight:5,KeyE:6,KeyF:6,KeyB:7,Enter:8,NumpadEnter:8,Escape:9,KeyP:9,Tab:10,KeyJ:11,KeyM:12,KeyR:13,KeyN:14,KeyL:15,KeyU:16,Digit1:20,Digit2:21,Digit3:22,Digit4:23,Digit5:24,Digit6:25,Digit7:26,PageUp:28,PageDown:29};
function recompute(){
 held=0;
 for(const code of keyboard)held|=1<<keyMap[code];
 for(const {key} of pointers.values())if(key!==null)held|=1<<key;
}
addEventListener('keydown',e=>{
 if(importPending)return;
 const shiftKey=e.code==='ShiftLeft'||e.code==='ShiftRight';
 if(composingKey(e))return;
 const target=e.target instanceof Element?e.target:null;
 const settingsBack=e.code==='Escape'&&settingsKeys.contains(target);
 const pauseScreen=pauseScreenOpen();
 const pauseActionShortcut=pauseScreen&&
  (e.code==='Escape'||e.code==='KeyP'||(e.code==='KeyR'&&!retryButton.disabled))&&
  target?.matches('#toolbar button[data-tap],#fullscreen-buttons button[data-tap],#touch-use');
 const pageKey=e.code==='PageUp'||e.code==='PageDown';
 if(pageKey&&!tourPicker&&!awardsOpen&&!journalAwards)return;
 const menuControl=(uiFlags&1)&&!(uiFlags&(settingsFlag|confirmSaveFlag))&&isMenuSelectionControl(target);
 const menuNavigation=menuControl&&(e.code==='ArrowUp'||e.code==='ArrowDown'||
  (e.code==='Escape'&&((uiFlags&profilePickerFlag)||awardsOpen))||(awardsOpen&&(e.code==='KeyB'||e.code==='ArrowLeft'||e.code==='ArrowRight'||pageKey)));
 const cancelReplacement=e.code==='Escape'&&(uiFlags&confirmSaveFlag)&&isSaveReplacementControl(target);
 const tourNavigation=tourPicker&&(tourControls.contains(target)||menuControl)&&
  (pageKey||e.code.startsWith('Arrow')||e.code==='Escape'||e.code==='KeyN');
 if(e.code==='Tab'&&(e.shiftKey||e.target!==canvas||pauseScreen))return;
 const timelineDirection=isTimelineDirection(target);
 const mapAction=isMapViewAction(target);
 const mapShortcut=(timelineDirection||mapAction)&&timelineShortcuts.has(e.code);
 const timelineNavigation=(timelineDirection||(mapAction&&!(overlayFlags()&roomMapFlag)))&&e.code.startsWith('Arrow');
 if(target?.closest('button,a,input,select,textarea,[contenteditable]')&&
    !pauseActionShortcut&&!settingsBack&&!menuNavigation&&!cancelReplacement&&!tourNavigation&&
    !mapShortcut&&!timelineNavigation)return;
 if(accounts?.active||!started||keyMap[e.code]===undefined||e.ctrlKey||e.metaKey||e.altKey)return;
 e.preventDefault();
 if(e.repeat)return;
 // Shift is an immediate movement input. When paused or in menus it stays a browser modifier.
 if(shiftKey&&((uiFlags&blockingFlags)||keyboard.has('ShiftLeft')||keyboard.has('ShiftRight')))return;
 if(pauseActionShortcut)canvas.focus({preventScroll:true});
 tap(keyMap[e.code]);
 keyboard.add(e.code);
 recompute();
});
canvas.addEventListener('blur',clearInput);
addEventListener('keyup',e=>{
 if(keyboard.delete(e.code))recompute();
});
const horizontalTouchControls='#touch .directions :is(button[data-key="0"], button[data-key="1"])';
function releaseHighlight(button){
 if(button&&![...pointers.values()].some(pointer=>pointer.button===button))button.classList.remove('held');
}
function recomputePresses(){
 pressed=completedPresses;
 for(const pointer of pointers.values())pressed|=pointer.pending;
}
function finishPointer(id,completed,event){
 const pointer=pointers.get(id);
 if(!pointer)return;
 pointers.delete(id);
 if(completed){
  if(pointer.dash&&(!completedDash||pointer.dash.order>completedDash.order))completedDash=pointer.dash;
  completedPresses|=pointer.pending;
  if(pointer.releaseKey!==null&&!accounts?.active&&!importPending&&!pointer.button.disabled&&
     pointer.button.contains(document.elementFromPoint(event.clientX,event.clientY)))
   completedPresses|=1<<pointer.releaseKey;
 }
 // A cancelled contact must not erase other inputs of the same action.
 recomputePresses();
 releaseHighlight(pointer.button);
 recompute();
}
matchMedia('(orientation:landscape)').addEventListener('change',()=>{
 // The old contact still targets its original button after the pad moves.
 for(const [id,pointer] of pointers)if(pointer.touch)finishPointer(id,false);
});
for(const button of document.querySelectorAll('[data-key]')){
 const action=()=>{
  if(uiFlags&forkDoorFlag){
   if(button.id==='touch-use')return 6;
   if(button.id==='touch-inspect')return 8;
  }
  if(button.id==='touch-inspect'){
   if((uiFlags&(confirmSaveFlag|profilePickerFlag))||questRecordOpen())return 9;
   if((uiFlags&1)&&!(uiFlags&settingsFlag)&&!awardsOpen)return 30;
  }
  if(button.id==='touch-use'&&awardsOpen)return 9;
  if(button.id!=='touch-use'||!lastBlocked)return +button.dataset.key;
  if(uiFlags&routeReturnFlag)return 8;
  if(readOnlyMapOpen())return 11;
  return (overlayFlags()&64)||(uiFlags&replaceRunFlag)?6:8;
 };
 button.addEventListener('click',e=>{
  // Pointer input is handled by the contact; this also serves keyboard clicks.
  if(e.detail!==0||button.disabled||accounts?.active||!started||importPending)return;
  if(!isTimelineDirection(button))canvas.focus();
  tap(action());
 });
 // Pointer events own game controls. Suppress the touch compatibility click,
 // which the browser can otherwise retarget to a footer link after resize.
 button.addEventListener('touchstart',e=>{if(e.cancelable)e.preventDefault();},{passive:false});
 button.addEventListener('pointerdown',e=>{
  if(e.button!==0||button.disabled||accounts?.active||!started||importPending)return;
  e.preventDefault();
  canvas.focus({preventScroll:true});
  const key=action();
  button.setPointerCapture(e.pointerId);
  const touch=e.pointerType==='touch';
  const canSlide=touch&&(button.matches(horizontalTouchControls)||(!lastBlocked&&button.closest('#touch .directions')));
  const confirmingReply=(overlayFlags()&(replyChoiceFlag|2))===replyChoiceFlag;
  const doorConfirm=!!doorwayContext(uiFlags)&&(button.id==='touch-use'||
   (button.id==='touch-inspect'&&!!(uiFlags&forkDoorFlag)));
  const travelConfirm=button.id==='touch-use'&&travelDistrict>=0;
  const mapConfirm=(button.id==='touch-use'&&readOnlyMapOpen())||
   (expeditionTimelineOpen()&&button.matches('.route-browse [data-key="8"]'));
  const onRelease=doorConfirm||travelConfirm||mapConfirm||
   (button.id==='touch-use'&&(confirmingReply||!!(uiFlags&confirmSaveFlag)));
  const pointer={key:onRelease?null:key,releaseKey:onRelease?key:null,button,touch,canSlide,
   x:e.clientX,y:e.clientY,doorConfirm,travelConfirm,mapConfirm,pending:0,dash:null};
  pointers.set(e.pointerId,pointer);
  button.classList.add('held');
  if(!onRelease)pointerPress(pointer);
  recompute();
 });
 button.addEventListener('pointermove',e=>{
  const pointer=pointers.get(e.pointerId);
  if(!pointer)return;
  // Mouse chords report the primary release as a move until every button is up.
  if(e.pointerType==='mouse'&&!(e.buttons&1)){
   finishPointer(e.pointerId,true,e);
   if(button.hasPointerCapture(e.pointerId))button.releasePointerCapture(e.pointerId);
   return;
  }
  pointer.x=e.clientX;pointer.y=e.clientY;
  if(!pointer.canSlide)return;
  // Keep an initial Up or Down press until the thumb leaves its button.
  // Sliding across a vertical button must never trigger a new jump or fall.
  const hit=document.elementFromPoint(e.clientX,e.clientY);
  const candidate=hit?.closest(horizontalTouchControls)??(pointer.button===button&&button.contains(hit)?button:null);
  const next=candidate&&!candidate.disabled?candidate:null;
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
  button.addEventListener(type,e=>finishPointer(e.pointerId,type==='pointerup',e));
}
for(const b of document.querySelectorAll('[data-tap]'))b.onclick=event=>{
 if(!started||accounts?.active||importPending)return;
 const key=+b.dataset.tap;
 // Keep keyboard focus on adjustments that leave the Settings panel open.
 const adjustsSetting=settingsKeys.contains(b)&&key!==25;
 const pagesTour=tourPicker&&(key===28||key===29);
 if(event.detail!==0||(!adjustsSetting&&!pagesTour))canvas.focus({preventScroll:true});
 if(b.id==='open-achievements')tap(key,8);
 else tap(key);
};
function clearInput(){keyboard.clear();pointers.clear();canvasPress=null;held=0;clearPresses();document.querySelectorAll('.held').forEach(e=>e.classList.remove('held'));}
function updateBrowserSaveStatus(status){
 browserSaveStatus=status;
 renderSaveStatus();
}
function runStorageOperation(start,onComplete){
 let completed=false;
 const finish=error=>{
  if(completed)return;
  completed=true;
  onComplete(error);
 };
 try{start(finish);}catch(error){
  if(completed)throw error;
  finish(error);
 }
}
function flushBrowserStorage(){
 // A failed restore or a second tab cannot safely replace the IndexedDB contents.
 if(!browserStorageWritable)return;
 if(syncing){storageFlushQueued=true;return;}
 if(!storageStatusTimers){
  // Avoid announcing fast autosaves. Keep the status across queued writes
  // until the latest progress has reached IndexedDB.
  storageStatusTimers=[
   setTimeout(()=>updateBrowserSaveStatus(['Zapisywanie…','Saving…']),250),
   setTimeout(()=>updateBrowserSaveStatus(['Zapis trwa długo — pobierz kopię','Save delayed — download a copy']),6000),
  ];
 }
 syncing=true;
 runStorageOperation(done=>Module.FS.syncfs(false,done),error=>{
  syncing=false;
  if(!bootFailed&&storageFlushQueued){
   storageFlushQueued=false;
   flushBrowserStorage();
   return;
  }
  storageFlushQueued=false;
  storageStatusTimers.forEach(timer=>clearTimeout(timer));
  storageStatusTimers=null;
  if(bootFailed)return;
  if(!error)try{localStorage.removeItem(pendingLanguageKey);}catch{}
  updateBrowserSaveStatus(error
   ?['Błąd zapisu — pobierz kopię','Save failed — download a copy']
   :['Zapis w tej przeglądarce ✓','Saved in this browser ✓']);
 });
}
window.tvSaveReady=()=>{
 refreshUnreadableSaveControls();
 flushBrowserStorage();
};
function syncSave(){
 if(!started||inTour()||importPending)return;
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
 if(started&&!accounts?.active&&!importPending)Module._web_tick(0,0,0,cols,rows,0);
 syncSave();
}
addEventListener('blur',()=>updateFocus(false));
addEventListener('focus',()=>updateFocus(true));
document.addEventListener('visibilitychange',()=>updateFocus(document.hasFocus()));
function resize(){
 const width=stage.clientWidth,height=stage.clientHeight;
 if(width!==stageWidth||height!==stageHeight)canvasFit=null;
 stageWidth=width;stageHeight=height;
 // Capture can outlive a layout change even after the button moves away.
 for(const [id,pointer] of pointers)
  if(pointer.touch&&pointer.button&&!pointer.button.contains(document.elementFromPoint(pointer.x,pointer.y)))
   finishPointer(id,false);
 const landscape=matchMedia('(orientation:landscape) and (max-height:500px)').matches;
 const compact=mobile()&&(!!(uiFlags&255)||(landscape&&expanded()));
 const portraitPhone=matchMedia('(pointer:coarse) and (orientation:portrait) and (max-width:480px)').matches;
 if(compact){
  cols=landscape?60:38;
  rows=landscape?20:28;
 }else{
  // Keep a 56-column horizon and grow it with the screen width.
  cols=portraitPhone?Math.max(56,Math.ceil(width/6.5)):Math.max(76,Math.min(160,Math.floor(width/10)));
  rows=Math.max(portraitPhone?20:24,Math.min(48,Math.floor(height/(width/cols*2))));
 }
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
const testSteamSound=document.querySelector('#test-steam-sound');
let achievementSoundBundled=false;
function renderSteamSoundSettings(){
 testSteamSound.textContent=tr('Test dźwięku osiągnięcia','Test achievement sound');
 testSteamSound.hidden=!achievementSoundBundled;
 testSteamSound.disabled=!started||Boolean(uiFlags&soundMutedFlag);
}
testSteamSound.onclick=()=>{
 Module._web_test_achievement_sound();
 canvas.focus({preventScroll:true});
};
const retryButton=document.querySelector('#toolbar [data-tap="13"]');
const soundButton=document.querySelector('#sound');
const soundAnchor=soundButton.nextElementSibling;
const toolbar=document.querySelector('#toolbar'),fullscreenTools=document.querySelector('#fullscreen-tools');
const fullscreenButtons=document.querySelector('#fullscreen-buttons');
const saveControls=document.querySelector('#save-controls'),fullscreenButton=document.querySelector('#fullscreen');
const leaveFullscreen=document.querySelector('#leave-fullscreen');
const pageControls=document.querySelectorAll('body > header, body > footer');
const expanded=()=>!!document.fullscreenElement||playfield.classList.contains('expanded');
function fullscreenLayout(){
 const active=expanded();
 const fallback=playfield.classList.contains('expanded');
 const settingsFocus=settingsKeys.contains(document.activeElement)?document.activeElement:null;
 for(const controls of pageControls)controls.inert=fallback;
 if(active){
  fullscreenButtons.prepend(start);
  document.querySelector('#account-fullscreen').before(retryButton,soundButton);
  fullscreenButtons.append(saveControls,languageToggle);
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
 if(settingsFocus&&!settingsKeys.hidden)settingsFocus.focus({preventScroll:true});
 else if(!accounts?.active||(field!==accounts.name&&field!==accounts.pass))canvas.focus();
 fullscreenButton.setAttribute('aria-pressed',String(active));
}
let fullscreenTransition=false;
async function toggleFullscreen(){
 if(fullscreenTransition)return;
 fullscreenTransition=true;
 try {
  if(document.fullscreenElement){await document.exitFullscreen();return;}
  if(playfield.classList.contains('expanded')){playfield.classList.remove('expanded');fullscreenLayout();return;}
  try {await playfield.requestFullscreen();}
  catch(error){
   // Embedded browsers and some mobile browsers refuse the native API. The
   // game still occupies the entire viewport, with an explicit way back out.
   console.info('Native fullscreen unavailable; using game viewport',error.name);
   playfield.classList.add('expanded');fullscreenLayout();
  }
 }finally{fullscreenTransition=false;}
}
addEventListener('keydown',e=>{
 if(composingKey(e))return;
 if((confirmKey(e)&&e.altKey)||(e.code==='Escape'&&expanded()&&!accounts?.active)){
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
let importPending=false,importPicker=null;

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

function readAccountSave(){
 if(Module._web_save())return readManualBackup();
 if(!Module._web_primary_usable_for_account())throw Error('save_write_failed');
 if(!Module._web_export_recovery())throw Error('save_write_failed');
 const recovery='/tmp/terminal-velocity-export.iceberg';
 try{return readManualBackup();}
 finally{Module.FS.unlink(recovery);}
}
function readManualBackup(){
 const path='/tmp/terminal-velocity-manual.iceberg';
 const fs=Module.FS;
 try{
  if(!Module._web_export_manual())throw Error('save_write_failed');
  return fs.readFile(path);
 }finally{if(fs.analyzePath(path).exists)fs.unlink(path);}
}
function downloadSave(event,suffix,read){
 try {
  const slot=selectedSlot===1?'':'-slot-'+selectedSlot;
  const url=URL.createObjectURL(new Blob([read()],{type:'application/octet-stream'}));
  const link=document.createElement('a');
  link.href=url;link.download='terminal-velocity'+slot+suffix+'.iceberg';link.click();
  setTimeout(()=>URL.revokeObjectURL(url),1000);
  setSaveActionError(null);
 } catch {
  setSaveActionError(['Brak zapisu do pobrania','No save to download']);
 } finally {
  if(event.detail!==0)canvas.focus({preventScroll:true});
 }
}
exportSave.onclick=event=>downloadSave(event,'',()=>{
 Module._web_save();
 return readManualBackup();
});
exportPreviousSave.onclick=event=>downloadSave(event,'-previous',()=>Module.FS.readFile(previousImportPath()));
unreadableSaveButtons.forEach((button,i)=>{
 button.onclick=event=>{
  const name=unreadableSaveNames[i];
  if(name)downloadSave(event,i===0?'-unreadable':'-unreadable-backup',()=>Module.FS.readFile(saveDirectory+'/'+name));
 };
});
function finishImport(returnToPicker=false){
 importPending=false;
 refreshSaveControls();
 clearInput();
 last=performance.now();
 const focusTarget=returnToPicker&&!importSave.disabled?importSave:canvas;
 if(!accounts?.active)focusTarget.focus({preventScroll:true});
}
async function readImportFile(file){
 let deadline;
 try {
  return await Promise.race([file.arrayBuffer(),new Promise((_,reject)=>{
   deadline=setTimeout(()=>reject(new Error('Save read timed out')),6000);
  })]);
 }finally{clearTimeout(deadline);}
}
function validateImportSave(fs,bytes){
 const probe='/tmp/terminal-velocity-import-check.iceberg';
 try {
  fs.writeFile(probe,bytes);
  return Module._web_validate_import_save();
 } finally {if(fs.analyzePath(probe).exists)fs.unlink(probe);}
}
function applyManualImport(fs,previous){
 const backup=previousImportPath(),retained=backup+'.previous';
 const hadBackup=fs.analyzePath(backup).exists;
 // Hold the older download until both its replacement and the import succeed.
 if(hadBackup)fs.rename(backup,retained);
 let result;
 try{
  fs.writeFile(backup,previous);
  result=Module._web_import_save();
  return result;
 }finally{
  if(result===1){
   if(hadBackup)fs.unlink(retained);
  }else if(hadBackup)fs.rename(retained,backup);
  else if(fs.analyzePath(backup).exists)fs.unlink(backup);
 }
}
importSave.onclick=event=>{
 const keyboard=event.detail===0&&document.activeElement===importSave;
 importPicker={slot:selectedSlot,keyboard};
 clearInput();
 if(!keyboard)canvas.focus({preventScroll:true});
 saveFile.click();
};
saveFile.oncancel=()=>{
 if(importPending)return;
 const returnToPicker=importPicker?.keyboard;
 importPicker=null;
 finishImport(returnToPicker);
};
saveFile.onchange=async()=>{
 const file=saveFile.files[0];
 saveFile.value='';
 const picker=importPicker;
 importPicker=null;
 // Only an actual selection starts an import. A picker dismissal may never
 // reach the page, so opening the picker cannot suspend the game loop.
 if(!file||importPending||inTour())return;
 const returnToPicker=picker?.keyboard;
 if(picker&&picker.slot!==selectedSlot){
  setSaveActionError(['Zmieniono slot zapisu — wybierz plik ponownie','Save slot changed — choose the file again']);
  finishImport(returnToPicker);
  return;
 }
 let loaded=false;
 importPending=true;
 importSave.disabled=true;
 setSaveActionError(null);
 clearInput();
 try {
  if(!file.size||file.size>Module._web_import_size_limit()) {
   setSaveActionError(['Nieprawidłowy plik zapisu','Invalid save file']);
   return;
  }
  let data;
  try{data=await readImportFile(file);}
  catch{
   setSaveActionError(['Nie można odczytać zapisu — spróbuj ponownie','Could not read save file — try again']);
   return;
  }
  const bytes=new Uint8Array(data);
  if(accountSlot()&&(accountBooting||accounts?.active||accounts?.busy||accounts?.mode==='choose')){
   setSaveActionError(['Zakończ obsługę konta przed wczytaniem zapisu','Finish the account action before importing a save']);
   return;
  }
  const fs=Module.FS;
  if(!validateImportSave(fs,bytes)){
   setSaveActionError(['Nieprawidłowy plik zapisu','Invalid save file']);
   return;
  }
  const question=accountSlot()&&accounts?.user
   ?tr('Zastąpić obecny zapis? Nowy zapis będzie synchronizowany z kontem.','Replace your current save? The imported save will sync to your account.')
   :tr('Zastąpić obecny zapis w tej przeglądarce?','Replace the current save in this browser?');
  if(!window.confirm(question))return;
  if(!Module._web_save()){
   setSaveActionError(['Błąd zapisu — pobierz kopię','Save failed — download a copy']);
   return;
  }
  const previous=readManualBackup();
  const incoming=saveDirectory+'/import.iceberg';
  try {
   fs.writeFile(incoming,bytes);
   const imported=applyManualImport(fs,previous);
   if(imported!==1){
    setSaveActionError(imported===0
     ?['Nieprawidłowy plik zapisu','Invalid save file']
     :['Nie udało się zapisać wczytywanej gry','Could not write the imported save']);
    return;
   }
  } finally {if(fs.analyzePath(incoming).exists)fs.unlink(incoming);}
  exportPreviousSave.hidden=false;
  Module._web_enter_campaign();
  loaded=true;
  flushBrowserStorage();
 } catch {
  setSaveActionError(['Nie udało się wczytać zapisu','Could not import save']);
 } finally {
  finishImport(returnToPicker&&!loaded);
 }
};
function frame(now){
 const elapsed=Math.max(0,(now-last)/1000);
 const stalled=elapsed>.5;
 // Resume with a normal step after an isolated stall so rapid motion stays visible.
 const dt=stalled&&!frameWasStalled?1/60:Math.min(.1,elapsed);
 frameWasStalled=stalled;
 if(window.tvParty?.active){window.tvParty.tick(held,pressed,cols,rows,english,focused);clearPresses();}
 else if(started&&accounts?.active)accounts.draw();
 else if(started&&!importPending){
  Module._web_profile_action_busy(accountBooting||accounts?.busy?1:0);
  Module._web_tick(dt,held,pressed,cols,rows,focused?1:0,queuedDashAim());
  clearPresses();
 }
 last=now;
 requestAnimationFrame(frame);
}
function stopWatchingEngine(){
 clearTimeout(engineLoadTimer);
 engineLoadTimer=null;
}
function watchEngineLoad(){
 stopWatchingEngine();
 engineLoadTimer=setTimeout(()=>failStartup(new Error('Game asset loading stalled')),30000);
}
function failStartup(error){
 if(bootFailed)return;
 stopWatchingEngine();
 if(error)console.error(error);
 bootFailed=true;started=false;ready=false;
 clearInput();
 start.hidden=false;start.disabled=false;
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
 if(bootFailed){location.reload();return;}
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
  translateUI();
  refreshSaveControls();
  last=performance.now();canvas.focus();requestAnimationFrame(frame);
 } catch(error) {
  failStartup(error);
 }
};
function canvasConfirmContext(){
 if(tourPicker)return profilePickerFlag;
 const overlay=overlayFlags();
 if((overlay&(replyChoiceFlag|2))===replyChoiceFlag)return replyChoiceFlag;
 if(overlay&4)return 4;
 if(expeditionTimelineOpen())return expeditionFlag;
 if(overlay&(8|64|128))return 0;
 return (overlay&2)||(overlay&doorReadyFlag);
}
function canvasCell(x,y){
 const box=canvas.getBoundingClientRect();
 return {
  column:Math.floor((x-box.left)*canvasFit.w/box.width),
  row:Math.floor((y-box.top)*canvasFit.h/box.height)
 };
}
function canvasTourChoice(x,y){
 if(!tourRows||!canvasFit)return -1;
 const {column,row}=canvasCell(x,y);
 const offset=row-tourRows.row;
 return offset>=0&&offset<tourRows.count&&column>=tourRows.left&&column<tourRows.right
  ?tourRows.first+offset:-1;
}
function canvasExpeditionRoom(x,y){
 if(!expeditionNodes||!canvasFit)return -1;
 const {column,row}=canvasCell(x,y);
 const nodes=expeditionNodes,offset=column-nodes.left;
 if(row!==nodes.row||offset<0||offset%nodes.stride>=nodes.width)return -1;
 const room=Math.floor(offset/nodes.stride);
 return room<nodes.count?nodes.first+room:-1;
}
function canvasReplyChoice(x,y){
 if(replyFirstRow<0||!canvasFit)return -1;
 const cell=canvasCell(x,y);
 const column=cell.column,row=cell.row-replyFirstRow;
 if(column<3||column>=canvasFit.w-2||row<0||row>=4)return -1;
 const rowStart=(replyFirstRow+row)*canvasFit.w*4;
 // The canvas keeps a copy of the current glyphs, including wrapped labels.
 let last=canvasFit.w-3;
 while(last>=5&&previous[rowStart+last*4]===32)last--;
 if(last<5||column>last||previous.length<rowStart+canvasFit.w*4)return -1;
 return Math.floor(row/2);
}
canvas.addEventListener('pointerdown',event=>{
 if(!event.isPrimary||event.button!==0||accounts?.active||importPending)return;
 const context=canvasConfirmContext();
 const choice=context===replyChoiceFlag?canvasReplyChoice(event.clientX,event.clientY):
  context===profilePickerFlag?canvasTourChoice(event.clientX,event.clientY):
  context===expeditionFlag?canvasExpeditionRoom(event.clientX,event.clientY):-1;
 canvasPress={id:event.pointerId,context,choice,x:event.clientX,y:event.clientY};
});
addEventListener('pointermove',event=>{
 const press=canvasPress;
 if(press?.id===event.pointerId&&Math.hypot(event.clientX-press.x,event.clientY-press.y)>8)canvasPress=null;
});
canvas.addEventListener('pointercancel',event=>{
 if(canvasPress?.id===event.pointerId)canvasPress=null;
});
canvas.onclick=event=>{
 const press=canvasPress;
 canvasPress=null;
 if(accounts?.active||importPending)return;
 const context=canvasConfirmContext();
 // Only a click from the same context can start or confirm.
 if(event.detail!==0&&(!press||press.context!==context||
    (event.pointerId!==undefined&&press.id!==event.pointerId)))return;
 if(!started){start.click();return;}
 if(context===profilePickerFlag){
  const choice=canvasTourChoice(event.clientX,event.clientY);
  if(choice>=0&&press?.choice===choice)Module._web_select_tour_stop(choice);
  return;
 }
 if(context===expeditionFlag){
  const room=canvasExpeditionRoom(event.clientX,event.clientY);
  if(room>=0&&press?.choice===room)Module._web_select_expedition_room(room);
  return;
 }
 if(context===replyChoiceFlag){
  const choice=canvasReplyChoice(event.clientX,event.clientY);
  if(choice>=0&&press?.choice===choice)tap(17+choice);
  return;
 }
 if(context)tap(8);
};
addEventListener('keydown',e=>{
 if(!started&&confirmKey(e)&&!composingKey(e)&&!e.defaultPrevented&&!e.ctrlKey&&!e.metaKey&&!e.altKey&&
    (e.target===canvas||e.target===document.body))start.click();
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
  stopWatchingEngine();
  if(bootFailed)return;
  try {
   const idbfs=Module.IDBFS,reconcile=idbfs.reconcile;
   // Reconciliation runs from a cursor event, outside the initial syncfs call.
   // Starting its write transaction can throw if the connection is closing.
   if(typeof reconcile==='function')idbfs.reconcile=(local,remote,done)=>
    runStorageOperation(finish=>reconcile.call(idbfs,local,remote,finish),done);
   achievementSoundBundled=Module.FS.analyzePath('/steam-sounds/deck_ui_achievement_toast.wav').exists;
   Module.FS.mkdir(persistentSaveDirectory);
   Module.FS.mount(idbfs,{},persistentSaveDirectory);
   restoreBrowserStorage().catch(failStartup);
  }catch(error){failStartup(error);}
 },
 setStatus(){
  if(engineLoadTimer!==null)watchEngineLoad();
  if(!started)drawBoot();
 },
 onAbort:failStartup
};
const accounts=typeof window.TVAccounts==='function'?new window.TVAccounts({
 english:()=>english,ready:()=>accountSlot()&&!accountBooting&&!importPending,frame:window.tvFrame,
 refreshLayout:()=>{clearInput();resize();},
 needsRedraw:()=>previous.length===0,flush:flushBrowserStorage,
 currentSave:()=>new TextDecoder().decode(readAccountSave()),
 recoveryCopy:()=>unreadableSaveNames[0]
}):null;
for(const button of document.querySelectorAll('#account,#account-fullscreen')){
 button.hidden=true;
 button.onclick=()=>{if(!accountBooting)accounts?.open();};
}
setInterval(syncSave,5000);
if(accounts)setInterval(()=>{if(accountSlot()&&!accountBooting&&!importPending)accounts.sync();},15000);

// The live entry names its versioned assets in <base>. Check it while the
// start screen is idle so a returning player does not launch an old build.
const entryRelease=page=>/^\.\/releases\/([a-f0-9]{12})\/$/.exec(
 page.querySelector('base[href]')?.getAttribute('href')||'')?.[1];
const loadedRelease=entryRelease(document);
let checkingRelease=false;
async function refreshIdleEntry(){
 if(!loadedRelease||started||checkingRelease||document.hidden)return;
 checkingRelease=true;
 const controller=new AbortController();
 const deadline=setTimeout(()=>controller.abort(),2500);
 try{
  const response=await fetch(location.href,{cache:'no-store',mode:'same-origin',redirect:'error',signal:controller.signal});
  if(!response.ok)return;
  const entry=new DOMParser().parseFromString(await response.text(),'text/html');
  const latest=entryRelease(entry);
  // A pending network response must never reload a game that has started.
  if(latest&&latest!==loadedRelease&&!started&&!document.hidden)location.reload();
 }catch{}finally{
  clearTimeout(deadline);
  checkingRelease=false;
 }
}
if(loadedRelease){
 addEventListener('focus',refreshIdleEntry);
 addEventListener('pageshow',refreshIdleEntry);
 document.addEventListener('visibilitychange',refreshIdleEntry);
}

translateUI();resize();watchEngineLoad();

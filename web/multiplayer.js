/* All multiplayer setup, errors and room codes are drawn inside the game canvas. */
(()=>{
 const canvas=document.getElementById('game');let socket=null,setup=null,mask=0,edges=0,lastSend=0,error='',room='',mode='race',token='',lastFrame=null;
 const api=window.tvParty={active:false};
 const english=()=>document.documentElement.lang==='en';
 const text=(pl,en)=>english()?en:pl;
 function panel(lines){
  if(!window.tvFrame)return;
  const w=76,h=34,data=new Uint32Array(w*h*4);
  for(let i=0;i<w*h;i++){data[i*4]=32;data[i*4+1]=0xd5e8ed;data[i*4+2]=0x101923;}
  lines.forEach((line,n)=>Array.from(line).slice(0,70).forEach((ch,x)=>{let at=((6+n*2)*w+3+x)*4;data[at]=ch.codePointAt(0);data[at+1]=n?0xd5e8ed:0xffd68a;}));
  window.tvFrame(data,w,h);
 }
 function showSetup(){panel([text('GRA WIELOOSOBOWA','MULTIPLAYER'),text('Wpisz sześciocyfrowy kod pokoju:','Enter the six-digit room code:'),room.padEnd(6,'_'),text('↑↓: wyścig / współpraca','↑↓: race / co-op'),mode==='race'?text('WYŚCIG','RACE'):text('WSPÓŁPRACA','CO-OP'),text('ENTER: dołącz · ESC: wróć','ENTER: join · ESC: back'),error,'1       2       3       4       5','','6       7       8       9       0','','⌫       TRYB / MODE          ENTER',text('Wróć','Back')]);}
 function leave(){api.active=false;setup=null;mask=edges=0;socket?.close();socket=null;canvas.focus();}
 function connect(code,reconnect=false){
  setup=null;error='';room=code;api.active=true;mask=edges=0;if(!reconnect)token='';
  panel([text('Łączenie…','Connecting…'),text('M: wróć do gry jednoosobowej','M: return to single player')]);
  const endpoint=document.querySelector('meta[name="tv-multiplayer-url"]')?.content||'wss://het.aridlin.pl/terminal-velocity/multiplayer';
  socket=new WebSocket(endpoint);socket.binaryType='arraybuffer';
  socket.onopen=()=>socket.send(`JOIN ${code} ${mode} 1 ${reconnect&&token?token:"-"}`);
  socket.onmessage=event=>{
   if(typeof event.data==='string'){
    const f=event.data.split(' ');
    if(f[0]==='WELCOME'){room=f[1];token=f[3];lastSend=0;}
    else if(f[0]==='ERROR'){token='';error=event.data.slice(6);panel([text('Nie można dołączyć','Unable to join'),error,text('M: wróć','M: return')]);}
    return;
   }
   if(event.data.byteLength<8||event.data.byteLength>153608)return;
   const view=new DataView(event.data),w=view.getUint32(0,true),encodedRows=view.getUint32(4,true),h=encodedRows&0x7fffffff;
   if(w<34||w>160||h<24||h>60)return;
   const data=new Uint32Array(w*h*4);
   if(encodedRows>>>31){
    let at=8,cell=0;
    while(at<view.byteLength){
     if(at+18>view.byteLength)return;const run=view.getUint16(at,true);at+=2;
     if(!run||cell+run>w*h)return;
     for(let n=0;n<run;n++)for(let k=0;k<4;k++)data[(cell+n)*4+k]=view.getUint32(at+k*4,true);
     cell+=run;at+=16;
    }
    if(cell!==w*h)return;
   }else{if(event.data.byteLength!==8+w*h*16)return;for(let i=0;i<data.length;i++)data[i]=view.getUint32(8+i*4,true);}
   const flags=data[3];data[3]&=1;
   window.Module?._web_party_audio?.((flags>>>1)&511,flags>>>24,document.hasFocus()?1:0);
   lastFrame={data,w,h};window.tvFrame(data,w,h);
  };
  socket.onerror=()=>{error=text('Serwer jest niedostępny. M: wróć.','Server unavailable. M: return.');panel([text('MULTIPLAYER','MULTIPLAYER'),error]);};
  socket.onclose=()=>{if(api.active&&!setup){panel([text('Połączenie utracone. Ponowne łączenie…','Connection lost. Reconnecting…'),text('M: wróć do gry','M: return to game')]);if(token)setTimeout(()=>{if(api.active&&!setup)connect(room,true);},1500);}};
  canvas.focus();
 }
 api.openSetup=()=>{api.active=true;setup=true;room='';mode='race';error='';showSetup();canvas.focus();};
 api.tick=(held,pressed,cols,rows,isEnglish,focused)=>{
  if(!api.active)return;
  if(setup){showSetup();return;}
  if(socket?.readyState!==WebSocket.OPEN)return;
  if(pressed&(1<<16))window.Module?._web_party_mute?.();
  edges|=pressed;const now=performance.now();
  if(now-lastSend<16)return;
  // Existing touch buttons and keyboard use the same action bits as native input.
  socket.send(`I ${focused?((held|mask|edges)&0x3ffff):0} ${Math.max(34,Math.min(160,cols))} ${Math.max(24,Math.min(60,rows))} ${isEnglish?1:0}`);edges=0;lastSend=now;
 };
 const keys={KeyA:0,ArrowLeft:0,KeyD:1,ArrowRight:1,KeyW:2,ArrowUp:2,KeyS:3,ArrowDown:3,Space:4,KeyX:5,ShiftLeft:5,ShiftRight:5,KeyE:6,KeyB:7,Enter:8,KeyP:9,Tab:10,KeyJ:11,KeyR:13,KeyU:16};
 document.addEventListener('keydown',event=>{
  if(!api.active)return;
  if(event.code==='KeyM'||event.code==='Escape'){event.preventDefault();event.stopImmediatePropagation();leave();return;}
  if(setup){
   event.preventDefault();event.stopImmediatePropagation();
   if(/^Digit[0-9]$/.test(event.code)&&room.length<6)room+=event.code.slice(-1);
   if(event.code==='Backspace')room=room.slice(0,-1);
   if(event.code==='ArrowUp'||event.code==='ArrowDown')mode=mode==='race'?'coop':'race';
   if(event.code==='Enter'&&room.length===6)connect(room);
   showSetup();return;
  }
  if(keys[event.code]!==undefined){event.preventDefault();event.stopImmediatePropagation();const bit=1<<keys[event.code];mask|=bit;if(!event.repeat){edges|=bit;if(event.code==='KeyU')window.Module?._web_party_mute?.();}}
 },true);
 document.addEventListener('keyup',event=>{if(api.active&&keys[event.code]!==undefined){mask&=~(1<<keys[event.code]);event.preventDefault();event.stopImmediatePropagation();}},true);
 window.addEventListener('blur',()=>{mask=edges=0;if(api.active)window.Module?._web_party_audio?.(0,0,0);});
 canvas.addEventListener('pointerup',event=>{
  if(!api.active)return;
  const rect=canvas.getBoundingClientRect();const x=(event.clientX-rect.left)/rect.width*76,y=(event.clientY-rect.top)/rect.height*34;
  if(!setup){if(y<4)edges|=256;return;}
  event.preventDefault();event.stopImmediatePropagation();
  if(y>=19&&y<27&&x>=2&&x<42){const group=y<23?0:1;const key=Math.floor((x-2)/8);if(key<5&&room.length<6)room+=String((group*5+key+1)%10);}
  else if(y>=27&&y<30){if(x<13)room=room.slice(0,-1);else if(x<38)mode=mode==='race'?'coop':'race';else if(room.length===6)connect(room);}
  else if(y>=30)leave();
  else if(y>=13&&y<16)mode=mode==='race'?'coop':'race';
  if(setup)showSetup();
 },true);

 for(const button of document.querySelectorAll('[data-party]'))button.addEventListener('click',()=>{
  try{window.Module?._web_save?.();}catch{}
  if(button.dataset.party==='join'){api.active=true;setup=true;room='';mode='race';error='';showSetup();canvas.focus();}
  else{mode=button.dataset.party;connect('new');}
 });
})();

/* Account screens use the same cell canvas as the game. Invisible, positioned
   native inputs provide mobile keyboards and password-manager support. */
window.TVAccounts=class {
 constructor(host){
  this.host=host;this.active=false;this.user=null;this.csrf='';this.revision=0;this.synced='';this.busy=false;this.mode='login';this.message='';this.fields=[];this.buttons=[];this.lastDraw=null;this.selected=0;this.entries=Object.create(null);
  const input=(type,autocomplete,label)=>{const e=document.createElement('input');e.type=type;e.autocomplete=autocomplete;e.setAttribute('aria-label',label);e.className='cell-input';e.hidden=true;e.spellcheck=false;document.querySelector('#stage').append(e);e.addEventListener('input',()=>this.draw());e.addEventListener('keydown',event=>{event.stopPropagation();if(event.key==='Tab'){event.preventDefault();if(e===this.name&&!event.shiftKey)this.pass.focus();else if(e===this.pass&&event.shiftKey)this.name.focus();else{document.querySelector('#game').focus();this.selected=0;this.draw();}}if(event.key==='Enter'){event.preventDefault();if(e===this.name)this.pass.focus();else this.authenticate();}if(event.key==='Escape')this.close();});return e;};
  this.name=input('text','username','Nazwa użytkownika / Username');this.name.maxLength=24;
  this.pass=input('password','current-password','Hasło / Password');this.pass.maxLength=72;
  this.name.addEventListener('focus',()=>this.draw());this.pass.addEventListener('focus',()=>this.draw());
  addEventListener('keydown',e=>{if(e.code==='F2'){e.preventDefault();this.active?this.close():this.open();}else if(this.active&&e.key==='Escape'){e.preventDefault();this.close();}else if(this.active&&['ArrowDown','ArrowUp','Tab'].includes(e.key)){e.preventDefault();const direction=e.key==='ArrowUp'||e.shiftKey?-1:1;this.selected=(this.selected+direction+this.buttons.length)%this.buttons.length;this.draw();}else if(this.active&&e.key==='Enter'){e.preventDefault();this.buttons[this.selected]?.fn();}});
  document.querySelector('#game').addEventListener('pointerdown',e=>{if(e.button!==0||!this.active)return;const r=e.target.getBoundingClientRect(),x=(e.clientX-r.left)/r.width*this.width,y=(e.clientY-r.top)/r.height*this.height;for(const b of this.buttons)if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+2){e.preventDefault();b.fn();break;}});
 }
 tr(pl,en){return this.host.english()?en:pl;}
 read(path){try{return Module.FS.readFile(path,{encoding:'utf8'});}catch{return '';}}
 local(){Module._web_save();return this.read('/persist/progress.dat.iceberg');}
 remember(){Module.FS.writeFile('/persist/accounts.json',JSON.stringify(this.entries));this.host.flush();}
 async request(action,method='GET',data){
  const base=document.querySelector('meta[name="tv-account-api"]')?.content||'api.php';
  const response=await fetch(new URL(base,location.href)+'?action='+action,{method,signal:AbortSignal.timeout(12000),credentials:'same-origin',cache:'no-store',headers:method==='GET'?{}:{'Content-Type':'application/json','X-TV-CSRF':this.csrf},body:data?JSON.stringify(data):undefined});
  let body;try{body=await response.json();}catch{throw Error('unavailable');}if(!response.ok)throw Error(body.error||'unavailable');return body;
 }
 error(e){const map={invalid_credentials:['Nieprawidłowy login lub hasło.','Incorrect username or password.'],name_unavailable:['Ta nazwa jest już zajęta.','That username is taken.'],credentials_format:['Login: 3–24 znaki a–z, 0–9, _ lub -. Hasło: 8–72 bajty.','Username: 3–24 letters, digits, _ or -. Password: 8–72 bytes.'],rate_limit:['Za dużo prób. Spróbuj później.','Too many attempts. Try again later.'],save_invalid:['Nie można odczytać zapisu. Poprzednia gra pozostaje bez zmian.','Cannot read that save. Your previous game is unchanged.'],save_conflict:['Inne urządzenie zapisało nowszą grę. Wybierz zapis.','Another device saved a newer game. Choose a save.'],session_expired:['Zaloguj się ponownie. Zapis jest tutaj bezpieczny.','Sign in again. Your save is safe here.']};return (map[e.message]||['Konto jest chwilowo niedostępne. Zapis lokalny działa.','Accounts are temporarily unavailable. Local saves still work.'])[this.host.english()?1:0];}
 async start(){
  try{this.entries=Object.assign(Object.create(null),JSON.parse(this.read('/persist/accounts.json')||'{}'));}catch{this.entries=Object.create(null);}
  try{const session=await this.request('session');this.csrf=session.csrf;if(session.username){this.pendingUser=session.username;const entry=this.entries[session.username];if(entry?.active){const remote=await this.request('save');this.candidate=this.local();this.remote=remote;if(remote.revision===entry.revision){this.bind(session.username,entry.revision,entry.synced);return;}if(this.candidate===entry.synced&&remote.save){this.apply(remote.save);this.bind(session.username,remote.revision,remote.save);return;}this.openChoice();}}}catch{}
 }
 open(){if(!this.host.ready())return;if(!this.active)Module._web_account_pause();this.active=true;document.body.classList.add('account-open');this.host.clear();if(this.mode!=='choose'){this.mode=this.user?'account':'login';this.message='';}this.draw();if(this.mode==='login'||this.mode==='register')this.name.focus();else document.querySelector('#game').focus();}
 close(){if(this.busy)return;this.active=false;this.lastDraw=null;document.body.classList.remove('account-open');Module._web_account_resume();this.name.hidden=this.pass.hidden=true;this.pass.value='';this.host.clear();document.querySelector('#game').focus();}
 async authenticate(){
  if(this.busy||!['login','register'].includes(this.mode))return;
  this.busy=true;this.message=this.tr('Łączenie…','Connecting…');this.draw();
  try{
   if(!this.csrf){const session=await this.request('session');this.csrf=session.csrf;}
   const result=await this.request(this.mode,'POST',{username:this.name.value,password:this.pass.value});this.csrf=result.csrf;this.pass.value='';this.pendingUser=result.username;
   this.candidate=this.entries[result.username]?.local||this.local();this.remote=await this.request('save');this.ensureGuest();
   if(!this.remote.save){this.bind(result.username,0,'');await this.upload(this.candidate);this.mode='account';this.message=this.tr('Konto gotowe. Twoja gra jest zapisana.','Account ready. Your game is saved.');}
   else if(this.candidate===this.remote.save){this.apply(this.candidate);this.bind(result.username,this.remote.revision,this.remote.save);this.mode='account';this.message=this.tr('Zalogowano.','Signed in.');}
   else this.openChoice();
  }catch(e){this.message=this.error(e);}finally{this.busy=false;this.draw();}
 }
 openChoice(){if(!this.active)Module._web_account_pause();this.active=true;document.body.classList.add('account-open');this.host.clear();this.mode='choose';this.message=this.tr('Masz dwa zapisy. Który kontynuujesz?','You have two saves. Which will you continue?');this.draw();}
 apply(save){Module.FS.writeFile('/persist/import.iceberg',save);if(!Module._web_import_save())throw Error('save_invalid');Module.FS.unlink('/persist/import.iceberg');this.host.flush();}
 ensureGuest(){if(!this.user&&!this.read('/persist/guest.iceberg'))Module.FS.writeFile('/persist/guest.iceberg',this.local());}
 bind(name,revision,synced){
  this.user=name;this.revision=revision;this.synced=synced;
  this.entries[name]={revision,synced,local:this.local(),active:true};this.remember();
 }
 async choose(remote){if(this.busy)return;this.busy=true;
  try{const latest=await this.request('save');if(latest.revision!==this.remote.revision){this.remote=latest;this.message=this.tr('Zapis konta znów się zmienił. Wybierz ponownie.','The account save changed again. Choose again.');return;}
   this.ensureGuest();if(remote){this.apply(latest.save);this.bind(this.pendingUser,latest.revision,latest.save);}
   else{this.apply(this.candidate);this.bind(this.pendingUser,latest.revision,latest.save);await this.upload(this.candidate);}
   this.mode='account';this.message=this.tr('Zapis gotowy. Możesz kontynuować.','Save ready. You can continue.');
  }catch(e){this.message=this.error(e);}finally{this.busy=false;this.draw();}
 }
 async upload(save){const result=await this.request('save','PUT',{revision:this.revision,save});this.revision=result.revision;this.synced=save;this.entries[this.user]={revision:this.revision,synced:save,local:save,active:true};this.remember();}
 async sync(){if(!this.user||this.busy||this.mode==='choose')return;const save=this.local();this.entries[this.user]={revision:this.revision,synced:this.synced,local:save,active:true};this.remember();if(save===this.synced)return;this.busy=true;
  try{await this.upload(save);this.message=this.tr('Zapisano na koncie.','Saved to your account.');}catch(e){this.message=this.error(e);if(e.message==='save_conflict'){this.pendingUser=this.user;this.candidate=save;this.remote=await this.request('save').catch(()=>null);if(this.remote)this.openChoice();}}finally{this.busy=false;if(this.active)this.draw();}
 }
 async logout(){if(this.busy)return;this.busy=true;
  try{this.entries[this.user]={revision:this.revision,synced:this.synced,local:this.local(),active:false};this.remember();const result=await this.request('logout','POST',{});this.csrf=result.csrf;this.user=null;const guest=this.read('/persist/guest.iceberg');if(guest){this.apply(guest);Module.FS.unlink('/persist/guest.iceberg');}else Module._web_guest_profile();this.host.flush();this.mode='login';this.message=this.tr('Wylogowano. Niewysłany zapis konta zachowano w tej przeglądarce.','Signed out. Any unsent account save stays in this browser.');}catch(e){this.message=this.error(e);}finally{this.busy=false;this.draw();}
 }
 draw(){
  if(!this.active)return;
  const landscape=innerHeight<500, narrow=innerWidth<760,w=landscape?60:narrow?38:76,h=landscape?20:28;
  const form=this.mode==='login'||this.mode==='register';
  // Autofill may change a native field without dispatching an input event.
  const name=form?this.name.value.slice(-28):'',passwordLength=form?Math.min(28,this.pass.value.length):0;
  const focus=document.activeElement;
  const state=[w,h,devicePixelRatio,this.host.english(),this.mode,this.user,this.message,this.busy,
   this.selected,name,passwordLength,focus===this.name,focus===this.pass];
  if(this.lastDraw&&!this.host.needsRedraw()&&state.every((value,i)=>value===this.lastDraw[i]))return;
  const nameRow=landscape?5:7,passRow=landscape?8:10,submitRow=landscape?11:13,otherRow=landscape?13:16,messageRow=landscape?15:21;this.width=w;this.height=h;this.buttons=[];
  const cells=new Uint32Array(w*h*4);for(let i=0;i<w*h;i++){cells[i*4]=32;cells[i*4+1]=0xc8bbb8;cells[i*4+2]=0x101923;}
  const text=(x,y,s,color=0xc8bbb8)=>{for(const ch of s){if(x>=w-2)break;let n=(y*w+x++)*4;cells[n]=ch.codePointAt(0);cells[n+1]=color;}};
  const wrap=(y,s)=>{let line='';for(const word of s.split(/\s+/)){if((line+' '+word).length>w-5){text(2,y++,line);line=word;}else line+=(line?' ':'')+word;}if(line)text(2,y++,line);return y;};
  const button=(y,label,fn)=>{text(2,y,(this.selected===this.buttons.length?'▸ ':'  ')+'[ '+label+' ]',0xe4ad68);this.buttons.push({x:2,y,w:w-4,fn});};
  for(let x=1;x<w-1;x++){text(x,0,'─',0x345866);text(x,h-1,'─',0x345866);}text(2,2,this.tr('POCZTA KURIERSKA — KONTO','COURIER POST — ACCOUNT'),0x9dd4dd);
  this.name.hidden=this.pass.hidden=!form;
  if(form){
   text(2,landscape?3:4,this.mode==='register'?this.tr('Nowe konto','Create account'):this.tr('Zaloguj się','Sign in'),0xe4ad68);
   text(2,nameRow-1,this.tr('Nazwa użytkownika','Username'));text(2,nameRow,'▏'+name+'_',focus===this.name?0xe4ad68:0xc8bbb8);
   text(2,passRow-1,this.tr('Hasło','Password'));text(2,passRow,'▏'+'•'.repeat(passwordLength)+'_',focus===this.pass?0xe4ad68:0xc8bbb8);
   button(submitRow,this.busy?this.tr('Łączenie…','Connecting…'):this.mode==='register'?this.tr('Utwórz konto','Create account'):this.tr('Zaloguj','Sign in'),()=>this.authenticate());
   button(otherRow,this.mode==='register'?this.tr('Mam już konto','I have an account'):this.tr('Załóż konto','Create an account'),()=>{this.mode=this.mode==='register'?'login':'register';this.pass.autocomplete=this.mode==='register'?'new-password':'current-password';this.message='';this.draw();});
   if(!landscape)wrap(19,this.tr('Tylko login i hasło. Bez adresu e-mail.','Just a username and password. No email.'));
   if(!this.message)wrap(messageRow,this.tr('Login: 3–24 znaki. Hasło: co najmniej 8 znaków.','Username: 3–24 characters. Password: at least 8 characters.'));
  }else if(this.mode==='choose'){
   wrap(5,this.message);button(landscape?8:10,this.tr('Wczytaj zapis konta','Load account save'),()=>this.choose(true));button(landscape?11:14,this.tr('Wyślij zapis z tej przeglądarki','Use this browser save'),()=>this.choose(false));wrap(landscape?14:18,this.tr('Poprzednia kopia pozostaje zachowana.','The previous copy is kept.'));
  }else{
   text(2,5,this.user||'',0xe4ad68);wrap(8,this.tr('Postęp zapisuje się także na koncie.','Progress also saves to your account.'));button(landscape?10:12,this.tr('Zapisz teraz','Save now'),()=>this.sync());button(landscape?12:16,this.tr('Wyloguj','Sign out'),()=>this.logout());
  }
  if(this.message&&this.mode!=='choose')wrap(messageRow,this.message);
  button(h-2,this.tr('Wróć do gry','Return to game'),()=>this.close());this.host.frame(cells,w,h);
  if(form){
   const r=document.querySelector('#game').getBoundingClientRect(),stage=document.querySelector('#stage').getBoundingClientRect();
   [this.name,this.pass].forEach((e,i)=>{e.hidden=false;Object.assign(e.style,{left:r.left-stage.left+2*r.width/w+'px',top:r.top-stage.top+(i?passRow:nameRow)*r.height/h+'px',width:(w-4)*r.width/w+'px',height:r.height/h+'px'});});
  }
  this.lastDraw=state;
 }
};

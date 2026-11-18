// Real Chromium / WASM entry checks; no production accounts or player storage.
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict');
const {chromium}=require('playwright');
const root=path.resolve(__dirname,'..'),out=path.resolve(process.argv[2]||path.join(root,'../frontend-evidence'));
fs.mkdirSync(out,{recursive:true});
const mime={'.html':'text/html','.js':'text/javascript','.css':'text/css','.wasm':'application/wasm','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{
 const url=new URL(req.url,'http://localhost');
 if(url.pathname.endsWith('/api.php')){res.setHeader('Content-Type','application/json');res.end(JSON.stringify({csrf:'isolated-entry-check',username:null}));return;}
 const file=path.join(root,'web/dist',url.pathname==='/'?'index.html':url.pathname);
 if(!file.startsWith(path.join(root,'web/dist')+path.sep)||!fs.existsSync(file)||!fs.statSync(file).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',mime[path.extname(file)]||'application/octet-stream');fs.createReadStream(file).pipe(res);
});
(async()=>{
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const url='http://127.0.0.1:'+server.address().port;
 const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME||'/opt/google/chrome/chrome',args:['--mute-audio']});
 const errors=[],visualOnly=process.argv.includes('--visual-only');
 try{
  // Select a language while WASM is still loading; async save restoration must
  // not replace that explicit entry-screen choice.
  if(!visualOnly){
  const loadingContext=await browser.newContext({locale:'en-US'}),loadingPage=await loadingContext.newPage();
  let resumeModule;const moduleGate=new Promise(resolve=>resumeModule=resolve);
  await loadingPage.route('**/terminal-velocity.js',async route=>{await moduleGate;await route.continue();});
  await loadingPage.route('**/language.php',route=>route.fulfill({json:{language:'en'}}));
  await loadingPage.goto(url,{waitUntil:'commit'});await loadingPage.waitForFunction(()=>typeof selectLanguage==='function');
  await loadingPage.locator('[data-language="pl"]').click();resumeModule();
  await loadingPage.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await loadingPage.locator('html').getAttribute('lang'),'pl','explicit choice during loading wins');
  await loadingContext.close();console.log('PASS explicit language choice during module loading');
  for(const asset of ['terminal-velocity.js','terminal-velocity.wasm','terminal-velocity.data']){
   const offline=await browser.newPage({locale:'en-US'});
   await offline.route('**/'+asset,route=>route.abort('failed'));
   await offline.goto(url,{waitUntil:'domcontentloaded'});
   const status=offline.locator('#save-status');
   await status.getByText('Startup failed — reload the page',{exact:true}).waitFor();
   assert.equal(await offline.locator('#start').isDisabled(),true);
   await offline.locator('[data-language="pl"]').click();
   assert.equal(await status.innerText(),'Błąd uruchamiania — odśwież stronę');
   await offline.close();
  }
  console.log('PASS missing startup assets report a localized reload message');
  const guest=await browser.newPage({locale:'en-US'});
  await guest.route('**/accounts.js',route=>route.abort('failed'));
  await guest.goto(url,{waitUntil:'domcontentloaded'});
  await guest.waitForFunction(()=>!document.querySelector('#start').disabled);
  await guest.locator('#start').click();
  await guest.waitForFunction(()=>started&&(uiFlags&256));
  assert.equal(await guest.locator('#account').isHidden(),true);
  assert.equal(await guest.locator('#export-save').isEnabled(),true);
  await guest.close();
  console.log('PASS local play starts when the optional account script is unavailable');
  const storageContext=await browser.newContext({locale:'en-US'});
  const interrupted=await storageContext.newPage();
  const legacySave=fs.readFileSync(path.join(root,'tests/fixtures/iceberg-v4.dat'),'utf8');
  await interrupted.goto(url,{waitUntil:'domcontentloaded'});
  await interrupted.waitForFunction(()=>!document.querySelector('#start').disabled);
  await interrupted.evaluate(bytes=>new Promise((resolve,reject)=>{
   Module.FS.writeFile('/persist/progress.dat.iceberg',bytes);
   Module.FS.syncfs(false,error=>error?reject(error):resolve());
  }),legacySave);
  await interrupted.addInitScript(()=>{
   if(!sessionStorage.getItem('test-failed-restore'))return;
   sessionStorage.removeItem('test-failed-restore');
   const open=IDBFactory.prototype.open;
   IDBFactory.prototype.open=function(name,...args){
    if(name!=='/persist')return open.call(this,name,...args);
    IDBFactory.prototype.open=open;
    const request={};
    queueMicrotask(()=>request.onerror?.({target:{error:new Error('Test storage read failure')},preventDefault(){}}));
    return request;
   };
  });
  await interrupted.evaluate(()=>sessionStorage.setItem('test-failed-restore','1'));
  await interrupted.reload({waitUntil:'domcontentloaded'});
  await interrupted.waitForFunction(()=>!document.querySelector('#start').disabled);
  await interrupted.locator('[data-language="en"]').click();
  await interrupted.locator('#start').click();
  await interrupted.waitForFunction(()=>started&&!syncing);
  assert.equal(await interrupted.locator('#save-status').textContent(),'Browser storage unavailable — download backups');
  await interrupted.reload({waitUntil:'domcontentloaded'});
  await interrupted.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await interrupted.evaluate(()=>Module.FS.readFile('/persist/progress.dat.iceberg',{encoding:'utf8'})),legacySave);
  console.log('PASS an interrupted IndexedDB restore leaves the previous browser save intact');
  // A database request can remain pending instead of firing an error. Delay
  // the real open past Start, then let the normal IDBFS restore finish late.
  await interrupted.addInitScript(()=>{
   if(!sessionStorage.getItem('test-stalled-restore'))return;
   sessionStorage.removeItem('test-stalled-restore');
   const open=IDBFactory.prototype.open;
   IDBFactory.prototype.open=function(name,...args){
    if(name!=='/persist')return open.call(this,name,...args);
    IDBFactory.prototype.open=open;
    const deferred={};
    window.releaseTestRestore=()=>{
     const actual=open.call(this,name,...args);
     actual.onupgradeneeded=deferred.onupgradeneeded;
     actual.onerror=deferred.onerror;
     actual.onsuccess=event=>{
      deferred.result=actual.result;
      deferred.onsuccess(event);
     };
    };
    return deferred;
   };
  });
  await interrupted.evaluate(()=>{
   localStorage.setItem('terminal-velocity.save-slot','1');
   sessionStorage.setItem('test-stalled-restore','1');
  });
  await interrupted.reload({waitUntil:'domcontentloaded'});
  await interrupted.locator('[data-language="en"]').click();
  await interrupted.locator('#fullscreen').click();
  await interrupted.waitForFunction(()=>document.fullscreenElement===playfield);
  const fullscreen=interrupted.locator('#fullscreen-tools');
  const fullscreenStart=fullscreen.locator('#start');
  await fullscreenStart.waitFor({state:'visible',timeout:3000});
  await interrupted.waitForFunction(()=>!document.querySelector('#start').disabled,{},{timeout:9000});
  await fullscreenStart.press('Enter');
  await interrupted.waitForFunction(()=>started&&!syncing);
  const fullscreenStatus=fullscreen.locator('#save-status');
  assert.equal(await fullscreenStatus.isVisible(),true);
  assert.equal(await fullscreenStatus.textContent(),'Browser storage unavailable — download backups');
  assert.equal(await interrupted.locator('#account-fullscreen').isHidden(),true);
  const sessionDownload=await Promise.all([
   interrupted.waitForEvent('download'),fullscreen.locator('#export-save').click(),
  ]);
  const sessionSave=fs.readFileSync(await sessionDownload[0].path(),'utf8');
  assert.match(sessionSave,/^TVICE6 /);
  // A new tab can become writer before the pending restore completes.
  const recovered=await storageContext.newPage();
  await recovered.goto(url,{waitUntil:'domcontentloaded'});
  await recovered.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await recovered.evaluate(()=>browserStorageWritable),true);
  assert.equal(await recovered.evaluate(()=>Module.FS.readFile('/persist/progress.dat.iceberg',{encoding:'utf8'})),legacySave);
  await interrupted.evaluate(()=>window.releaseTestRestore());
  await interrupted.waitForFunction(()=>Module.FS.analyzePath('/persist/progress.dat.iceberg').exists);
  assert.equal(await interrupted.evaluate(()=>Module._web_saved_language()),1,'the late Polish save must not replace the English guest session');
  interrupted.once('dialog',dialog=>dialog.accept());
  await interrupted.locator('#save-file').setInputFiles({
   name:'recovered.iceberg',mimeType:'application/octet-stream',buffer:Buffer.from(legacySave),
  });
  await interrupted.waitForFunction(()=>!importPending&&Module._web_saved_language()===0);
  assert.equal(await fullscreen.locator('#export-previous-save').isVisible(),true);
  assert.equal(await interrupted.evaluate(()=>Module.FS.readFile('/persist/progress.dat.iceberg',{encoding:'utf8'})),legacySave);
  await fullscreen.locator('#leave-fullscreen').click();
  await interrupted.waitForFunction(()=>!expanded());
  await interrupted.locator('header #save-status').waitFor({state:'visible'});
  await interrupted.locator('#toolbar #export-previous-save').waitFor({state:'visible'});
  const chooseSlot=async(page,slot)=>{
   await page.locator('footer [data-tap="12"]').click();
   await page.waitForFunction(()=>uiFlags&1);
   await page.keyboard.press('Digit5');
   await page.keyboard.press('Enter');
   await page.waitForFunction(()=>uiFlags&16);
   await page.keyboard.press('Digit6');
   await page.waitForFunction(()=>uiFlags&profilePickerFlag);
   await page.keyboard.press('Digit'+slot);
   await page.keyboard.press('Enter');
   await page.waitForFunction(slot=>selectedSlot===slot,slot);
  };
  await chooseSlot(interrupted,3);
  assert.equal(await interrupted.evaluate(()=>localStorage.getItem(selectedSlotKey)),'1',
   'temporary slot switches preserve the last saved slot');

  await recovered.close();
  await interrupted.reload({waitUntil:'domcontentloaded'});
  await interrupted.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await interrupted.evaluate(()=>selectedSlot),1,'normal reload opens the last saved slot');
  console.log('PASS temporary play preserves the original saves and selected slot');

  await interrupted.locator('[data-language="en"]').click();
  await interrupted.locator('#start').click();
  await interrupted.waitForFunction(()=>started&&!syncing);
  const secondTab=await storageContext.newPage();
  await secondTab.goto(url,{waitUntil:'domcontentloaded'});
  await secondTab.waitForFunction(()=>!document.querySelector('#start').disabled);
  await interrupted.locator('[data-language="pl"]').click();
  await interrupted.waitForFunction(()=>!syncing&&Module._web_saved_language()===0);
  await secondTab.locator('#start').click();
  await secondTab.waitForFunction(()=>started&&!syncing);
  assert.equal(await secondTab.locator('#save-status').textContent(),'Another tab is saving — download a copy');
  await chooseSlot(secondTab,2);
  assert.equal(await secondTab.evaluate(()=>localStorage.getItem(selectedSlotKey)),'1',
   'a read-only tab cannot change the writer’s remembered slot');
  await interrupted.close();
  await secondTab.reload({waitUntil:'domcontentloaded'});
  await secondTab.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await secondTab.evaluate(()=>Module._web_saved_language()),0,'stale tab preserved the writer’s profile');
  assert.equal(await secondTab.evaluate(()=>selectedSlot),1);
  await secondTab.locator('#start').click();
  await secondTab.waitForFunction(()=>started&&!syncing&&browserStorageWritable);
  await chooseSlot(secondTab,2);
  assert.equal(await secondTab.evaluate(()=>localStorage.getItem(selectedSlotKey)),'2');
  await secondTab.reload({waitUntil:'domcontentloaded'});
  await secondTab.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await secondTab.evaluate(()=>selectedSlot),2,'the writer still remembers a deliberate slot change');
  await storageContext.close();
  console.log('PASS a second browser tab preserves the current writer’s save');
  const pendingSave=await browser.newPage({locale:'en-US'});
  await pendingSave.goto(url);
  await pendingSave.waitForFunction(()=>!document.querySelector('#start').disabled);
  await pendingSave.locator('[data-language="en"]').click();
  await pendingSave.locator('#start').click();
  await pendingSave.waitForFunction(()=>started&&!syncing);
  const pendingStatus=pendingSave.locator('#save-status');
  await pendingStatus.getByText('Saved in this browser ✓',{exact:true}).waitFor();
  // Keep the actual IndexedDB writes pending until each is explicitly released.
  await pendingSave.evaluate(()=>{
   const fs=Module.FS,original=fs.syncfs,release=[];
   window.pendingWrites=release;
   fs.syncfs=function(populate,done){
    if(populate||release.length>=2)return original.call(fs,populate,done);
    release.push(()=>original.call(fs,false,done));
   };
   syncSave();
  });
  await pendingStatus.getByText('Saving…',{exact:true}).waitFor({timeout:2000});
  await pendingStatus.getByText('Save delayed — download a copy',{exact:true}).waitFor({timeout:9000});
  assert.equal(await pendingSave.evaluate(()=>pendingWrites.length),1);
  await pendingSave.locator('[data-language="pl"]').click();
  assert.equal(await pendingStatus.textContent(),'Zapis trwa długo — pobierz kopię');
  const [pendingBackup]=await Promise.all([
   pendingSave.waitForEvent('download'),pendingSave.locator('#export-save').click(),
  ]);
  assert.match(fs.readFileSync(await pendingBackup.path(),'utf8'),/^TVICE6 /);
  assert.equal(await pendingStatus.textContent(),'Zapis trwa długo — pobierz kopię');
  await pendingSave.evaluate(()=>pendingWrites[0]());
  await pendingSave.waitForFunction(()=>pendingWrites.length===2);
  assert.equal(await pendingStatus.textContent(),'Zapis trwa długo — pobierz kopię');
  await pendingSave.evaluate(()=>pendingWrites[1]());
  await pendingStatus.getByText('Zapis w tej przeglądarce ✓',{exact:true}).waitFor();
  await pendingSave.reload();
  await pendingSave.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await pendingSave.evaluate(()=>Module._web_saved_language()),0);
  await pendingSave.close();
  console.log('PASS a delayed save reports progress, keeps backup available and confirms only the last write');

  const rejectedSave=await browser.newPage({locale:'en-US'});
  const saveErrors=[];
  rejectedSave.on('pageerror',error=>saveErrors.push(error.message));
  await rejectedSave.goto(url);
  await rejectedSave.waitForFunction(()=>!document.querySelector('#start').disabled);
  await rejectedSave.locator('[data-language="en"]').click();
  await rejectedSave.evaluate(()=>{
   const syncfs=Module.FS.syncfs;
   Module.FS.syncfs=function(populate,done){
    if(populate)return syncfs.call(this,populate,done);
    Module.FS.syncfs=syncfs;
    throw Error('Simulated synchronous storage failure');
   };
   const transaction=IDBDatabase.prototype.transaction;
   IDBDatabase.prototype.transaction=function(stores,mode,...args){
    if(window.rejectWriteTransaction&&this.name==='/persist'&&mode==='readwrite'){
     window.rejectedWrites=(window.rejectedWrites||0)+1;
     throw new DOMException('The database connection is closing','InvalidStateError');
    }
    return transaction.call(this,stores,mode,...args);
   };
  });
  await rejectedSave.locator('#start').click();
  const recoveryStatus=rejectedSave.locator('#save-status');
  const warning='Save failed — download a copy';
  const savedText='Saved in this browser ✓';
  await recoveryStatus.getByText(warning,{exact:true}).waitFor();
  assert.equal(await rejectedSave.evaluate(()=>started&&!bootFailed&&!syncing),true);
  await rejectedSave.evaluate(()=>syncSave());
  await recoveryStatus.getByText(savedText,{exact:true}).waitFor();
  await rejectedSave.evaluate(()=>window.rejectWriteTransaction=true);
  await rejectedSave.keyboard.press('r');
  await rejectedSave.waitForFunction(()=>window.rejectedWrites>0&&!syncing);
  await recoveryStatus.getByText(warning,{exact:true}).waitFor();
  const [recoveryBackup]=await Promise.all([
   rejectedSave.waitForEvent('download'),rejectedSave.locator('#export-save').click(),
  ]);
  assert.match(fs.readFileSync(await recoveryBackup.path(),'utf8'),/^TVICE6 /);
  await rejectedSave.locator('footer [data-tap="9"]').click();
  await rejectedSave.waitForFunction(()=>uiFlags&2);
  await rejectedSave.evaluate(()=>{window.rejectWriteTransaction=false;syncSave();});
  await recoveryStatus.getByText(savedText,{exact:true}).waitFor();
  assert.deepEqual(saveErrors,[]);
  await rejectedSave.close();
  console.log('PASS a failed browser save leaves the game playable, allows a backup and recovers on the next write');

  const located=await browser.newPage({locale:'en-US'});
  await located.route('**/language.php',route=>route.fulfill({json:{language:'pl'}}));
  await located.goto(url);
  await located.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await located.locator('html').getAttribute('lang'),'pl','a fresh browser uses approximate country');
  await located.close();
  console.log('PASS country-based first-run language with manual override');
  }
  for(const [locale,expected] of (visualOnly?[['en-US','en']]:[['en-US','en'],['pl-PL','pl'],['de-DE','en']])){
   const context=await browser.newContext({locale,viewport:{width:1280,height:900}}),page=await context.newPage();page.on('pageerror',e=>errors.push(String(e)));
   await page.goto(url);await page.waitForFunction(()=>!document.querySelector('#start').disabled);
   assert.equal(await page.locator('html').getAttribute('lang'),expected,locale+' fresh boot');
   assert.equal(await page.locator('[data-language="'+expected+'"]').getAttribute('aria-pressed'),'true');
   await page.screenshot({path:path.join(out,'web-boot-'+locale+'.png')});
   if(locale==='en-US'&&!visualOnly){
    const pause=page.locator('footer [data-tap="9"]');
    assert.equal(await pause.isDisabled(),true);
    const box=await pause.boundingBox();
    await page.locator('#game').focus();
    await page.mouse.click(box.x+box.width/2,box.y+box.height/2);
    await page.keyboard.press('Enter');
    await page.waitForFunction(()=>started&&(uiFlags&256));
    assert.equal(await page.evaluate(()=>uiFlags&(1|2)),0,'Start cannot replay an earlier toolbar click');
    assert.equal(await pause.isEnabled(),true);
    await page.keyboard.press('m');
    await page.waitForFunction(()=>uiFlags&1);
    await page.keyboard.press('Digit2');
    await page.keyboard.press('Enter');
    await page.waitForFunction(()=>uiFlags&32);
    const replacement=page.locator('#save-replacement-actions');
    assert.equal(await replacement.getByRole('button',{name:'Replace saved journey'}).isVisible(),true);
    await replacement.getByRole('button',{name:'Cancel'}).click();
    await page.waitForFunction(()=>!(uiFlags&32));
    assert.equal(await replacement.isVisible(),false);
    await page.keyboard.press('Digit1');
    await page.keyboard.press('Enter');
    await page.waitForFunction(()=>!(uiFlags&1));
   }else await page.locator('#start').click();
   await page.waitForFunction(()=>document.querySelector('#start').hidden);
   await page.waitForTimeout(100);assert.equal(await page.locator('html').getAttribute('lang'),expected,locale+' fresh game');
   if(locale==='en-US'){
    await page.evaluate(()=>{const frame=window.tvFrame,ui=window.tvUI;window.tvFrame=(cells,w,h)=>{window.entryFrame=Array.from({length:h},(_,y)=>Array.from({length:w},(_,x)=>String.fromCodePoint(cells[(y*w+x)*4])).join('')).join('\n');frame(cells,w,h);};window.tvUI=u=>{window.entryFlags=u.flags;ui(u);};});
    await page.waitForFunction(()=>!(uiFlags&1)&&(uiFlags&256));await page.waitForTimeout(650);
    const frameWork=await page.evaluate(async()=>{
     const buffer=previous.buffer;
     let mutations=0;
     const observer=new MutationObserver(records=>{mutations+=records.length;});
     observer.observe(document.querySelector('#touch-use'),{childList:true,attributes:true});
     try{
      for(let i=0;i<8;i++)await new Promise(requestAnimationFrame);
      return {mutations:mutations+observer.takeRecords().length,reusesBuffer:buffer===previous.buffer};
     }finally{observer.disconnect();}
    });
    assert.deepEqual(frameWork,{mutations:0,reusesBuffer:true},'steady gameplay reuses frame storage and leaves unchanged controls alone');
    console.log('PASS steady browser animation reuses its framebuffer and keeps stable controls untouched');
    await page.keyboard.down('d');await page.keyboard.down('ArrowRight');
    await page.keyboard.up('d');
    assert.ok(await page.evaluate(()=>held&(1<<1)),'right remains held through the other physical key');
    await page.keyboard.up('ArrowRight');
    assert.equal(await page.evaluate(()=>held&(1<<1)),0,'right releases after both physical keys are up');

    await page.keyboard.down('d');
    const focusPause=await page.evaluate(()=>{
      // Simulate a focus round trip with no animation frame between events.
      window.dispatchEvent(new Event('blur'));
      window.dispatchEvent(new Event('focus'));
      window.dispatchEvent(new KeyboardEvent('keydown',{code:'KeyD',repeat:true}));
      return {paused:!!(uiFlags&2),held,pressed};
    });
    await page.keyboard.up('d');
    assert.deepEqual(focusPause,{paused:true,held:0,pressed:0},'short focus loss pauses immediately and drops held input');
    await page.keyboard.press('Enter');
    await page.waitForFunction(()=>!(uiFlags&2));
    console.log('PASS short focus loss between animation frames pauses and resumes with cleared input');

    const game=page.locator('#game');
    await game.hover();
    await page.mouse.down();
    try{
     await page.keyboard.press('p');
     await page.waitForFunction(()=>uiFlags&2);
    }finally{await page.mouse.up();}
    await page.evaluate(async()=>{
     for(let i=0;i<3;i++)await new Promise(requestAnimationFrame);
    });
    assert.equal(await page.evaluate(()=>!!(uiFlags&2)),true,
     'a press that began during gameplay cannot dismiss a later pause');
    await game.click();
    await page.waitForFunction(()=>!(uiFlags&2));
    console.log('PASS a fresh canvas click resumes a pause and a stale release is ignored');

    await page.evaluate(()=>{
     const tick=Module._web_tick;
     window.dashEdges=0;
     Module._web_tick=(dt,held,pressed,...rest)=>{
      if(pressed&(1<<5))window.dashEdges++;
      return tick(dt,held,pressed,...rest);
     };
    });
    await page.keyboard.down('Shift');
    await page.evaluate(async()=>{
     for(let i=0;i<4;i++)await new Promise(requestAnimationFrame);
    });
    assert.equal(await page.evaluate(()=>window.dashEdges),0,
      'holding Shift before reverse Tab navigation does not dash');
    await page.keyboard.press('Tab');
    await page.keyboard.up('Shift');
    assert.equal(await page.evaluate(()=>document.activeElement.id),'fullscreen',
      'Shift+Tab moves out of the game to the preceding browser control');
    assert.equal(await page.evaluate(()=>!!(uiFlags&64)),false,'browser navigation does not toggle Quests');
    await page.locator('header a').focus();
    await page.keyboard.press('Tab');
    assert.equal(await page.evaluate(()=>document.activeElement.dataset.language),'pl',
      'Tab moves from the site link to the first language control');
    await page.locator('#game').focus();
    await page.keyboard.down('ShiftLeft');
    await page.locator('#game').click();
    await page.keyboard.up('ShiftLeft');
    await page.keyboard.down('ShiftLeft');
    await page.keyboard.down('ShiftRight');
    await page.keyboard.up('ShiftLeft');
    await page.keyboard.up('ShiftRight');
    await page.evaluate(async()=>{for(let i=0;i<4;i++)await new Promise(requestAnimationFrame);});
    assert.equal(await page.evaluate(()=>window.dashEdges),0,'modified clicks and overlapping Shift keys do not dash');
    await page.keyboard.press('p');
    await page.waitForFunction(()=>!!(uiFlags&2));
    await page.keyboard.press('Tab');
    assert.equal(await page.evaluate(()=>document.activeElement.dataset.tap),'12','Tab reaches the toolbar while paused');
    await page.locator('#game').focus();
    await page.keyboard.press('F2');
    const accountFocus=()=>page.evaluate(()=>{
     if(document.activeElement===accounts.name)return 'name';
     if(document.activeElement===accounts.pass)return 'password';
     return document.activeElement===canvas?accounts.selected:null;
    });
    const walkAccount=async key=>{
     const visits=[await accountFocus()];
     for(let i=0;i<5;i++){await page.keyboard.press(key);visits.push(await accountFocus());}
     return visits;
    };
    assert.deepEqual(await walkAccount('Tab'),['name','password',0,1,2,'name']);
    assert.deepEqual(await walkAccount('Shift+Tab'),['name',2,1,0,'password','name']);
    await page.keyboard.press('Tab');
    await page.keyboard.press('Alt+Enter');
    await page.waitForFunction(()=>expanded()&&settingsKeys.parentElement.id==='fullscreen-tools');
    assert.equal(await accountFocus(),'password','fullscreen does not interrupt account input');
    await page.keyboard.press('Alt+Enter');
    await page.waitForFunction(()=>!expanded()&&settingsKeys.parentElement.tagName==='FOOTER');
    assert.equal(await accountFocus(),'password','exiting fullscreen preserves the current field');
    await page.keyboard.press('F2');
    await page.waitForFunction(()=>!accounts.active&&(uiFlags&2)&&canvas===document.activeElement);
    console.log('PASS account keyboard traversal returns to both fields and F2 returns to the paused game');
    await page.keyboard.press('F2');
    const returnPoint=await page.evaluate(()=>{
     const button=accounts.buttons.at(-1),rect=canvas.getBoundingClientRect();
     return {x:rect.left+(button.x+3)*rect.width/accounts.width,y:rect.top+(button.y+.5)*rect.height/accounts.height};
    });
    await page.mouse.move(returnPoint.x,returnPoint.y);
    await page.mouse.down();
    assert.equal(await page.evaluate(()=>accounts.active),true,'a held press does not activate the account action');
    await page.mouse.up();
    await page.waitForFunction(()=>!accounts.active&&(uiFlags&2)&&canvas===document.activeElement);
    await page.evaluate(()=>{
     const tick=Module._web_tick;
     window.canvasConfirms=0;
     Module._web_tick=(dt,held,pressed,...rest)=>{
      if(pressed&(1<<8))window.canvasConfirms++;
      return tick(dt,held,pressed,...rest);
     };
    });
    const clickGameCanvas=async()=>{
     await page.locator('#game').click();
     await page.evaluate(async()=>{for(let i=0;i<3;i++)await new Promise(requestAnimationFrame);});
    };
    await page.keyboard.press('j');
    await page.waitForFunction(()=>!!(uiFlags&128)&&entryFrame.includes('JOURNEY MAP'));
    await clickGameCanvas();
    assert.equal(await page.evaluate(()=>window.canvasConfirms),0,'clicking the paused map does not confirm travel');
    assert.equal(await page.evaluate(()=>entryFrame.includes('A MOMENT TO BREATHE')),false);
    await page.keyboard.press('p');
    await page.waitForFunction(()=>(uiFlags&(2|128))===2&&entryFrame.includes('A MOMENT TO BREATHE'));
    await page.locator('footer [data-tap="10"]').click();
    await page.waitForFunction(()=>!!(uiFlags&64)&&entryFrame.includes('QUEST BOOK'));
    assert.equal(await page.evaluate(()=>entryFrame.includes('A MOMENT TO BREATHE')),false);
    await clickGameCanvas();
    assert.equal(await page.evaluate(()=>window.canvasConfirms),0,'clicking the paused quest book does not select an entry');
    await page.keyboard.press('Tab');
    await page.waitForFunction(()=>(uiFlags&(2|64))===2);
    await clickGameCanvas();
    await page.waitForFunction(()=>!(uiFlags&2));
    assert.equal(await page.evaluate(()=>window.canvasConfirms),1,'clicking the pause screen still resumes');
    console.log('PASS map and quests above pause ignore stray canvas clicks and closing them returns to pause');


    await page.screenshot({path:path.join(out,'web-opening-en.png')});
    await page.keyboard.press('j');await page.waitForFunction(()=>!!(window.entryFlags&128));
    await page.keyboard.press('ArrowUp');await page.keyboard.press('Enter');await page.waitForTimeout(100);
    assert.ok(await page.evaluate(()=>!!(entryFlags&128)),'locked travel keeps district map open');
    assert.match(await page.evaluate(()=>entryFrame),/Finish Lower Harbor/,'locked travel explains route and pump condition');
    await page.screenshot({path:path.join(out,'web-map-locked.png')});
    await page.keyboard.press('b');await page.waitForTimeout(100);assert.match(await page.evaluate(()=>entryFrame),/Route|ROUTE/);
    await page.screenshot({path:path.join(out,'web-route-map.png')});
    await page.keyboard.press('j');await page.waitForFunction(()=>!(entryFlags&128));await page.keyboard.press('Tab');await page.waitForFunction(()=>!!(entryFlags&64));
    assert.match(await page.evaluate(()=>entryFrame),/QUEST BOOK/);await page.screenshot({path:path.join(out,'web-quest-book.png')});
    await page.keyboard.press('b');await page.waitForTimeout(100);await page.keyboard.press('Tab');await page.waitForFunction(()=>!(entryFlags&64));
    await page.keyboard.down('d');await page.waitForTimeout(360);await page.keyboard.up('d');await page.keyboard.press('e');
    await page.waitForFunction(()=>!!(entryFlags&4));await page.waitForTimeout(150);await page.screenshot({path:path.join(out,'web-dialogue.png')});
    await page.keyboard.press('Enter');await page.waitForTimeout(80);assert.ok(await page.evaluate(()=>!!(entryFlags&4)),'reveal keeps unread objective on screen');
    await page.keyboard.press('Enter');await page.waitForTimeout(80);assert.match(await page.evaluate(()=>entryFrame),/Start at the board/,'next essential objective shown in one press after reveal');
    await page.keyboard.press('Enter');await page.waitForFunction(()=>!(entryFlags&4));
    console.log('PASS locked map explanation, route tab, quest book and shortened reveal/continue flow through browser controls');
    await page.keyboard.press('Shift');
    await page.waitForFunction(()=>window.dashEdges===1);
    await page.keyboard.down('x');
    await page.waitForFunction(()=>window.dashEdges===2);
    await page.keyboard.up('x');
    console.log('PASS Shift+Tab exits without dashing, tap Shift dashes, and X still dashes on keydown');
    const fullscreenTools=page.locator('#fullscreen-tools');
    const fullscreenMap=fullscreenTools.locator('[data-tap="11"]');
    assert.equal(await fullscreenTools.isVisible(),false);
    await page.keyboard.down('Alt');
    await page.keyboard.down('Enter');
    await page.waitForFunction(()=>document.fullscreenElement===playfield);
    await page.keyboard.down('Enter');
    await page.keyboard.up('Enter');
    await page.keyboard.up('Alt');
    await page.evaluate(async()=>{for(let i=0;i<4;i++)await new Promise(requestAnimationFrame);});
    assert.equal(await page.evaluate(()=>document.fullscreenElement===playfield),true,'holding Alt+Enter enters fullscreen once');
    await fullscreenMap.click();
    await page.waitForFunction(()=>!!(uiFlags&128));
    await fullscreenTools.locator('#leave-fullscreen').click();
    await page.waitForFunction(()=>!expanded()&&(uiFlags&128));
    assert.equal(await fullscreenTools.isVisible(),false);
    await page.locator('footer [data-tap="11"]').click();
    await page.waitForFunction(()=>!(uiFlags&128));
    console.log('PASS wide desktop native fullscreen exposes the map and a mouse exit without closing the map');
    const currentRoundtrip=await page.evaluate(()=>{
      Module._web_save();const exported=Module.FS.readFile('/persist/progress.dat.iceberg',{encoding:'utf8'});
      Module._web_set_language(0);Module._web_save();const changed=Module.FS.readFile('/persist/progress.dat.iceberg',{encoding:'utf8'});
      Module.FS.writeFile('/persist/import.iceberg',exported);const ok=Module._web_import_save();
      const backup=Module.FS.readFile('/persist/progress.dat.iceberg.bak',{encoding:'utf8'});Module._web_save();
      return {exported,changed,ok,backup,restored:Module.FS.readFile('/persist/progress.dat.iceberg',{encoding:'utf8'}),language:Module._web_saved_language()};
    });
    assert.match(currentRoundtrip.exported,/^TVICE6 /,'current browser game exports genuine v6');
    assert.equal(currentRoundtrip.ok,1);assert.equal(currentRoundtrip.language,1);
    assert.equal(currentRoundtrip.backup,currentRoundtrip.changed,'v6 import retains the profile being replaced');
    assert.equal(currentRoundtrip.restored,currentRoundtrip.exported,'v6 import preserves all serialized story, task, route and language state');
    fs.writeFileSync(path.join(out,'browser-current-v6.iceberg'),currentRoundtrip.exported);
    console.log('PASS genuine current TVICE6 browser export/import preserves byte-exact progress and language with rollback');
    if(visualOnly){await context.close();continue;}
    // Hold completion of a real IndexedDB write after it has committed. The
    // language change and account metadata then arrive during that flush.
    await page.waitForFunction(()=>!syncing);
    const queued=await page.evaluate(async()=>{
      const fs=Module.FS,original=fs.syncfs,results=[];
      let release,requests=0;
      const firstCommitted=new Promise(resolve=>{
        fs.syncfs=function(populate,done) {
          ++requests;
          return original.call(fs,populate,error=>{
            results.push(error?String(error):null);
            if(results.length===1){release=()=>done(error);resolve();}
            else done(error);
          });
        };
      });
      syncSave();
      await firstCommitted;
      const marker={active:false,revision:42};
      selectLanguage('pl');
      accounts.entries.fixture=marker;
      accounts.remember();
      const writesBeforeRelease=requests;
      release();
      while(syncing)await new Promise(resolve=>setTimeout(resolve,10));
      fs.syncfs=original;
      return {results,writesBeforeRelease};
    });
    assert.equal(queued.writesBeforeRelease,1,'game and account storage share one active flush');
    assert.deepEqual(queued.results,[null,null],'one follow-up persist for both overlapping requests');
    await page.evaluate(()=>localStorage.removeItem('terminal-velocity.language'));
    await page.reload();await page.waitForFunction(()=>!document.querySelector('#start').disabled);
    assert.equal(await page.evaluate(()=>Module._web_saved_language()),0,'the queued language must reach IndexedDB before the next timer');
    assert.equal(await page.evaluate(()=>JSON.parse(Module.FS.readFile('/persist/accounts.json',{encoding:'utf8'})).fixture.revision),42);
    console.log('PASS overlapping autosave and account metadata flush persist in order across reload');
    await page.locator('#start').click();
    await page.locator('[data-language="pl"]').click();
    await page.waitForFunction(()=>!syncing);
    await page.evaluate(()=>localStorage.removeItem('terminal-velocity.language'));
    await page.reload();await page.waitForFunction(()=>!document.querySelector('#start').disabled);
    assert.equal(await page.locator('html').getAttribute('lang'),'pl','saved Polish beats English browser even without localStorage');
    await page.locator('[data-language="en"]').click();
    await page.reload();await page.waitForFunction(()=>!document.querySelector('#start').disabled);
    assert.equal(await page.locator('html').getAttribute('lang'),'en','explicit pre-start choice survives reload over saved Polish');
    assert.equal(await page.evaluate(()=>Module._web_saved_language()),0,'pre-start choice did not replace game progress');
    console.log('PASS explicit pre-start language survives reload over an existing save without replacing its progress');
    await page.locator('#start').click();await page.waitForFunction(()=>localStorage.getItem('terminal-velocity.pending-language')===null);
    await page.waitForFunction(()=>!syncing);
    await page.reload();await page.waitForFunction(()=>!document.querySelector('#start').disabled);
    assert.equal(await page.locator('html').getAttribute('lang'),'en','explicit English persists across reload');
    await page.locator('#start').click();await page.waitForTimeout(100);
    const sample=fs.readFileSync(path.join(root,'tests/fixtures/iceberg-v4.dat'));
    const importButton=page.locator('#import-save');
    const openPicker=()=>Promise.all([page.waitForEvent('filechooser'),importButton.click()]).then(([picker])=>picker);
    // A chooser that never reports a file or cancellation must leave play available.
    await openPicker();
    const liveTicks=await page.evaluate(async()=>{
      let ticks=0;
      const tick=Module._web_tick;
      Module._web_tick=(dt,...args)=>{if(dt>0)ticks++;return tick(dt,...args);};
      try {
        for(let i=0;i<6;i++)await new Promise(requestAnimationFrame);
        return ticks;
      } finally {Module._web_tick=tick;}
    });
    assert.ok(liveTicks>0,'the chooser alone cannot freeze the game');
    assert.equal(await importButton.isEnabled(),true);
    assert.equal(await page.evaluate(()=>canvas===document.activeElement),true);
    const invalidPicker=await openPicker();
    const acceptInvalid=page.waitForEvent('dialog').then(dialog=>dialog.accept());
    await invalidPicker.setFiles({name:'broken.iceberg',mimeType:'application/octet-stream',buffer:Buffer.from('broken save')});
    await acceptInvalid;
    await page.waitForFunction(()=>saveStatus.textContent==='Invalid save file');
    await page.evaluate(()=>{
      const save=Module._web_save;
      window.savesAfterRejectedImport=0;
      Module._web_save=(...args)=>{window.savesAfterRejectedImport++;return save(...args);};
    });
    await page.waitForFunction(()=>window.savesAfterRejectedImport>0&&!syncing);
    const saveMessage=page.locator('#save-status');
    assert.equal(await saveMessage.textContent(),'Invalid save file','autosave preserves the import error');
    await page.locator('[data-language="pl"]').click();
    await page.waitForFunction(()=>!syncing);
    assert.equal(await saveMessage.textContent(),'Nieprawidłowy plik zapisu','a pending import error follows the selected language');
    await page.locator('[data-language="en"]').click();

    const blockedPicker=await openPicker();
    await page.evaluate(()=>{
      const importSave=Module._web_import_save;
      const fs=Module.FS;
      window.restoreImportWrite=()=>{Module._web_import_save=importSave;};
      Module._web_import_save=()=>{
        const rename=fs.rename;
        fs.rename=(from,to)=>{
          if(to==='/persist/progress.dat.iceberg')throw new fs.ErrnoError(28);
          return rename.call(fs,from,to);
        };
        try{return importSave();}
        finally{fs.rename=rename;}
      };
    });
    const acceptBlocked=page.waitForEvent('dialog').then(dialog=>dialog.accept());
    await blockedPicker.setFiles({name:'polish.iceberg',mimeType:'application/octet-stream',buffer:sample});
    await acceptBlocked;
    await page.waitForFunction(()=>!importPending&&saveStatus.textContent==='Could not write the imported save');
    assert.equal(await page.locator('html').getAttribute('lang'),'en','the failed replacement leaves the old session active');
    await page.evaluate(()=>window.restoreImportWrite());

    await page.keyboard.press('u');
    await page.waitForFunction(()=>document.querySelector('#sound').getAttribute('aria-pressed')==='false');
    const picker=await openPicker();
    await page.evaluate(()=>{
      const runImport=Module._web_import_save;
      Module._web_import_save=()=>{
        window.replacedSave=Module.FS.readFile('/persist/progress.dat.iceberg',{encoding:'utf8'});
        const result=runImport();
        window.importBackup=Module.FS.readFile('/persist/progress.dat.iceberg.bak',{encoding:'utf8'});
        return result;
      };
    });
    const confirmation=page.waitForEvent('dialog').then(dialog=>{
      assert.equal(dialog.type(),'confirm');
      return dialog.accept();
    });
    await picker.setFiles({name:'polish.iceberg',mimeType:'application/octet-stream',buffer:sample});
    await confirmation;
    await page.waitForFunction(()=>!importPending&&!document.querySelector('#export-previous-save').hidden&&document.documentElement.lang==='pl');
    const imported=await page.evaluate(()=>({
      previous:window.replacedSave,
      backup:window.importBackup,
      manual:Module.FS.readFile('/persist/before-manual-import.iceberg',{encoding:'utf8'})
    }));
    assert.equal(imported.backup,imported.previous,'import retains the previous profile as rollback');
    assert.equal(imported.manual,imported.previous,'the previous-save download uses that same profile');
    await page.waitForFunction(()=>!syncing);
    assert.equal(await saveMessage.textContent(),'Zapis w tej przeglądarce ✓','a valid retry clears the previous import error');
    assert.equal(await page.locator('#sound').getAttribute('aria-pressed'),'false','import keeps the browser muted');
    await page.reload();
    await page.waitForFunction(()=>!document.querySelector('#start').disabled);
    await page.locator('#start').click();
    assert.equal(await page.locator('#export-previous-save').isVisible(),true);
    await page.waitForFunction(()=>!!(uiFlags&256));
    assert.equal(await page.locator('#sound').getAttribute('aria-pressed'),'false','reload keeps the browser muted');
    assert.equal(await page.evaluate(()=>Module.FS.readFile('/persist/before-manual-import.iceberg',{encoding:'utf8'})),imported.previous);
    console.log('PASS silent file picker dismissal, real Polish save import and rollback after reload');
    await page.waitForFunction(()=>!syncing);
    await page.evaluate(()=>{
      const fs=Module.FS,save='/persist/progress.dat.iceberg';
      fs.rename(save,save+'.test');
      fs.mkdir(save);
      fs.writeFile(save+'/blocked','test');
      syncSave();
    });
    await page.waitForFunction(()=>!syncing);
    assert.equal(await saveMessage.textContent(),'Błąd zapisu','IndexedDB completion cannot hide a game save write failure');
    const [recovery]=await Promise.all([page.waitForEvent('download'),page.locator('#export-save').click()]);
    assert.match(fs.readFileSync(await recovery.path(),'utf8'),/^TVICE6 /,'the live session remains downloadable');
    assert.equal(await saveMessage.textContent(),'Błąd zapisu','a recovery download preserves the storage warning');
    await page.evaluate(()=>{
      const fs=Module.FS,save='/persist/progress.dat.iceberg';
      fs.unlink(save+'/blocked');
      fs.rmdir(save);
      fs.rename(save+'.test',save);
      syncSave();
    });
    await page.waitForFunction(()=>!syncing&&saveStatus.textContent==='Zapis w tej przeglądarce ✓');
    console.log('PASS browser save status reports an engine write failure and recovers on a successful save');
   }
   console.log('PASS',locale,'fresh boot/game language and entry control');await context.close();
  }
  const expedition=await browser.newPage({locale:'en-US',viewport:{width:1280,height:900}});
  expedition.on('pageerror',e=>errors.push(String(e)));
  await expedition.goto(url);
  await expedition.locator('[data-language="en"]').click();
  await expedition.locator('#start').click();
  await expedition.waitForFunction(()=>started&&!accountBooting);
  const desktopToolbar=expedition.locator('#toolbar');
  const desktopMap=desktopToolbar.locator('[data-tap="11"]');
  const desktopSwitch=desktopToolbar.locator('.map-view-switch');
  const desktopBrowse=desktopToolbar.locator('.route-browse');
  await desktopMap.click();
  await expedition.waitForFunction(()=>uiFlags&128);
  assert.equal(await desktopSwitch.isVisible(),false,'the Campaign has its own map controls');
  assert.equal(await desktopBrowse.isVisible(),false);
  await desktopToolbar.locator('[data-tap="12"]').click();
  await expedition.waitForFunction(()=>uiFlags&1);
  await expedition.keyboard.press('Digit3');
  await expedition.keyboard.press('Enter');
  await expedition.waitForFunction(()=>(uiFlags&(8192|128|1))===8192);
  await expedition.evaluate(()=>{
   const draw=window.tvFrame;
   window.tvFrame=(cells,w,h)=>{
    if(uiFlags&(128|1)){
     const text=Array.from({length:h},(_,y)=>Array.from({length:w},(_,x)=>
      String.fromCodePoint(cells[4*(y*w+x)])).join('')).join('\n');
     if(uiFlags&1)window.desktopMenuFrame=text;
     else window.desktopMapFrame=text;
    }
    draw(cells,w,h);
   };
  });
  await desktopMap.click();
  await expedition.waitForFunction(()=>uiFlags&128);
  const nextRoom=desktopBrowse.getByRole('button',{name:'Next room'});
  const previousRoom=desktopBrowse.getByRole('button',{name:'Previous room'});
  await nextRoom.click();
  await expedition.waitForFunction(()=>window.desktopMapFrame?.includes('Room 2:'));
  await nextRoom.hover();
  await expedition.mouse.down();
  try {
   await expedition.waitForFunction(()=>window.desktopMapFrame?.includes('Room 8:'),null,{timeout:5000});
  } finally {await expedition.mouse.up();}
  await previousRoom.click();
  await expedition.waitForFunction(()=>window.desktopMapFrame?.includes('Room 7:'));
  assert.equal(await desktopSwitch.getAttribute('aria-label'),'Show room map');
  await desktopSwitch.click();
  await expedition.waitForFunction(()=>uiFlags&(1<<17));
  assert.equal(await desktopBrowse.isVisible(),false);
  assert.equal(await desktopSwitch.getAttribute('aria-label'),'Show route map');
  await desktopSwitch.press('Enter');
  await expedition.waitForFunction(()=>(uiFlags&(128|(1<<17)))===128);
  await expedition.waitForFunction(()=>window.desktopMapFrame?.includes('Room 7:'));
  await desktopMap.click();
  await expedition.waitForFunction(()=>!(uiFlags&128));
  assert.equal(await desktopSwitch.isVisible(),false);
  assert.equal(await desktopBrowse.isVisible(),false);
  const pickerActions=expedition.locator('#menu-selection-actions');
  assert.equal(await pickerActions.isVisible(),false);
  await desktopToolbar.locator('[data-tap="12"]').click();
  await expedition.waitForFunction(()=>window.desktopMenuFrame?.includes('▸ 1  '));
  assert.equal(await pickerActions.getByRole('button',{name:'Back'}).isVisible(),false);
  await pickerActions.getByRole('button',{name:'Previous option'}).click();
  await expedition.waitForFunction(()=>window.desktopMenuFrame?.includes('▸ 6  '));
  await pickerActions.getByRole('button',{name:'Previous option'}).click();
  await expedition.waitForFunction(()=>window.desktopMenuFrame?.includes('▸ 5  SETTINGS'));
  await pickerActions.getByRole('button',{name:'Select'}).click();
  await desktopToolbar.getByRole('button',{name:'Back',exact:true}).click();
  await expedition.waitForFunction(()=>(uiFlags&(1|16))===1);
  assert.equal(await desktopToolbar.getByRole('button',{name:'Pause',exact:true}).isVisible(),true);
  await pickerActions.getByRole('button',{name:'Select'}).click();
  await expedition.locator('#settings-keys [data-tap="25"]').click();
  await pickerActions.getByRole('button',{name:'Next option'}).click();
  await pickerActions.getByRole('button',{name:'Select'}).click();
  await expedition.waitForFunction(()=>selectedSlot===2&&!(uiFlags&(1<<23)));
  assert.equal(await pickerActions.isVisible(),true,'switching slots returns to the main menu');
  assert.equal(await pickerActions.getByRole('button',{name:'Back'}).isVisible(),false);
  await expedition.close();
  console.log('PASS desktop Expedition mouse navigation browses rooms and survives switching views');
  console.log('PASS desktop mouse controls reach Settings and select a save slot');

  const mobile=await browser.newContext({locale:'pl-PL',viewport:{width:390,height:844},deviceScaleFactor:3,isMobile:true,hasTouch:true});
  const page=await mobile.newPage();page.on('pageerror',e=>errors.push(String(e)));await page.goto(url);await page.waitForFunction(()=>!document.querySelector('#start').disabled);
  await page.locator('[data-language="en"]').click();await page.locator('#start').click();await page.waitForTimeout(150);
  await page.waitForFunction(()=>!(uiFlags&1)&&(uiFlags&256));await page.waitForTimeout(650);await page.screenshot({path:path.join(out,'web-mobile-opening.png')});
  const inspectControl=page.locator('#touch-inspect');
  const touchDirectionNames=()=>Promise.all([2,3].map(key=>
   page.locator('#touch [data-key="'+key+'"]').getAttribute('aria-label')));
  await inspectControl.tap();
  await page.waitForFunction(()=>!!(uiFlags&8));
  assert.equal(await inspectControl.getAttribute('aria-label'),'Close inspection');
  assert.deepEqual(await touchDirectionNames(),['Previous page','Next page']);
  await inspectControl.tap();
  await page.waitForFunction(()=>!(uiFlags&8));
  assert.equal(await inspectControl.getAttribute('aria-label'),'Inspect');
  const touchSession=await mobile.newCDPSession(page);
  const [left,up,right]=await Promise.all([0,2,1].map(async key=>{
   const box=await page.locator('#touch [data-key="'+key+'"]').boundingBox();
   return {x:box.x+box.width/2,y:box.y+box.height/2,id:1};
  }));
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[left]});
  await page.waitForFunction(()=>held===1);
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[up]});
  await page.waitForFunction(()=>held===0);
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[right]});
  await page.waitForFunction(()=>held===2);
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
  await page.waitForFunction(()=>held===0&&!document.querySelector('#touch .held'));
  console.log('PASS a touch slide reverses direction and releases without triggering a jump');
  // Put a whole slide between game frames while Chrome still dispatches touch events.
  await page.evaluate(()=>{
   const raf=window.requestAnimationFrame;
   window.requestAnimationFrame=callback=>{
    if(callback.name!=='frame')return raf.call(window,callback);
    window.resumeTouchFrames=()=>{
     window.requestAnimationFrame=raf;
     raf.call(window,callback);
    };
    return 0;
   };
  });
  await page.waitForFunction(()=>typeof window.resumeTouchFrames==='function');
  try {
   const touch=(type,touchPoints)=>touchSession.send('Input.dispatchTouchEvent',{type,touchPoints});
   const touchState=()=>page.evaluate(()=>({held,pressed}));
   await touch('touchStart',[left]);
   await touch('touchMove',[right]);
   await touch('touchEnd',[]);
   assert.deepEqual(await touchState(),{held:0,pressed:2},'a completed slide queues its final direction');
   await page.evaluate(()=>clearPresses());
   await touch('touchStart',[left]);
   await page.keyboard.press('ArrowRight');
   await touch('touchMove',[up]);
   await touch('touchEnd',[]);
   assert.deepEqual(await touchState(),{held:0,pressed:2},'leaving the direction buttons keeps only the independent keyboard press');
   await page.evaluate(()=>clearPresses());
   await touch('touchStart',[left]);
   await touch('touchMove',[up]);
   await touch('touchEnd',[]);
   assert.deepEqual(await touchState(),{held:0,pressed:0},'a contact released outside the direction controls leaves no queued aim');
  } finally {
   await page.evaluate(()=>window.resumeTouchFrames());
  }
  console.log('PASS a quick touch slide drops the obsolete direction before the next frame');
  await page.keyboard.press('j');
  await page.waitForFunction(()=>(uiFlags&(64|128))===128);
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[right]});
  await page.waitForFunction(()=>held===2);
  await page.keyboard.press('Tab');
  await page.waitForFunction(()=>(uiFlags&(64|128))===64);
  const directionState=()=>page.evaluate(()=>({held,highlight:!!document.querySelector('#touch .held')}));
  assert.deepEqual(await directionState(),{held:0,highlight:false},'opening the quest book clears the map contact');
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[left]});
  assert.deepEqual(await directionState(),{held:0,highlight:false},'the old contact cannot activate another direction');
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[left]});
  await page.waitForFunction(()=>held===1);
  await touchSession.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
  await page.keyboard.press('Tab');
  await page.waitForFunction(()=>!(uiFlags&64)&&held===0);
  await touchSession.detach();
  console.log('PASS switching from the map to the quest book clears held touch controls');
  await page.evaluate(()=>{
   window.touchControlEdges=[];
   const tick=Module._web_tick;
   window.restoreTouchControlTick=()=>{Module._web_tick=tick;};
   Module._web_tick=(dt,held,pressed,...rest)=>{
    if(pressed)window.touchControlEdges.push(pressed);
    return tick(dt,held,pressed,...rest);
   };
  });
  const dashControl=page.locator('#touch [data-key="5"]'),jumpControl=page.locator('#touch [data-key="4"]');
  const takeTouchControlEdges=()=>page.evaluate(async()=>{
   for(let i=0;i<4;i++)await new Promise(requestAnimationFrame);
   return window.touchControlEdges.splice(0);
  });
  await dashControl.focus();
  await page.keyboard.press('Tab');
  assert.equal(await jumpControl.evaluate(button=>button===document.activeElement),true);
  await page.keyboard.press('Space');
  assert.deepEqual(await takeTouchControlEdges(),[1<<4],'Space activates the focused Jump button');
  assert.equal(await page.evaluate(()=>document.activeElement===canvas),true);
  await dashControl.focus();
  await page.keyboard.press('Enter');
  assert.deepEqual(await takeTouchControlEdges(),[1<<5],'Enter activates the focused Dash button');
  await page.evaluate(()=>document.addEventListener('contextmenu',e=>e.preventDefault(),{once:true}));
  await dashControl.click({button:'right'});
  await dashControl.click({button:'middle'});
  assert.deepEqual(await takeTouchControlEdges(),[],'secondary mouse buttons do not dash');
  // Isolate chord cleanup from Chromium's additional blur on right-click.
  await page.evaluate(()=>canvas.removeEventListener('blur',clearInput));
  await page.mouse.move(right.x,right.y);
  await page.mouse.down();
  await page.waitForFunction(()=>held===2);
  await page.mouse.down({button:'right'});
  await page.mouse.up();
  await page.waitForFunction(()=>held===0&&!document.querySelector('#touch .held'));
  await page.mouse.up({button:'right'});
  await page.evaluate(()=>canvas.addEventListener('blur',clearInput));
  assert.deepEqual(await takeTouchControlEdges(),[1<<1],'releasing left ends the chord with a single movement press');
  await jumpControl.tap();
  assert.deepEqual(await takeTouchControlEdges(),[1<<4],'a touch tap still sends Jump once');
  await page.evaluate(()=>window.restoreTouchControlTick());
  console.log('PASS compact game controls activate with keyboard and touch');
  for(const size of [{width:280,height:653},{width:360,height:260},{width:390,height:844},{width:844,height:390},{width:760,height:600},{width:1920,height:1080}]){
   await page.setViewportSize(size);await page.waitForTimeout(100);
   const ratio=await page.locator('#game').evaluate(c=>({bitmap:c.width/c.height,display:c.getBoundingClientRect().width/c.getBoundingClientRect().height}));
   assert.ok(Math.abs(ratio.bitmap-ratio.display)<.01,'canvas aspect preserved '+JSON.stringify(size));
   assert.equal(await page.locator('[data-language="en"]').isVisible(),true,'entry language visible '+JSON.stringify(size));
   if(size.width<=360){
    const controls=await page.locator('#touch').evaluate(pad=>({
     page:document.documentElement.scrollWidth,
     overflow:[...pad.querySelectorAll('button:not([hidden])')].some(button=>button.scrollWidth>button.clientWidth)
    }));
    assert.equal(controls.page,size.width,'touch controls leave the narrow viewport at actual size');
    assert.equal(controls.overflow,false,'touch labels fit their buttons');
   }
  }
  for(const fallback of [false,true]){
   if(fallback)await page.evaluate(()=>{
    document.querySelector('#playfield').requestFullscreen=async()=>{throw Error('fullscreen unavailable');};
   });
   await page.setViewportSize({width:320,height:700});
   await page.locator('#fullscreen').tap();
   await page.waitForFunction(()=>expanded());
   assert.equal(await page.evaluate(()=>!!document.fullscreenElement),!fallback);
   const tools=page.locator('#fullscreen-tools');
   await tools.locator('[data-tap="13"]').waitFor({state:'visible'});
   assert.equal(await tools.locator('#save-status').isVisible(),true);
   const [fullscreenSave]=await Promise.all([
    page.waitForEvent('download'),tools.locator('#export-save').tap(),
   ]);
   assert.match(fs.readFileSync(await fullscreenSave.path(),'utf8'),/^TVICE6 /);
   await page.locator('#game').focus();

   const sound=tools.locator('#sound');
   await page.keyboard.press('u');
   await page.waitForFunction(()=>document.querySelector('#sound').getAttribute('aria-pressed')==='false');
   await sound.tap();
   await page.waitForFunction(()=>document.querySelector('#sound').getAttribute('aria-pressed')==='true');
   if(fallback){
    await page.waitForFunction(()=>document.activeElement===canvas);
    await page.keyboard.press('Shift+Tab');
    assert.equal(await page.evaluate(()=>!!document.activeElement.closest('header,footer')),false,
     'reverse Tab skips the covered page controls in fallback fullscreen');
   }
   const language=tools.locator('[data-language-toggle]');
   await language.tap();
   await page.waitForFunction(()=>document.documentElement.lang==='pl');
   await language.tap();
   await page.waitForFunction(()=>document.documentElement.lang==='en');
   for(const [key,flag] of [[11,128],[10,64]]){
    const button=tools.locator('[data-tap="'+key+'"]');
    assert.equal(await button.count(),1,'fullscreen keeps Map and Quests in reach');
    await button.tap();
    await page.waitForFunction(flag=>!!(uiFlags&flag),flag);
    if(flag===128) {
     const route=page.locator('#touch-inspect');
     const use=page.locator('#touch-use');
     if(await page.evaluate(()=>!!(uiFlags&2048))){
      assert.equal(await route.innerText(),'Room');
      await route.tap();
      await page.waitForFunction(()=>!!(uiFlags&(1<<17)));
     }
     if(await page.evaluate(()=>!!(uiFlags&(1<<17)))){
      assert.equal(await route.innerText(),'Districts');
      await route.tap();
      await page.waitForFunction(()=>!(uiFlags&(2048|(1<<17))));
     }
     assert.equal(await route.innerText(),'Route');
     assert.equal(await use.getAttribute('aria-label'),'Close map');
     assert.deepEqual(await touchDirectionNames(),['Previous district','Next district']);
     await route.tap();
     await page.waitForFunction(()=>!!(uiFlags&2048));
     assert.equal(await route.innerText(),'Room');
     assert.deepEqual(await touchDirectionNames(),['Previous leg','Next leg']);
     await route.tap();
     await page.waitForFunction(()=>!!(uiFlags&(1<<17)));
     assert.equal(await route.innerText(),'Districts');
     assert.equal(await use.getAttribute('aria-label'),'Close map');
     await use.tap();
     await page.waitForFunction(()=>!(uiFlags&128));
     await button.tap();
     await page.waitForFunction(()=>(uiFlags&(128|(1<<17)))===(128|(1<<17)));
     assert.equal(await route.innerText(),'Districts','fullscreen reopens the room map');
     await use.tap();
    }else {
     assert.equal(await inspectControl.getAttribute('aria-label'),'Next quest tab');
     assert.deepEqual(await touchDirectionNames(),['Previous entry','Next entry']);
     await button.tap();
    }
    await page.waitForFunction(flag=>!(uiFlags&flag),flag);
    assert.equal(await inspectControl.getAttribute('aria-label'),'Inspect');
    assert.equal(await page.locator('#touch-use').getAttribute('aria-label'),'Use');
   }
   await tools.locator('[data-tap="10"]').tap();
   await page.waitForFunction(()=>!!(uiFlags&64));
   await tools.locator('[data-tap="12"]').tap();
   await page.waitForFunction(()=>(uiFlags&(1|64))===(1|64));
   const menuConfirm=page.locator('#touch-use');
   assert.equal(await menuConfirm.getAttribute('aria-label'),'Confirm');
   await menuConfirm.tap();
   await page.waitForFunction(()=>!(uiFlags&1));
   const bounds=await tools.locator('button:visible').evaluateAll(buttons=>buttons.every(button=>{
    const rect=button.getBoundingClientRect();
    return rect.left>=0&&rect.right<=window.innerWidth&&rect.bottom<=window.innerHeight;
   }));
   assert.ok(bounds,'fullscreen tools fit a narrow phone');
   await page.screenshot({path:path.join(out,fallback?'web-fullscreen-fallback.png':'web-fullscreen-touch.png')});
   await tools.locator('[data-tap="12"]').tap();
   await page.waitForFunction(()=>!!(uiFlags&1));
   await page.keyboard.press('Digit5');
   await page.locator('#touch-use').tap();
   await page.waitForFunction(()=>!!(uiFlags&16));
   const settings=tools.locator('#settings-keys');
   assert.equal(await settings.count(),1,'fullscreen contains the settings buttons');
   await page.evaluate(()=>{
    const render=window.tvFrame;
    window.tvFrame=(cells,w,h)=>{
     window.volumeLine=Array.from({length:h},(_,r)=>
      Array.from({length:w},(_,c)=>String.fromCodePoint(cells[(r*w+c)*4])).join('')
     ).find(line=>line.includes('Music:'));
     render(cells,w,h);
    };
   });
   await page.waitForFunction(()=>!!window.volumeLine);
   const oldVolume=await page.evaluate(()=>window.volumeLine);
   await settings.locator('[data-tap="20"]').tap();
   await page.waitForFunction(old=>window.volumeLine&&window.volumeLine!==old,oldVolume);
   const focusPause=settings.locator('#focus-pause');
   const initialFocusPause=await focusPause.getAttribute('aria-pressed');
   if(initialFocusPause==='true')await focusPause.tap();
   await page.waitForFunction(()=>!(uiFlags&4096));
   await page.screenshot({path:path.join(out,fallback?"web-settings-fallback.png":"web-settings-fullscreen.png")});
   await page.locator('#touch').getByRole('button',{name:'Back',exact:true}).tap();
   await page.waitForFunction(()=>!(uiFlags&16));
   await page.keyboard.press('Digit1');
   await page.locator('#touch-use').tap();
   await page.waitForFunction(()=>!(uiFlags&1));
   const paused=await page.evaluate(()=>{
    window.dispatchEvent(new Event('blur'));
    window.dispatchEvent(new Event('focus'));
    return !!(uiFlags&2);
   });
   assert.equal(paused,false,'turning off focus pause keeps the game running');
   await page.locator('#leave-fullscreen').tap();
   await page.waitForFunction(()=>!expanded());
   await page.locator("footer #settings-keys").waitFor({state:"attached"});
   await page.locator('#toolbar [data-language-toggle]').waitFor({state:'visible'});
   await page.locator('#toolbar [data-tap="13"]').waitFor({state:'visible'});
  }
  console.log('PASS mobile native and fallback fullscreen Map, Quests and settings controls');
  console.log('PASS mobile, landscape, resized and large canvas preserve proportions; language remains reachable');
  await page.setViewportSize({width:568,height:320});
  const toolbar=page.locator('#toolbar');
  const swipeStart=await toolbar.evaluate(row=>{
   row.scrollLeft=0;
   const edge=row.getBoundingClientRect().right;
   const button=[...row.querySelectorAll('button:not([hidden])')].reverse().find(b=>b.getBoundingClientRect().right<=edge);
   const box=button.getBoundingClientRect();
   row.addEventListener('click',()=>{window.landscapeToolbarClicked=true;},{once:true});
   return {x:box.left+box.width/2,y:box.top+box.height/2,overflow:row.scrollWidth-row.clientWidth};
  });
  assert.ok(swipeStart.overflow>0,'the short viewport needs a scrollable toolbar');
  const swipeSession=await mobile.newCDPSession(page);
  await swipeSession.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{x:swipeStart.x,y:swipeStart.y}]});
  for(let i=1;i<=12;i++){
   await swipeSession.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[{x:swipeStart.x-i*12,y:swipeStart.y}]});
   await page.evaluate(()=>new Promise(requestAnimationFrame));
  }
  await swipeSession.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
  await swipeSession.detach();
  await page.waitForFunction(()=>{
   const row=document.querySelector('#toolbar');
   return row.scrollLeft>=row.scrollWidth-row.clientWidth-2;
  });
  assert.equal(await page.evaluate(()=>!!window.landscapeToolbarClicked),false,'swiping over a button does not activate it');
  console.log('PASS a real landscape swipe reaches the end of the toolbar without pressing a button');
  await page.setViewportSize({width:390,height:844});
  const action=page.locator('#touch-use');
  const openMenu=async()=>{
   await page.locator('footer #toolbar [data-tap="12"]').tap();
   await page.waitForFunction(()=>!!(uiFlags&1));
   assert.deepEqual(await touchDirectionNames(),['Previous option','Next option']);
  };
  await openMenu();
  await page.keyboard.press('Digit3');
  await page.waitForFunction(()=>uiFlags&16384);
  const newRun=page.locator('#toolbar [data-tap="13"]');
  assert.equal(await newRun.textContent(),'New run');
  assert.equal(await newRun.getAttribute('aria-label'),'Start a new run');
  await action.tap();
  await page.waitForFunction(()=>(uiFlags&(8192|1))===8192);
  assert.equal(await newRun.textContent(),'Retry');
  assert.equal(await newRun.getAttribute('aria-label'),null);
  const expeditionMap=page.locator('footer #toolbar [data-tap="11"]');
  await page.evaluate(()=>{
   const render=window.tvFrame;
   window.tvFrame=(cells,w,h)=>{
    window.expeditionFrame=Array.from({length:h},(_,y)=>Array.from({length:w},(_,x)=>
     String.fromCodePoint(cells[4*(y*w+x)])).join('')).join('\n');
    render(cells,w,h);
   };
  });
  for(const roomMap of [false,true]){
   await expeditionMap.tap();
   await page.waitForFunction(()=>!!(uiFlags&128));
   assert.equal(await page.locator('#toolbar .map-view-switch').isVisible(),false,
    'touch uses its existing room and route control');
   assert.equal(await page.locator('#toolbar .route-browse').isVisible(),false);
   if(roomMap){
    await inspectControl.tap();
    await page.waitForFunction(()=>!!(uiFlags&(1<<17)));
    assert.deepEqual(await touchDirectionNames(),['Up','Down']);
   }else{
    assert.deepEqual(await touchDirectionNames(),['Previous room','Next room']);
    await page.locator('#touch').getByRole('button',{name:'Next room'}).tap();
    await page.waitForFunction(()=>window.expeditionFrame?.includes('Room 2:'));
    await page.keyboard.down('ArrowDown');
    try{
     await page.waitForFunction(()=>window.expeditionFrame?.includes('Room 8:'),null,{timeout:5000});
    }finally{await page.keyboard.up('ArrowDown');}
   }
   await action.tap();
   await page.waitForFunction(()=>!(uiFlags&128));
   assert.equal(await action.getAttribute('aria-label'),'Use',
    'closing either Expedition map restores the touch interaction action');
   assert.deepEqual(await touchDirectionNames(),['Up / jump','Down']);
  }

  await openMenu();
  await page.keyboard.press('Digit5');
  await page.waitForFunction(()=>!(uiFlags&16384));
  assert.equal(await newRun.isVisible(),false,'the run action is hidden for other menu choices');
  await action.tap();
  await page.waitForFunction(()=>!!(uiFlags&16));
  await page.keyboard.press('Digit6');
  await page.waitForFunction(()=>!!(uiFlags&(1<<23)));
  assert.equal(await action.getAttribute('aria-label'),'Select');
  assert.equal(await inspectControl.getAttribute('aria-label'),'Back');
  assert.deepEqual(await touchDirectionNames(),['Previous option','Next option']);
  await inspectControl.tap();
  await page.waitForFunction(()=>(uiFlags&(16|(1<<23)))===16);
  await page.keyboard.press('Digit6');
  await page.waitForFunction(()=>uiFlags&(1<<23));
  await page.evaluate(()=>{
   const render=window.tvFrame;
   window.tvFrame=(cells,w,h)=>{
    window.tourPickerHeading=Array.from({length:w},(_,x)=>String.fromCodePoint(cells[4*(2*w+x)])).join('');
    render(cells,w,h);
   };
  });
  await page.keyboard.press('Digit4');
  await action.tap();
  await page.waitForFunction(()=>window.tourPickerHeading?.includes('DEBUG TOUR'));
  await inspectControl.tap();
  await page.waitForFunction(()=>window.tourPickerHeading?.includes('SAVE SLOTS'));
  await action.tap();
  await page.waitForFunction(()=>window.tourPickerHeading?.includes('DEBUG TOUR'));
  await action.tap();
  await page.waitForFunction(()=>(uiFlags&((1<<22)|1))===(1<<22));
  assert.equal(await action.getAttribute('aria-label'),'Use','Debug Tour play supports touch interactions');
  assert.equal(await inspectControl.getAttribute('aria-label'),'Inspect');
  await page.evaluate(()=>{
   const tick=Module._web_tick;
   Module._web_tick=(dt,held,pressed,...args)=>{
    if(pressed&64)window.tourUseReceived=true;
    tick(dt,held,pressed,...args);
   };
  });
  await action.tap();
  await page.waitForFunction(()=>window.tourUseReceived===true);
  console.log('PASS Expedition maps and Debug Tour restore the normal touch Use action');
  await mobile.close();
  assert.deepEqual(errors,[],'browser errors');console.log(visualOnly?'PASS current Chromium/WASM visual smoke':'PASS real Chromium/WASM entry, save-language restoration and responsive rendering');
 }finally{await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(e=>{console.error(e);server.close();process.exitCode=1;});

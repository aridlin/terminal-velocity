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
    await page.keyboard.press('j');
    await page.waitForFunction(()=>!!(uiFlags&128)&&entryFrame.includes('JOURNEY MAP'));
    assert.equal(await page.evaluate(()=>entryFrame.includes('A MOMENT TO BREATHE')),false);
    await page.keyboard.press('p');
    await page.waitForFunction(()=>(uiFlags&(2|128))===2&&entryFrame.includes('A MOMENT TO BREATHE'));
    await page.locator('footer [data-tap="10"]').click();
    await page.waitForFunction(()=>!!(uiFlags&64)&&entryFrame.includes('QUEST BOOK'));
    assert.equal(await page.evaluate(()=>entryFrame.includes('A MOMENT TO BREATHE')),false);
    await page.keyboard.press('Tab');
    await page.waitForFunction(()=>(uiFlags&(2|64))===2);
    console.log('PASS map and quests stay readable over pause and close back to the paused game');
    await page.keyboard.press('Enter');
    await page.waitForFunction(()=>!(uiFlags&2));


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
    await page.reload();
    await page.waitForFunction(()=>!document.querySelector('#start').disabled);
    await page.locator('#start').click();
    assert.equal(await page.locator('#export-previous-save').isVisible(),true);
    assert.equal(await page.evaluate(()=>Module.FS.readFile('/persist/before-manual-import.iceberg',{encoding:'utf8'})),imported.previous);
    console.log('PASS silent file picker dismissal, real Polish save import and rollback after reload');
   }
   console.log('PASS',locale,'fresh boot/game language and entry control');await context.close();
  }
  const mobile=await browser.newContext({locale:'pl-PL',viewport:{width:390,height:844},deviceScaleFactor:3,isMobile:true,hasTouch:true});
  const page=await mobile.newPage();page.on('pageerror',e=>errors.push(String(e)));await page.goto(url);await page.waitForFunction(()=>!document.querySelector('#start').disabled);
  await page.locator('[data-language="en"]').click();await page.locator('#start').click();await page.waitForTimeout(150);
  await page.waitForFunction(()=>!(uiFlags&1)&&(uiFlags&256));await page.waitForTimeout(650);await page.screenshot({path:path.join(out,'web-mobile-opening.png')});
  const inspectControl=page.locator('#touch-inspect');
  await inspectControl.tap();
  await page.waitForFunction(()=>!!(uiFlags&8));
  assert.equal(await inspectControl.getAttribute('aria-label'),'Close inspection');
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
  await touchSession.detach();
  console.log('PASS a touch slide reverses direction and releases without triggering a jump');
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
   for(const [key,flag] of [[11,128],[10,64]]){
    const button=tools.locator('[data-tap="'+key+'"]');
    assert.equal(await button.count(),1,'fullscreen keeps Map and Quests in reach');
    await button.tap();
    await page.waitForFunction(flag=>!!(uiFlags&flag),flag);
    if(flag===128) {
     const route=page.locator('#touch-inspect');
     const use=page.locator('#touch-use');
     if(await page.evaluate(()=>!!(uiFlags&2048))){
      assert.equal(await route.innerText(),'Districts');
      await route.tap();
      await page.waitForFunction(()=>!(uiFlags&2048));
     }
     assert.equal(await route.innerText(),'Route');
     assert.equal(await use.getAttribute('aria-label'),'Confirm');
     await route.tap();
     await page.waitForFunction(()=>!!(uiFlags&2048));
     assert.equal(await route.innerText(),'Districts');
     assert.equal(await use.getAttribute('aria-label'),'Close map');
     await use.tap();
    }else {
     assert.equal(await inspectControl.getAttribute('aria-label'),'Next quest tab');
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
   await page.locator('#touch-use').tap();
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
  await mobile.close();
  assert.deepEqual(errors,[],'browser errors');console.log(visualOnly?'PASS current Chromium/WASM visual smoke':'PASS real Chromium/WASM entry, save-language restoration and responsive rendering');
 }finally{await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(e=>{console.error(e);server.close();process.exitCode=1;});

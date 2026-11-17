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
  await loadingPage.goto(url,{waitUntil:'commit'});await loadingPage.waitForFunction(()=>typeof selectLanguage==='function');
  await loadingPage.locator('[data-language="pl"]').click();resumeModule();
  await loadingPage.waitForFunction(()=>!document.querySelector('#start').disabled);
  assert.equal(await loadingPage.locator('html').getAttribute('lang'),'pl','explicit choice during loading wins');
  await loadingContext.close();console.log('PASS explicit language choice during module loading');
  }
  for(const [locale,expected] of (visualOnly?[['en-US','en']]:[['en-US','en'],['pl-PL','pl'],['de-DE','en']])){
   const context=await browser.newContext({locale,viewport:{width:1280,height:900}}),page=await context.newPage();page.on('pageerror',e=>errors.push(String(e)));
   await page.goto(url);await page.waitForFunction(()=>!document.querySelector('#start').disabled);
   assert.equal(await page.locator('html').getAttribute('lang'),expected,locale+' fresh boot');
   assert.equal(await page.locator('[data-language="'+expected+'"]').getAttribute('aria-pressed'),'true');
   await page.screenshot({path:path.join(out,'web-boot-'+locale+'.png')});
   await page.locator('#start').click();await page.waitForFunction(()=>document.querySelector('#start').hidden);
   await page.waitForTimeout(100);assert.equal(await page.locator('html').getAttribute('lang'),expected,locale+' fresh game');
   if(locale==='en-US'){
    await page.evaluate(()=>{const frame=window.tvFrame,ui=window.tvUI;window.tvFrame=(cells,w,h)=>{window.entryFrame=Array.from({length:h},(_,y)=>Array.from({length:w},(_,x)=>String.fromCodePoint(cells[(y*w+x)*4])).join('')).join('\n');frame(cells,w,h);};window.tvUI=u=>{window.entryFlags=u.flags;ui(u);};});
    await page.waitForFunction(()=>!(uiFlags&1)&&(uiFlags&256));await page.waitForTimeout(650);
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

    await page.keyboard.press('Shift+Tab');
    assert.equal(await page.evaluate(()=>document.activeElement.id),'fullscreen',
      'Shift+Tab moves out of the game to the preceding browser control');
    assert.equal(await page.evaluate(()=>!!(uiFlags&64)),false,'browser navigation does not toggle Quests');
    await page.locator('header a').focus();
    await page.keyboard.press('Tab');
    assert.equal(await page.evaluate(()=>document.activeElement.dataset.language),'pl',
      'Tab moves from the site link to the first language control');
    await page.locator('#game').focus();
    await page.keyboard.press('p');
    await page.waitForFunction(()=>!!(uiFlags&2));
    await page.keyboard.press('Tab');
    assert.equal(await page.evaluate(()=>document.activeElement.dataset.tap),'12','Tab reaches the toolbar while paused');
    await page.locator('#game').focus();
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
    const sample=fs.readFileSync(path.join(root,'tests/fixtures/iceberg-v4.dat'),'utf8');
    const imported=await page.evaluate(sample=>{Module._web_save();const previous=Module.FS.readFile('/persist/progress.dat.iceberg',{encoding:'utf8'});Module.FS.writeFile('/persist/import.iceberg',sample);return {ok:Module._web_import_save(),previous,backup:Module.FS.readFile('/persist/progress.dat.iceberg.bak',{encoding:'utf8'})};},sample);
    assert.equal(imported.ok,1);assert.equal(imported.backup,imported.previous,'import retains original profile as rollback');
    await page.waitForFunction(()=>document.documentElement.lang==='pl');console.log('PASS imported Polish save keeps its language and previous profile backup');
   }
   console.log('PASS',locale,'fresh boot/game language and entry control');await context.close();
  }
  const mobile=await browser.newContext({locale:'pl-PL',viewport:{width:390,height:844},deviceScaleFactor:3,isMobile:true,hasTouch:true});
  const page=await mobile.newPage();page.on('pageerror',e=>errors.push(String(e)));await page.goto(url);await page.waitForFunction(()=>!document.querySelector('#start').disabled);
  await page.locator('[data-language="en"]').click();await page.locator('#start').click();await page.waitForTimeout(150);
  await page.waitForFunction(()=>!(uiFlags&1)&&(uiFlags&256));await page.waitForTimeout(650);await page.screenshot({path:path.join(out,'web-mobile-opening.png')});
  for(const size of [{width:390,height:844},{width:844,height:390},{width:760,height:600},{width:1920,height:1080}]){
   await page.setViewportSize(size);await page.waitForTimeout(100);
   const ratio=await page.locator('#game').evaluate(c=>({bitmap:c.width/c.height,display:c.getBoundingClientRect().width/c.getBoundingClientRect().height}));
   assert.ok(Math.abs(ratio.bitmap-ratio.display)<.01,'canvas aspect preserved '+JSON.stringify(size));
   assert.equal(await page.locator('[data-language="en"]').isVisible(),true,'entry language visible '+JSON.stringify(size));
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
    await button.tap();
    await page.waitForFunction(flag=>!(uiFlags&flag),flag);
   }
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
   await page.screenshot({path:path.join(out,fallback?"web-settings-fallback.png":"web-settings-fullscreen.png")});
   await page.locator('#touch-use').tap();
   await page.waitForFunction(()=>!(uiFlags&16));
   await page.keyboard.press('Digit1');
   await page.locator('#touch-use').tap();
   await page.waitForFunction(()=>!(uiFlags&1));
   await page.locator('#leave-fullscreen').tap();
   await page.waitForFunction(()=>!expanded());
   await page.locator("footer #settings-keys").waitFor({state:"attached"});
  }
  console.log('PASS mobile native and fallback fullscreen Map, Quests and settings controls');
  console.log('PASS mobile, landscape, resized and large canvas preserve proportions; language remains reachable');await mobile.close();
  assert.deepEqual(errors,[],'browser errors');console.log(visualOnly?'PASS current Chromium/WASM visual smoke':'PASS real Chromium/WASM entry, save-language restoration and responsive rendering');
 }finally{await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(e=>{console.error(e);server.close();process.exitCode=1;});

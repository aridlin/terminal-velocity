// Drive the public WASM input API from a fresh, isolated Chromium profile.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path');
(async()=>{
 const browser=await chromium.launch({executablePath:process.env.CHROME||'/opt/google/chrome/chrome',args:['--mute-audio']});
 try {
  for(const legacy of [false,true]) {
  const page=await browser.newPage({locale:'en-US'});
  const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto(process.argv[2]);await page.waitForFunction(()=>!document.querySelector('#start').disabled);
  if(legacy){
   const fixture=fs.readFileSync(path.join(__dirname,'../tests/fixtures/iceberg-v4.dat'),'utf8');
   await page.evaluate(bytes=>Module.FS.writeFile('/persist/progress.dat.iceberg',bytes),fixture);
  }
  await page.locator('[data-language="en"]').click();
  await page.locator('#start').click();
  await page.waitForFunction(()=>started&&!syncing&&(uiFlags&256)&&!(uiFlags&1));
  const lesson=legacy?"School without a bell":"Lessons on the quay";
  await page.keyboard.down('d');
  const heldAtDoor=await page.evaluate(()=>{
   const draw=window.tvFrame;window.tvFrame=()=>{};
   try {
    for(let i=0;i<300&&!(uiFlags&1024);i++)
     Module._web_tick(.1,held|16,i%7===0?16:0,90,32,1);
    return !!(uiFlags&1024)&&!!(held&2);
   }finally{window.tvFrame=draw;}
  });
  assert.ok(heldAtDoor,'a real held key survives reaching the first WASM doorway');
  await page.waitForFunction(()=>!!(uiFlags&4)||!(uiFlags&1024));
  await page.keyboard.up('d');

  const result=await page.evaluate(lesson=>{
   const realFrame=window.tvFrame;let title='',arrived=false,briefing=false,frames=0;
   let firstLessonFrame='',lastLessonFrame='',lessonTicks=0;
   window.tvFrame=(cells,w,h)=>{
    const row=y=>Array.from({length:w},(_,x)=>String.fromCodePoint(cells[(y*w+x)*4])).join('');
    title=row(1).trim();frames++;
    if(title.includes(lesson)){
     arrived=true;
     const view=Array.from({length:h-7},(_,i)=>row(i+4)).join('\n');
     if(!firstLessonFrame)firstLessonFrame=view;
     lastLessonFrame=view;
    }
   };
   try {
    for(let i=0;i<650;i++){
     if(arrived&&++lessonTicks>35)break;
     if(uiFlags&4)briefing=true;
     // Walk and jump east, acknowledge briefing pages, never confirm a door.
     Module._web_tick(.1,2|16,(uiFlags&4)?256:(i%7===0?16:0),90,32,1);
    }
    return {title,arrived,briefing,frames,lessonTicks,moved:firstLessonFrame!==lastLessonFrame};
   }finally{window.tvFrame=realFrame;}
  },lesson);
  console.log(result);
  assert.ok(result.briefing&&result.arrived&&result.moved);
  await page.waitForFunction(()=>!syncing);
  const saves=await page.evaluate(()=>new Promise((resolve,reject)=>{
   const path='/persist/progress.dat.iceberg';
   const open=indexedDB.open('/persist');
   open.onerror=()=>reject(open.error);
   open.onsuccess=()=>{
    const db=open.result;
    const stored=db.transaction('FILE_DATA').objectStore('FILE_DATA').get(path);
    stored.onerror=()=>{db.close();reject(stored.error);};
    stored.onsuccess=()=>{
     const stage=text=>Number(text.split('\n')[1].trim().split(/\s+/)[6]);
     const memory=Module.FS.readFile(path,{encoding:'utf8'});
     const disk=new TextDecoder().decode(stored.result.contents);
     db.close();resolve({memory:stage(memory),disk:stage(disk)});
    };
   };
  }));
  assert.ok(saves.memory>0,JSON.stringify(saves));
  assert.equal(saves.disk,saves.memory,'the room transition reaches IndexedDB before a manual flush');
  await page.reload();await page.waitForFunction(()=>!document.querySelector('#start').disabled);
  await page.locator('#start').click();
  await page.waitForFunction(()=>started && (uiFlags&256) && !(uiFlags&1));
  const resumed=await page.evaluate(()=>{let title='';const draw=window.tvFrame;window.tvFrame=(cells,w)=>{title=Array.from({length:w},(_,i)=>String.fromCodePoint(cells[(w+i)*4])).join('');};try{Module._web_tick(.016,0,0,90,32,1);}finally{window.tvFrame=draw;}return title;});
  assert.ok(resumed.includes(lesson));assert.deepEqual(errors,[]);
  await page.close();
  console.log(legacy?'Legacy save':'Fresh guest','PASS one-click campaign, first threshold, briefing, playable lesson, reload and resume');
  }
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});

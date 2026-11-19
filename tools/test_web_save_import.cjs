// Test the public save controls using Chromium and real WASM save files.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const {readFile}=require('node:fs/promises');
const path=require('node:path');

(async()=>{
 const browser=await chromium.launch({
  executablePath:process.env.CHROME||'/opt/google/chrome/chrome',args:['--mute-audio']
 });
 try {
  const page=await browser.newPage({locale:'en-US',acceptDownloads:true});
  const errors=[];
  page.on('pageerror',error=>errors.push(error.message));
  await page.goto(process.argv[2]);
  assert.equal(await page.locator('#import-save').isDisabled(),true);
  await page.locator('[data-language="en"]').click();
  await page.locator('#start').click();
  await page.waitForFunction(()=>started&&!syncing);
  const download=async selector=>{
   const [file]=await Promise.all([page.waitForEvent('download'),page.locator(selector).click()]);
   return readFile(await file.path());
  };
  const current=await download('#export-save');
  assert.match(current.toString(),/^TVICE6 /);
  await page.evaluate(()=>{
   const tick=Module._web_tick;
   window.simulatedTime=0;
   Module._web_tick=(dt,...args)=>{
    window.simulatedTime+=dt;
    return tick(dt,...args);
   };
  });
  const [canceledPicker]=await Promise.all([
   page.waitForEvent('filechooser'),page.locator('#import-save').click()
  ]);
  const pickerTime=await page.evaluate(()=>window.simulatedTime);
  await page.evaluate(async()=>{
   for(let i=0;i<8;i++)await new Promise(requestAnimationFrame);
  });
  assert.ok(await page.evaluate(time=>window.simulatedTime>time,pickerTime),
   'a file chooser without a selection leaves the browser ticking');
  await canceledPicker.setFiles([]);
  await page.waitForFunction(()=>document.activeElement.id==='game');
  await page.waitForFunction(time=>window.simulatedTime>time,pickerTime);

  const legacy=await readFile(path.join(__dirname,'../tests/fixtures/iceberg-v4.dat'));
  const pick=async (buffer,accept)=>{
   const [chooser]=await Promise.all([page.waitForEvent('filechooser'),page.locator('#import-save').click()]);
   let prompts=0;
   const answer=async dialog=>{prompts++;await (accept?dialog.accept():dialog.dismiss());};
   page.on('dialog',answer);
   try {
    await chooser.setFiles({name:'journey.iceberg',mimeType:'application/octet-stream',buffer});
    await page.waitForFunction(()=>!importPending);
    assert.equal(prompts,accept===null?0:1);
    assert.equal(await page.evaluate(()=>document.activeElement.id),'game');
   }finally{page.off('dialog',answer);}
  };
  await pick(legacy,false);
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),1,'cancel keeps the English journey');
  assert.equal(await page.locator('#export-previous-save').isVisible(),false);

  await pick(Buffer.from('broken save'),null);
  assert.match(await page.locator('#save-status').innerText(),/Invalid save file/);
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),1,'invalid file leaves save intact');
  assert.equal(await page.locator('#export-previous-save').isVisible(),false);

  await page.evaluate(()=>{
   const importSave=Module._web_import_save;
   Module._web_import_save=()=>{
    window.replacedSave=Array.from(Module.FS.readFile('/persist/progress.dat.iceberg'));
    return importSave();
   };
  });
  await pick(legacy,true);
  await page.waitForFunction(()=>!syncing&&(uiFlags&256)&&!(uiFlags&1));
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),0,'legacy import resumes the Polish campaign');
  const replaced=Buffer.from(await page.evaluate(()=>window.replacedSave));
  await page.reload();
  await page.locator('#start').click();
  await page.waitForFunction(()=>started&&!syncing);
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),0);
  const backup=await download('#export-previous-save');
  assert.deepEqual(backup,replaced,'downloadable copy matches the exact save replaced by the import');

  await pick(backup,true);
  await page.waitForFunction(()=>!syncing);
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),1,'re-importing the backup restores the original campaign');
  assert.deepEqual(errors,[]);
  console.log('PASS invalid saves stop before confirmation and valid imports preserve a recoverable backup.');
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});

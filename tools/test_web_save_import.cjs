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
  const legacy=await readFile(path.join(__dirname,'../tests/fixtures/iceberg-v4.dat'));
  const pick=async (buffer,accept)=>{
   const [chooser]=await Promise.all([page.waitForEvent('filechooser'),page.locator('#import-save').click()]);
   page.once('dialog',dialog=>accept?dialog.accept():dialog.dismiss());
   await chooser.setFiles({name:'journey.iceberg',mimeType:'application/octet-stream',buffer});
   await page.waitForFunction(()=>!document.querySelector('#import-save').disabled);
  };
  await pick(legacy,false);
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),1,'cancel keeps the English journey');
  assert.equal(await page.locator('#export-previous-save').isVisible(),false);

  await pick(Buffer.from('broken save'),true);
  assert.match(await page.locator('#save-status').innerText(),/Invalid save file/);
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),1,'invalid file leaves save intact');
  assert.equal(await page.locator('#export-previous-save').isVisible(),false);

  await pick(legacy,true);
  await page.waitForFunction(()=>!syncing&&(uiFlags&256)&&!(uiFlags&1));
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),0,'legacy import resumes the Polish campaign');
  await page.reload();
  await page.locator('#start').click();
  await page.waitForFunction(()=>started&&!syncing);
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),0);
  const backup=await download('#export-previous-save');
  assert.deepEqual(backup,current,'downloadable copy matches the save replaced by the import');

  await pick(current,true);
  await page.waitForFunction(()=>!syncing);
  assert.equal(await page.evaluate(()=>Module._web_saved_language()),1,'re-importing the backup restores the original campaign');
  assert.deepEqual(errors,[]);
  console.log('PASS browser download, canceled and invalid import, legacy import, reload, and recovery of the previous save.');
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});

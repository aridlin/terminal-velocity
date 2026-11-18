// Actual account controller against the PHP CGI backend, using isolated browser
// storage/DOM adapters. Visual browser QA is separate from these sync checks.
import vm from 'node:vm';import fs from 'node:fs';import path from 'node:path';import os from 'node:os';import {spawnSync} from 'node:child_process';import assert from 'node:assert/strict';
const root=path.resolve(path.dirname(new URL(import.meta.url).pathname),'..'),[php,extensions]=process.argv.slice(2),privateDir=fs.mkdtempSync(path.join(os.tmpdir(),'tv-client-account-'));
const source=fs.readFileSync(path.join(root,'web/accounts.js'),'utf8'),sample=fs.readFileSync(path.join(root,'tests/fixtures/iceberg-v4.dat'),'utf8');
function variant(n){const payload=sample.slice(sample.indexOf('\n')+1)+' '.repeat(n);let hash=1469598103934665603n;for(const c of Buffer.from(payload))hash=BigInt.asUintN(64,(hash^BigInt(c))*1099511628211n);return 'TVICE4 '+hash+'\n'+payload;}
function browser(initial){
 const files=new Map([['/persist/progress.dat.iceberg',initial]]);let cookie='',frames=0;
 const elements=new Map();const element=()=>({value:'',style:{},hidden:false,setAttribute(){},addEventListener(){},append(){},focus(){},getBoundingClientRect(){return {left:0,top:0,width:760,height:560};}});
 const context={console,Uint32Array,URL,AbortSignal,innerWidth:900,innerHeight:800,devicePixelRatio:1,location:{href:'https://game.test/terminal-velocity/'},addEventListener(){},document:{body:{classList:{add(){},remove(){}}},activeElement:null,createElement:element,querySelector(id){if(!elements.has(id))elements.set(id,element());return elements.get(id);}},window:{}};
 context.Module={FS:{readFile(p){if(!files.has(p))throw Error('missing');return files.get(p);},writeFile(p,s){files.set(p,s);},unlink(p){files.delete(p);}},_web_save(){},_web_account_pause(){},_web_account_resume(){},_web_import_save(){const s=files.get('/persist/import.iceberg');if(!s?.startsWith('TVICE'))return 0;files.set('/persist/progress.dat.iceberg',s);return 1;},_web_guest_profile(){files.set('/persist/progress.dat.iceberg',sample);}};
 context.fetch=async (url,options)=>{
  const body=options.body||'',u=new URL(url);const env={...process.env,REDIRECT_STATUS:'200',SCRIPT_FILENAME:path.join(root,'web/server/api.php'),SCRIPT_NAME:'/terminal-velocity/api.php',REQUEST_METHOD:options.method,QUERY_STRING:u.search.slice(1),DOCUMENT_ROOT:path.join(root,'web/server'),HTTP_HOST:'game.test',SERVER_NAME:'game.test',SERVER_PORT:'443',HTTPS:'on',REMOTE_ADDR:'127.0.0.1',HTTP_COOKIE:cookie,HTTP_X_TV_CSRF:options.headers['X-TV-CSRF']||'',HTTP_ORIGIN:'https://game.test',CONTENT_TYPE:'application/json',CONTENT_LENGTH:String(Buffer.byteLength(body)),TV_ACCOUNT_DATA:privateDir};
  const result=spawnSync(php,['-n','-d','extension_dir='+extensions,'-d','extension=pdo_sqlite'],{input:body,env,encoding:'utf8'});assert.equal(result.status,0);const split=result.stdout.indexOf('\r\n\r\n'),headers=result.stdout.slice(0,split),payload=result.stdout.slice(split+4);const status=Number(headers.match(/^Status: (\d+)/m)?.[1]||200);const next=headers.match(/^Set-Cookie: ([^;\r\n]+)/m);if(next)cookie=next[1];return {ok:status<400,json:async()=>JSON.parse(payload)};
 };
 vm.createContext(context);vm.runInContext(source,context);const controller=new context.window.TVAccounts({english:()=>true,ready:()=>true,needsRedraw:()=>false,clear(){},frame(){frames++;},flush(){}});
 return {controller,files,context,frames:()=>frames};
}
function pass(label){console.log('PASS',label);}
try{
 const a=browser(sample),b=browser(variant(1));await a.controller.start();a.controller.open();a.controller.mode='register';a.controller.name.value='sync_courier';a.controller.pass.value='fixture-password-3';await a.controller.authenticate();assert.equal(a.controller.user,'sync_courier');assert.equal(a.controller.synced,sample);pass('registration keeps the guest game and saves it to the account');
 await b.controller.start();b.controller.open();b.controller.name.value='sync_courier';b.controller.pass.value='fixture-password-3';await b.controller.authenticate();assert.equal(b.controller.mode,'choose');assert.equal(b.files.get('/persist/progress.dat.iceberg'),variant(1));pass('login with different progress asks before replacing anything');
 const originalImport=b.context.Module._web_import_save;
 b.context.Module._web_import_save=()=>-1;
 assert.throws(()=>b.controller.apply(sample),{message:'save_write_failed'});
 assert.equal(b.files.has('/persist/import.iceberg'),false);
 b.context.Module._web_import_save=originalImport;
 await b.controller.choose(true);assert.equal(b.files.get('/persist/progress.dat.iceberg'),sample);assert.equal(b.files.get('/persist/guest.iceberg'),variant(1));pass('account load preserves the previous guest profile');
 a.files.set('/persist/progress.dat.iceberg',variant(2));await a.controller.sync();b.files.set('/persist/progress.dat.iceberg',variant(3));await b.controller.sync();assert.equal(b.controller.mode,'choose');assert.equal(b.files.get('/persist/progress.dat.iceberg'),variant(3));pass('a stale device keeps its local progress and opens a conflict choice');
 b.controller.close();b.controller.open();assert.equal(b.controller.mode,'choose');pass('closing the panel cannot bypass a save conflict');
 await b.controller.choose(false);assert.equal(b.controller.synced,variant(3));pass('explicitly choosing local progress updates the matching remote revision');
 await b.controller.logout();assert.equal(b.controller.user,null);assert.equal(b.files.get('/persist/progress.dat.iceberg'),variant(1));assert.equal(b.controller.entries.sync_courier.local,variant(3));pass('logout restores guest play and retains the account backup');
 b.controller.open();b.controller.name.value='sync_courier';b.controller.pass.value='fixture-password-3';await b.controller.authenticate();assert.equal(b.controller.user,'sync_courier');assert.equal(b.files.get('/persist/progress.dat.iceberg'),variant(3));pass('returning to the account resumes its profile, not the guest profile');
 b.context.fetch=async()=>{throw Error('network');};b.files.set('/persist/progress.dat.iceberg',variant(4));await b.controller.sync();assert.equal(b.controller.entries.sync_courier.local,variant(4));assert.equal(b.controller.busy,false);pass('offline sync preserves progress and releases the interface');
 assert.ok(a.frames()>0&&b.frames()>0);pass('account screens render through the shared cell-frame output');
}finally{fs.rmSync(privateDir,{recursive:true,force:true});}

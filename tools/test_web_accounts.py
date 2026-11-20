#!/usr/bin/env python3
"""Exercise real PHP CGI requests; no public server or production accounts."""
import argparse,http.cookies,json,os,pathlib,subprocess,tempfile,concurrent.futures,sqlite3
p=argparse.ArgumentParser();p.add_argument('--php-cgi',required=True);p.add_argument('--extensions',required=True);a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1];api=root/'web/server/api.php'
with tempfile.TemporaryDirectory(prefix='tv-account-test-') as private:
 def request(action,method='GET',body=None,cookie='',csrf='',origin='https://game.test',extra=None):
  data=json.dumps(body or {}).encode() if method!='GET' else b''
  env=dict(os.environ,REDIRECT_STATUS='200',SCRIPT_FILENAME=str(api),SCRIPT_NAME='/terminal-velocity/api.php',REQUEST_METHOD=method,QUERY_STRING='action='+action,DOCUMENT_ROOT=str(root/'web/server'),SERVER_NAME='game.test',SERVER_PORT='443',HTTP_HOST='game.test',HTTPS='on',REMOTE_ADDR='127.0.0.1',HTTP_COOKIE=cookie,HTTP_X_TV_CSRF=csrf,HTTP_ORIGIN=origin,CONTENT_TYPE='application/json',CONTENT_LENGTH=str(len(data)),TV_ACCOUNT_DATA=private)
  if extra:env.update(extra)
  result=subprocess.run([a.php_cgi,'-n','-d','extension_dir='+a.extensions,'-d','extension=pdo_sqlite'],input=data,capture_output=True,env=env,check=True)
  headers,payload=result.stdout.split(b'\r\n\r\n',1);headers=headers.decode();status=200;new_cookie=cookie
  for line in headers.splitlines():
   if line.startswith('Status:'):status=int(line.split()[1])
   if line.startswith('Set-Cookie:'):
    jar=http.cookies.SimpleCookie();jar.load(line.split(':',1)[1]);new_cookie='; '.join(k+'='+v.value for k,v in jar.items())
    assert 'HttpOnly' in line and 'SameSite=Strict' in line and 'secure' in line.lower()
  return status,json.loads(payload),new_cookie
 def check(value,label):
  assert value,label;print('PASS',label)
 status,s,cookie=request('session');csrf=s['csrf'];check(status==200 and s['username'] is None,'anonymous CSRF session')
 check(request('register','POST',{'username':'courier_one','password':'fixture-password-1'},cookie,'wrong')[0]==403,'registration rejects missing CSRF')
 check(request('register','POST',{'username':'courier_one','password':'fixture-password-1'},cookie,csrf,origin='https://other.test')[0]==403,'cross-origin registration denied')
 status,s,cookie=request('register','POST',{'username':'Courier_One','password':'fixture-password-1'},cookie,csrf);csrf=s.get('csrf','');check(status==200 and s['username']=='courier_one','register and normalize username')
 db=sqlite3.connect(private+'/accounts.sqlite3');hashed=db.execute('select password from users').fetchone()[0];check(hashed.startswith('$2y$') and 'fixture-password' not in hashed,'only salted password hash stored')
 sample=(root/'tests/fixtures/iceberg-v4.dat').read_text()
 check(request('save','PUT',{'revision':0,'save':sample},cookie,csrf)[0]==200,'save persisted')
 check(request('save',cookie=cookie)[1]['save']==sample,'save round-trip is byte-exact')
 check(request('save','PUT',{'revision':0,'save':sample},cookie,csrf)[0]==409,'stale device cannot overwrite newer save')
 status,s,guest=request('session');gcsrf=s['csrf'];check(request('save',cookie=guest)[0]==401,'anonymous cannot read save')
 check(request('login','POST',{'username':'courier_one','password':'wrong-password'},guest,gcsrf)[0]==401,'wrong password rejected')
 status,s,second=request('login','POST',{'username':'courier_one','password':'fixture-password-1'},guest,gcsrf);second_csrf=s.get('csrf','');check(status==200,'second device login')
 check(request('save',cookie=second)[1]['revision']==1,'second device sees account save')
 with concurrent.futures.ThreadPoolExecutor(2) as pool:
  jobs=[pool.submit(request,'save','PUT',{'revision':1,'save':sample},c,t) for c,t in [(cookie,csrf),(second,second_csrf)]]
  statuses=sorted(j.result()[0] for j in jobs)
 check(statuses==[200,409],'concurrent writes resolve atomically without lost progress')
 check(request('save','PUT',{'revision':2,'save':'not a save'},cookie,csrf)[0]==400,'malformed save rejected')
 check(request('save','PUT',{'revision':2,'save':sample+'0'},cookie,csrf)[0]==400,'checksum corruption rejected')
 status,s,third=request('session');status,s,third=request('register','POST',{'username':'courier_two','password':'fixture-password-2'},third,s['csrf']);check(status==200,'independent second account')
 check(request('save',cookie=third)[1]['revision']==0,'account saves isolated')
 check(request('logout','POST',{},cookie,csrf)[0]==200,'logout succeeds')
 check(request('save',cookie=cookie)[0]==401,'logout invalidates prior session cookie')
 check(request('save',cookie=second)[0]==200,'logout preserves other device session')
 # Current route-aware envelopes and legacy files both survive cloud storage.
 sample6=sample.replace('TVICE4 ', 'TVICE6 ', 1)
 check(request('save','PUT',{'revision':2,'save':sample6},second,second_csrf)[0]==200,'TVICE6 route-aware envelope accepted')
 check(request('save',cookie=second)[1]['save']==sample6,'TVICE6 cloud round-trip is byte-exact')
 check(request('save','PUT',{'revision':3,'save':sample6.replace('TVICE6','TVICE7',1)},second,second_csrf)[0]==400,'unknown future envelope rejected')
 for _ in range(21):last=request('login','POST',{'username':'courier_one','password':'wrong-password'},third,s['csrf'])[0]
 check(last==429,'login attempts throttled')
 check(db.execute('select count(*) from history').fetchone()[0]==2,'previous saves retained for recovery')
 def save_hash(data):
  value=1469598103934665603
  for byte in data.encode('ascii'):
   value=((value^byte)*1099511628211)&((1<<64)-1)
  return value
 capabilities={'HTTP_X_TV_SAVE_FORMAT':'portable'}
 anchor=sample6.split('\n',1)[0].split()[1]
 routes='1\n0 1 2 3 4 5 6 7 \n'
 portable=f'TVBACKUP1 {len(sample6)}\n{sample6}TVROUTE1 {anchor} {save_hash(routes)}\n{routes}'
 def put(revision,body,headers=capabilities):
  return request('save','PUT',{'revision':revision,'save':body},second,second_csrf,extra=headers)
 check(put(3,portable,None)[0]==400,'standalone clients cannot submit a portable variant accidentally')
 check(put(3,portable[:-2]+'8\n')[0]==400,'a damaged route journal is rejected with no revision change')
 check(put(3,portable.replace('TVROUTE1 '+anchor,'TVROUTE1 1',1))[0]==400,'a journal with a different primary checksum is rejected')
 check(put(3,portable+'0'*524288)[0]==413,'the transport rejects a portable backup above the account limit')
 check(put(3,portable)[0]==200,'portable archive accepted after all rejected writes')
 check(request('save',cookie=second,extra=capabilities)[1]['save']==portable,'portable sync returns the completed-route archive byte-exactly')
 check(request('save',cookie=second)[1]['save']==sample6,'older clients still receive the standalone profile')
 check(request('save',cookie=third,extra=capabilities)[1]['save'] is None,'portable accounts remain isolated')
 check(put(4,sample6,None)[0]==200 and request('save',cookie=second,extra=capabilities)[1]['save']==portable,
       'a legacy upload of the identical primary retains the portable data')
 payload=sample6.split('\n',1)[1]+' '
 changed=f'TVICE6 {save_hash(payload)}\n{payload}'
 check(put(5,changed,None)[0]==200 and request('save',cookie=second,extra=capabilities)[1]['save']==changed,
       'new legacy progress never receives an unrelated journal')
 check(db.execute('select backup from portable_saves where revision=5').fetchone()[0]==portable,
       'the earlier portable archive stays in revision history')
 for revision in range(6,12):
  assert put(revision,changed,None)[0]==200
 check(db.execute('select count(*) from portable_saves').fetchone()[0]==0,'older portable data is pruned with its corresponding revision')

 print('All account endpoint checks passed through real PHP CGI; no live deployment claimed.')

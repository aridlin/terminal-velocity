<?php
declare(strict_types=1);
// Deploy at /terminal-velocity/api.php. Data belongs outside the document root.
ini_set('display_errors', '0');
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');
function answer(array $data, int $status = 200): never {
    http_response_code($status); echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_THROW_ON_ERROR); exit;
}
function fail(string $code, int $status): never { answer(['error' => $code], $status); }
function query(PDO $db, string $sql, array $values = []): PDOStatement {
    $statement = $db->prepare($sql); $statement->execute($values); return $statement;
}
function validSave(string $body): bool {
    if (strlen($body)>65536 || !preg_match('/\ATVICE[2-5] ([0-9]{1,20})\n([\x09\x0a\x0d\x20-\x7e]+)\z/D', $body, $parts)) return false;
    // The native save's 64-bit FNV checksum, computed as two exact 32-bit limbs.
    $high=342167472; $low=1939669891;
    for($i=0,$n=strlen($parts[2]);$i<$n;$i++) {
        $low ^= ord($parts[2][$i]); $product=$low*435;
        $high=($high*435+intdiv($product,4294967296)+$low*256)&0xffffffff;
        $low=$product&0xffffffff;
    }
    $wantedHigh=0; $wantedLow=0;
    foreach(str_split($parts[1]) as $digit) {
        $product=$wantedLow*10+(int)$digit;
        $wantedHigh=$wantedHigh*10+intdiv($product,4294967296);
        if($wantedHigh>4294967295)return false;
        $wantedLow=$product&0xffffffff;
    }
    return $high===$wantedHigh && $low===$wantedLow;
}
try {
    $local = getenv('TV_ACCOUNT_TEST_HTTP') === '1' && in_array($_SERVER['REMOTE_ADDR'] ?? '', ['127.0.0.1', '::1'], true);
    $secure = ($_SERVER['HTTPS'] ?? '') === 'on';
    if (!$secure && !$local) fail('https_required', 400);
    $method = $_SERVER['REQUEST_METHOD'];
    if (!in_array($method, ['GET', 'POST', 'PUT'], true)) fail('method', 405);
    if (isset($_SERVER['HTTP_ORIGIN'])) {
        $origin = ($secure ? 'https://' : 'http://') . ($_SERVER['HTTP_HOST'] ?? '');
        if ($_SERVER['HTTP_ORIGIN'] !== $origin) fail('origin', 403);
    }
    if ((int)($_SERVER['CONTENT_LENGTH'] ?? 0) > 100000) fail('too_large', 413);
    $dataDir = getenv('TV_ACCOUNT_DATA') ?: dirname(__DIR__, 2) . '/terminal-velocity-private';
    if (!is_dir($dataDir) && !mkdir($dataDir, 0700, true)) throw new RuntimeException('storage');
    $realData = realpath($dataDir); $documentRoot = realpath($_SERVER['DOCUMENT_ROOT'] ?? '');
    if (!$realData || !$documentRoot || $realData === $documentRoot || str_starts_with($realData, $documentRoot . DIRECTORY_SEPARATOR)) throw new RuntimeException('storage location');
    umask(0077);
    $db = new PDO('sqlite:' . $realData . '/accounts.sqlite3', null, null, [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]);
    $db->exec('PRAGMA busy_timeout=5000; PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON;');
    $db->exec('CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT UNIQUE NOT NULL, password TEXT NOT NULL, created INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS sessions (token TEXT PRIMARY KEY, user_id INTEGER REFERENCES users(id), expires INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS saves (user_id INTEGER PRIMARY KEY REFERENCES users(id), revision INTEGER NOT NULL, body TEXT NOT NULL, updated INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS history (user_id INTEGER REFERENCES users(id), revision INTEGER, body TEXT NOT NULL, updated INTEGER NOT NULL, PRIMARY KEY(user_id,revision));
      CREATE TABLE IF NOT EXISTS limits (bucket TEXT PRIMARY KEY, start INTEGER NOT NULL, attempts INTEGER NOT NULL);');
    $now = time();
    query($db, 'DELETE FROM sessions WHERE expires < ?', [$now]);
    query($db, 'DELETE FROM limits WHERE start < ?', [$now - 86400]);
    $path = rtrim(str_replace('\\', '/', dirname($_SERVER['SCRIPT_NAME'])), '/') . '/';
    $cookie = 'tv_game_session';
    $token = $_COOKIE[$cookie] ?? '';
    if (!is_string($token) || !preg_match('/\A[a-f0-9]{64}\z/', $token)) $token = '';
    $session = $token ? query($db, 'SELECT user_id, expires FROM sessions WHERE token=?', [hash('sha256', $token)])->fetch(PDO::FETCH_ASSOC) : false;
    $setCookie = function(string $value, int $expires) use ($cookie, $path, $secure): void {
        setcookie($cookie, $value, ['expires'=>$expires, 'path'=>$path, 'secure'=>$secure, 'httponly'=>true, 'samesite'=>'Strict']);
    };
    $newSession = function(?int $user) use ($db, &$token, &$session, $now, $setCookie): void {
        if ($token) query($db, 'DELETE FROM sessions WHERE token=?', [hash('sha256', $token)]);
        $token = bin2hex(random_bytes(32)); $session = ['user_id'=>$user, 'expires'=>$now + 2592000];
        query($db, 'INSERT INTO sessions VALUES(?,?,?)', [hash('sha256', $token), $user, $session['expires']]);
        $setCookie($token, $session['expires']);
    };
    $action = $_GET['action'] ?? 'session';
    if ($method === 'GET' && $action === 'session') {
        if (!$session) $newSession(null);
        $name = $session['user_id'] ? query($db, 'SELECT name FROM users WHERE id=?', [$session['user_id']])->fetchColumn() : null;
        answer(['username'=>$name ?: null, 'csrf'=>hash('sha256', $token . ':csrf')]);
    }
    if (!$session) fail('session_expired', 401);
    if ($method !== 'GET') {
        if (!hash_equals(hash('sha256', $token . ':csrf'), $_SERVER['HTTP_X_TV_CSRF'] ?? '')) fail('csrf', 403);
        if (!str_starts_with(strtolower($_SERVER['CONTENT_TYPE'] ?? ''), 'application/json')) fail('content_type', 415);
        $raw = file_get_contents('php://input', false, null, 0, 100001);
        if (strlen($raw) > 100000) fail('too_large', 413);
        try { $input = json_decode($raw, true, 16, JSON_THROW_ON_ERROR); } catch (JsonException $e) { fail('json', 400); }
        if (!is_array($input)) fail('json', 400);
    }
    $rateLimit = function(string $bucket, int $maximum, int $seconds) use ($db, $now): void {
        $key = hash('sha256', $bucket);
        query($db, 'INSERT INTO limits VALUES(?,?,1) ON CONFLICT(bucket) DO UPDATE SET attempts=CASE WHEN start<? THEN 1 ELSE attempts+1 END, start=CASE WHEN start<? THEN excluded.start ELSE start END', [$key,$now,$now-$seconds,$now-$seconds]);
        if ((int)query($db, 'SELECT attempts FROM limits WHERE bucket=?', [$key])->fetchColumn() > $maximum) fail('rate_limit', 429);
    };
    if ($method === 'POST' && in_array($action, ['register','login'], true)) {
        $name = strtolower(trim(is_string($input['username'] ?? null) ? $input['username'] : ''));
        $password = is_string($input['password'] ?? null) ? $input['password'] : '';
        $rateLimit('auth-ip:' . ($_SERVER['REMOTE_ADDR'] ?? ''), 30, 900);
        $rateLimit('auth-name:' . $name, 20, 900);
        if (!preg_match('/\A[a-z0-9_-]{3,24}\z/', $name) || preg_match_all("/./us",$password)<8 || strlen($password)>72 || str_contains($password,"\0")) fail('credentials_format',400);
        if ($action === 'register') {
            $rateLimit('register:' . ($_SERVER['REMOTE_ADDR'] ?? ''), 5, 3600);
            if (query($db,'SELECT id FROM users WHERE name=?',[$name])->fetchColumn()) fail('name_unavailable',409);
            $hash = password_hash($password, PASSWORD_DEFAULT);
            try { query($db,'INSERT INTO users(name,password,created) VALUES(?,?,?)',[$name,$hash,$now]); }
            catch (PDOException $e) { if ($e->getCode()==='23000') fail('name_unavailable',409); throw $e; }
            $user=(int)$db->lastInsertId();
        } else {
            $record=query($db,'SELECT id,password FROM users WHERE name=?',[$name])->fetch(PDO::FETCH_ASSOC);
            // A fixed valid bcrypt hash keeps missing-user verification comparable.
            $valid=password_verify($password,$record['password'] ?? '$2y$12$A7mpp6RCvc4eERh8AKLXf.fXpiCvBYM3bOzw0x7rmiuEMQ59uDQsK');
            if (!$record || !$valid) fail('invalid_credentials',401);
            $user=(int)$record['id'];
            if (password_needs_rehash($record['password'],PASSWORD_DEFAULT)) query($db,'UPDATE users SET password=? WHERE id=?',[password_hash($password,PASSWORD_DEFAULT),$user]);
        }
        $newSession($user); answer(['username'=>$name,'csrf'=>hash('sha256',$token.':csrf')]);
    }
    if ($method === 'POST' && $action === 'logout') { $newSession(null); answer(['username'=>null,'csrf'=>hash('sha256',$token.':csrf')]); }
    $user=$session['user_id']; if (!$user) fail('login_required',401);
    if ($action === 'save' && $method === 'GET') {
        $save=query($db,'SELECT revision,body,updated FROM saves WHERE user_id=?',[$user])->fetch(PDO::FETCH_ASSOC);
        answer($save ? ['revision'=>(int)$save['revision'],'save'=>$save['body'],'updated'=>(int)$save['updated']] : ['revision'=>0,'save'=>null,'updated'=>null]);
    }
    if ($action === 'save' && $method === 'PUT') {
        $rateLimit('save:'.$user,120,60);
        $body=$input['save'] ?? null; $expected=$input['revision'] ?? null;
        if (!is_string($body) || !validSave($body) || !is_int($expected) || $expected<0) fail('save_format',400);
        $db->exec('BEGIN IMMEDIATE');
        $old=query($db,'SELECT revision,body,updated FROM saves WHERE user_id=?',[$user])->fetch(PDO::FETCH_ASSOC);
        $revision=$old?(int)$old['revision']:0;
        if ($revision!==$expected) { $db->exec('ROLLBACK'); fail('save_conflict',409); }
        if ($old) query($db,'INSERT OR REPLACE INTO history VALUES(?,?,?,?)',[$user,$revision,$old['body'],$old['updated']]);
        query($db,'INSERT INTO saves VALUES(?,?,?,?) ON CONFLICT(user_id) DO UPDATE SET revision=excluded.revision,body=excluded.body,updated=excluded.updated',[$user,$revision+1,$body,$now]);
        query($db,'DELETE FROM history WHERE user_id=? AND revision NOT IN (SELECT revision FROM history WHERE user_id=? ORDER BY revision DESC LIMIT 5)',[$user,$user]);
        $db->exec('COMMIT'); answer(['revision'=>$revision+1,'updated'=>$now]);
    }
    fail('not_found',404);
} catch (Throwable $e) { error_log('Terminal Velocity account API unavailable: ' . get_class($e)); fail('unavailable',503); }

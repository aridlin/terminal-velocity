# Account save service

The browser edition supports optional username/password accounts. Guest play and local saves continue when the service is unavailable. The service is deployed at `https://aridlin.pl/terminal-velocity/api.php`. Registration, login, save upload/download and logout have been exercised on the public domain with a separate QA account; local CGI and two-device conflict tests cover revision handling.

Deploy `api.php` to `/terminal-velocity/api.php`, outside the versioned static release folders. PHP 8.1+ with PDO_SQLite and HTTPS is required. Set `TV_ACCOUNT_DATA` to an application-owned directory outside the web document root; alternatively the default for that deployed location is a sibling `terminal-velocity-private` directory above `public_html`. The service refuses storage beneath the document root. Back up that directory (using SQLite's backup interface for an active database), and never include it in public build artifacts. A static-only server cannot run this endpoint.

The database stores salted password hashes, hashed random session tokens, current saves and five prior save revisions. Cookies are Secure, HttpOnly and SameSite=Strict. State-changing calls require the session's CSRF token and same-origin JSON requests. Authentication is throttled by account and source address; source addresses are hashed in the throttle table. Save uploads use optimistic revisions in SQLite transactions, validate native save checksums, and cannot silently replace a newer device's save.

Accounts need only a username and password. There is no email recovery. The interface preserves guest progress when an account is loaded and keeps an account's unsent local copy on logout. Conflicts require choosing which save to continue. The native save parser validates a downloaded save before importing it. Guests can still export a local copy.

Tests, with an available PHP CGI executable and its PDO_SQLite extension:

```
python3 tools/test_web_accounts.py --php-cgi /path/to/php-cgi --extensions /path/to/php/extensions
node tools/test_web_account_client.mjs /path/to/php-cgi /path/to/php/extensions
```

These tests create disposable fixture accounts and isolated temporary databases; they do not call the live site. `TV_ACCOUNT_TEST_HTTP=1` permits HTTP only for loopback requests in an isolated development server. Do not enable it on the public host.

References: [PHP password hashing](https://www.php.net/manual/en/function.password-hash.php) and [PDO SQLite](https://www.php.net/manual/en/ref.pdo-sqlite.php).

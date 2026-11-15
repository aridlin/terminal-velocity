# Terminal Velocity — Ghostty first

Gra platformowa C++ z fizyką 120 Hz, grana w Ghostty. SDL jest wyłącznie frontendem debugowania i używa tego samego silnika, map oraz mechanik.

## Start i wybór poziomu

Uruchom `terminal-velocity` (launcher jest w `~/.local/bin`) albo `./ghostty-play.sh`. W istniejącym Ghostty możesz użyć `./play.sh`.

- **1 — event JP2 / 11 listopada:** zachowana oryginalna Sztafeta Wolności z Wadowic do Warszawy, 15 kremówek i dziennik niepodległości.
- **2 — ekspedycja proceduralna:** osiem losowanych sekcji, opcjonalne górne trasy, kryształy, windy, sprężyny i patrole.
- **Enter:** start; **M:** powrót do menu; **N:** ponów tę samą mapę; **G:** kolejny seed proceduralny.

Każda mapa proceduralna pokazuje swój seed. Uruchom ponownie konkretną mapę: `terminal-velocity --seed 73`. Oryginalny event bez wybierania w menu: `terminal-velocity --event`.

Generator dobiera ograniczone skoki, szerokości przerw i moduły z bezpiecznymi zatokami. Trudniejsze przeszkody i nagrody są na dodatkowych trasach. Testy przechodzą 100 seedów zwykłymi wejściami bez teleportowania ani wyłączania kolizji; nie jest to dowód dla wszystkich 2³² seedów. Mapa pozostaje identyczna dla tego samego seedu w tej wersji generatora.

## Sterowanie i mechaniki

- A/D lub strzałki: ruch. SPACJA/W/góra: skok; przytrzymanie zwiększa wysokość.
- Shift/X + kierunek: zryw w ośmiu kierunkach. Lądowanie i nowa znajdźka odnawiają zryw.
- S/dół: szybkie opadanie; na ruchomej platformie — zejście przez nią.
- Przy ścianie: skok od ściany. Zachowane coyote time, bufor skoku i kontrola w powietrzu.
- `===`: ruchome platformy przenoszą postać; można przez nie skoczyć od dołu.
- `^^`: sprężyny automatycznie wybijają w górę i odnawiają zryw.
- `[!]`: patrol. Skok na głowę lub zryw pokonuje go; kontakt z boku resetuje do checkpointu.
- R: ostatni punkt zapisu. N resetuje bieg, przeciwników i znajdźki; śmierć zachowuje zebrane znajdźki i pokonane patrole w tym biegu.
- TAB: historia eventu albo pomoc mechanik. ESC/P: pauza. F3/V (V w SSH): diagnostyka. Q: koniec.

Zalecany terminal 110 × 34, minimum 76 × 24. Przełączenie fokusu pauzuje grę; Enter wznawia. Lokalne renderowanie do 60 Hz, wejście odczytywane w pętli ok. 1 ms. Zmiany kolorów i obrazu są synchronizowane w Ghostty.

## Wejście lokalne — poza terminalem

Domyślnie evdev dostarcza rzeczywiste key-down/key-up. Autorepeat terminala nie steruje postacią. Jeśli użytkownik nie ma dostępu do klawiatury, `play.sh` poprosi o hasło sudo **lokalnie w terminalu**. `keyboard_launcher` otwiera urządzenia klawiatury, trwale porzuca root i uruchamia grę jako pierwotny użytkownik. Nie zmienia ACL, grup, udev ani ustawień konta i nie jest instalowany setuid. Dostęp kończy się wraz z grą. Klawisze nie są zapisywane.

## SSH — bez sudo, bez GUI, bez evdev

Skopiuj folder źródłowy na serwer Linux i zbuduj tylko terminal:

```sh
make terminal_velocity
./play.sh --ssh --seed 73
```

Można uruchomić przez połączenie z wymuszonym PTY:

```sh
ssh -tt user@host '/sciezka/do/terminal-velocity-polska/play.sh --ssh --seed 73'
```

`play.sh` wykrywa też `SSH_CONNECTION`/`SSH_TTY` i sam wybiera SSH. `terminal-velocity --ssh` działa w bieżącym terminalu, nie otwiera okna Ghostty. Gra i kompilator muszą być na serwerze; lokalny launcher nie przesyła plików automatycznie.

**Tryb rozszerzony:** `--ssh` negocjuje protokół klawiatury kitty (np. lokalny Ghostty). Przesyła naciśnięcia i zwolnienia przez SSH, dzięki czemu działają jednoczesne klawisze. Parser zachowuje sekwencje rozdzielone pomiędzy pakietami. Po wyjściu przywraca protokół i ustawienia terminala.

**Tryb podstawowy:** gdy terminal nie obsługuje rozszerzenia lub wybierzesz `--ssh-basic`, **A/D ustawia kierunek biegu, S zatrzymuje, SPACJA skacze, X robi zryw**. To świadomie inne sterowanie, które nie wymaga key-up ani zgadywania czasu autorepeat. Menu, pauza, wybór poziomu i reset nadal działają. Używaj P do pauzy. W tym trybie zryw jest poziomy, a skok ma ustalony czas przytrzymania; zejdź z platformy jej krawędzią.

SSH odświeża obraz do 30 Hz przy fizyce 120 Hz i wysyła różnice komórek. Opóźnienie sieci nadal wpływa na reakcję. Przy problemach z przechwytywaniem sekwencji przez tmux/screen użyj połączenia bez multipleksera albo wymuś `--ssh-basic`.

## Debug GUI i testy

`./terminal_velocity_sdl --seed 73` albo `--event`. Te same wybory 1/2/G w menu lub pauzie. F3: hitbox, prędkości, FPS. R: checkpoint, Ctrl+R: reset, M: dźwięk, F11: pełny ekran. Czas „zdarzenie → tick” nie mierzy pełnej latencji sprzętu.

Budowanie wszystkiego: `make` (C++20; SDL2 i FreeType są potrzebne tylko dla GUI). Budowanie na serwerze bez bibliotek graficznych: `make terminal_velocity`.

`make test` obejmuje pierwotne testy ruchu i eventu, deterministyczność seedów, 100 przejść proceduralnych, platformy, sprężyny, starcia z patrolami i parser SSH. `./terminal_velocity --demo --seed 73` pokazuje automatyczne przejście bez klawiatury evdev; `--demo --event` odtwarza event. Demo jest testem, nie alternatywnym wejściem.

## Inspiracje

- [Celeste & Forgiveness, Maddy Makes Games](https://www.mattmakesgames.com/articles/celeste_and_forgiveness/index.html): coyote time, bufor skoku i wybaczające okna sterowania. Zachowano istniejące, dobrze odbierane parametry ruchu; platformy dodają przenoszenie postaci, nie pełną implementację „lift momentum” z Celeste.
- [Spelunky, Mossmouth](https://spelunkyworld.com/whatis.html): świeże losowane przebiegi, dodatkowe trasy, ryzyko i nagrody. Generator tutaj jest prostszy i oparty na ośmiu sekcjach.
- [Oficjalny protokół klawiatury kitty](https://sw.kovidgoyal.net/kitty/keyboard-protocol/): negocjacja flag i zdarzenia press/repeat/release przez PTY.

## Event edukacyjny: 11 listopada

JP2 jest żartobliwym przewodnikiem po kulturze i pamięci, a nie uczestnikiem wydarzeń 1918 roku. Miasta są symbolicznymi odcinkami. Event zachowuje biało-czerwone akcenty i papieskie złoto.

Źródła dziennika: [Sejm RP — Narodowe Święto Niepodległości](https://www.sejm.gov.pl/sejm10.nsf/komunikat.xsp?documentId=DE27C0547C515309C1258D3B003FDE4F) oraz [Watykan — biografia Jana Pawła II](https://www.vatican.va/content/john-paul-ii/en/biografia/documents/hf_jp-ii_spe_20190722_biografia.html).

Font GUI: `assets/FONT-LICENSE.txt`. Kod bazuje na wcześniejszym prototypie Terminal Velocity.

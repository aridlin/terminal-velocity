# Terminal Velocity — Pod powierzchnią

Platformówka o kurierze w mieście zbudowanym wewnątrz ogromnej góry lodowej. Poczta nie dochodzi, ciepło ucieka, a ktoś musi wejść tam, gdzie administracja dawno przestała zaglądać.

**Terminal jest właściwą grą.** Sceneria, portrety i animowane postacie korzystają z Unicode: bloków, Braille’a, ramek i liter. Wersja okienkowa pokazuje **ten sam obraz z komórek**, używając dołączonych fontów. Nie jest osobnym opracowaniem graficznym.

[English guide](README.md) · [Trasy klasyczne](docs/LEGACY.pl.md) · [Rozwój gry](DEVELOPMENT.md)

Rozmowy pojawiają się litera po literze, z krótkimi nagranymi głosami. **E / Enter** pokazuje od razu całą stronę; kolejne naciśnięcie przechodzi dalej. **P** wstrzymuje rozmowę.

## Co można zagrać

Podróż obejmuje **Dolną Przystań**, **Ciepłownię Łazarz**, **Osiedle Parapet**, **Archiwum Ciszy** **Ogrody Podszronu** i **Koronę**: sześć osad, 84 zaprojektowane szablony tras, naukę ruchu, maszyny, ruchome barki, ratunek dla konduktora i dostawę przesyłki, której lepiej nie przewracać. Osiem zadań mieszkańców daje dostęp do listów, skrótów serwisowych i tramwajowych, przejścia przez dach, ogrodu w przystani, strojów i prób mistrzowskich.

Historia prowadzi za oryginalnym planem sieci przez ogrody do pękniętej Korony. Nawadnianie rozwija żywe mosty; podwieszone wózki i przeciwwagi prowadzą do latarni. Po uruchomieniu obiegów wracasz na grywalny poranny obchód w przystani. Automatyczne przejście ze wszystkimi ośmioma zadaniami zajmuje około 35 minut aktywnej symulacji oraz 15 minut szacowanego czytania — **nie jest to zmierzony czas pierwszego gracza**. Zakładany czas 100–140 minut gry człowieka nie został potwierdzony.

Po zakończeniu można wrócić do mieszkańców i stałych tras, ruszyć na wyprawę bez końca albo sprawdzić się w sześciu próbach mistrzowskich. Każda składa się z siedmiu trudniejszych sal i egzaminu ruchowego. Rekord czasu, najmniejsza liczba powrotów i najlepszy zbiór zapisują się niezależnie. Własne seedy mają osobne rekordy. Ukończenie próby daje odznakę widoczną na postaci i strój.

Język polski jest domyślny. **L** w menu przełącza całą główną grę na angielski. Warto rozglądać się uważnie, ale dodatkowe odkrycia nie blokują zwykłej historii.

## Uruchamianie

Linux, w terminalu:

```sh
./play.sh
./play.sh --ssh                 # bez pomocnika evdev
./play.sh --terminal-view       # te same znaki i grafika w oknie SDL
./ghostty-play.sh               # nowe okno Ghostty
```

Minimum to **76×24 komórki**, zalecane **110×34**. Okno można skalować; **F11** przełącza pełny ekran. **Ctrl +/−** zmienia wielkość komórek, a **Ctrl 0** ją resetuje; ekrany o dużej gęstości pikseli skalują się automatycznie. Utrata fokusu pauzuje grę; **Enter/P** wznawia.

Windows 10/11 x64: rozpakuj całą paczkę, uruchom **play.cmd** w Windows Terminal albo **play-window.cmd** dla wersji okienkowej. Zachowaj katalog `assets/` obok plików wykonywalnych. Wersja okienkowa potrzebuje też dołączonego `SDL2.dll`; terminalowa nie potrzebuje SDL. Administrator nie jest wymagany.

Domyślne wejście linuksowe używa zdarzeń naciśnięcia i zwolnienia klawiszy z evdev. Jeżeli urządzenie nie jest dostępne, launcher pyta lokalnie o sudo: pomocnik otwiera klawiaturę, trwale porzuca uprawnienia i uruchamia grę jako zwykły użytkownik. Nie zmienia grup ani praw urządzeń. `--ssh` i wersja okienkowa nie używają tego pomocnika.

## Sterowanie

| Klawisz | Działanie |
| --- | --- |
| A/D lub lewo/prawo | Ruch |
| Spacja, W lub góra | Skok; przytrzymanie zwiększa wysokość |
| X lub Shift + kierunek | Zryw |
| S/dół | Szybkie opadanie; zejście przez ruchomą platformę |
| E/F | Rozmowa, obsługa maszyny, zabranie przedmiotu zadania |
| B | Oględziny z osobną, szczegółową ilustracją; B zamyka bez wykonania działania |
| E/Enter | Dalsza rozmowa/opis oględzin albo przejście po ukończeniu sali |
| R | Szybki powrót do lampy |
| Tab | Dziennik; E przy ukończonym zadaniu otwiera jego nagrodę |
| J | Mapa; W/S wybiera, Enter podróżuje, gdy przejazd jest dostępny |
| P/Escape | Pauza lub zamknięcie panelu |
| M | Menu główne |
| U | Wyciszenie |
| Q | Zapis i wyjście |

Złoty tułów oznacza gotowy zryw, niebieski — zużyty albo chwilowo ograniczony przez przesyłkę. Jasnoturkusowe **✦** odnawiają zryw; ciepłe **◆** są opcjonalnymi znajdźkami. Lampy wyznaczają punkty powrotu. Jasne krawędzie platform są fizyczne, przygaszona architektura jest tłem. Zryw nie chroni przed czerwonym zagrożeniem.

Trzy techniki: **zryw po ziemi i natychmiastowy skok** zachowuje rozpęd; **zryw w górę przy ścianie, potem skok** wybija od ściany; **skok z wznoszącej się windy** przejmuje jej prędkość. Lekcje mają bliskie punkty powrotu, a trasy mistrzowskie łączą te ruchy.

W menu wybierzesz kontynuację, nową historię, wyprawę bez końca, próbę, ustawienia lub klasyczne trasy. **J** przy wyprawie/próbie zmienia odblokowaną dzielnicę. **Enter** wznawia zapisany bieg; **R/N** zaczyna świeży. Nowa historia wymaga potwierdzenia i zastępuje postęp fabularny; zachowuje rekordy, stroje oraz osobne wyprawy.

## Seedy i zapis

```sh
./play.sh --expedition --seed 73
./play.sh --trial --seed 73 --district 2
./play.sh --campaign --language en
./play.sh --legacy --summit
```

Dzielnice odblokowuje historia. Próby odblokowuje zadanie Kuby albo koniec dostępnego otwarcia. Nowa wyprawa bez jawnego seedu dostaje nowy układ; `--seed` pozwala powtarzać dokładnie to wyzwanie. Dla zgodności **samo `--seed` nadal wybiera dawną ekspedycję proceduralną** — dodaj `--campaign`, `--expedition` lub `--trial`, aby wybrać górę lodową.

Śmierć i kontynuacja nie losują terenu ponownie. Wracasz do ostatniej lampy; postęp rozmów i zadań pozostaje. Kampania, wyprawa i próba mają osobne punkty kontynuacji. Zapis jest sprawdzany i zastępowany atomowo, z kopią odzyskiwania.

- Linux: `$XDG_STATE_HOME/terminal-velocity/progress.dat.iceberg`, zwykle `~/.local/state/terminal-velocity/progress.dat.iceberg`.
- Windows: `%LOCALAPPDATA%/TerminalVelocity/progress.dat.iceberg`.
- `TV_ICEBERG_FILE` wybiera osobny profil. `TV_PROGRESS_FILE` zmienia ścieżkę dawnych rekordów i domyślną ścieżkę towarzyszącego pliku `.iceberg`.

Stary `progress.dat` pozostaje nienaruszony. Kopiuj oba pliki przy robieniu kopii zapasowej. Gra nie nadpisuje zapisu z nowszej wersji. Zapis na końcu otwarcia będzie mógł kontynuować podróż po dodaniu dalszych dzielnic.

## Dźwięk i SSH

Nagrania muzyczne, woda przystani, maszyny ciepłowni, kroki po różnych materiałach i sygnały postaci używają wspólnego silnika audio. W ustawieniach **1/2/3** zmienia poziom muzyki/efektów/otoczenia, **4** wybiera odblokowany strój. Źródła, licencje, modyfikacje i sumy kontrolne są w [credits](assets/audio/CREDITS.txt) i [manifeście](assets/audio/manifest.json).

SSH domyślnie milczy. `--audio` odtwarza dźwięk na zdalnym komputerze z grą, nie przesyła go do klienta. `--mute` uruchamia bez dźwięku.

`--ssh` negocjuje zdarzenia klawiatury kitty, w tym zwolnienia i kombinacje. W trybie podstawowym **A/D rozpoczyna bieg, S zatrzymuje, Spacja skacze, X robi zryw**. **I/O** robi zryw w górę/dół; bieżący kierunek biegu daje skos, więc przed pionowym zrywem naciśnij S. Skok ma stały czas przytrzymania. `--ssh-basic` wymusza ten wariant.

## Duchy na żywo

```sh
./terminal_velocity --relay 27888
./play.sh --expedition --seed 73 --connect HOST:27888 --room friends
```

Używajcie tej samej wersji gry, dzielnicy, trybu i seedu. Duchy pokazują się tylko na zgodnych trasach, nie mają kolizji i znikają w menu, rozmowie oraz po utracie fokusu. Góra lodowa wymaga aktualnego relaya; klasyczne trasy pozostają osobne. Relay używa UDP. Nazwa pokoju grupuje graczy, nie jest hasłem. Nie ma wspólnego stanu fabuły ani nagranych powtórek.

## Budowanie i testy

C++20 i make wystarczą do `make terminal_velocity`. Cała gra: `make -j2 all` — okno wymaga dodatkowo SDL2 i FreeType. Pakiety Debian/Ubuntu: `g++ make pkg-config libsdl2-dev libfreetype6-dev`; Arch/CachyOS: `base-devel pkgconf sdl2-compat freetype2`.

`make test` sprawdza dawną fizykę i sieć, 10 000 seedów strukturalnie, 1000 tras zwykłymi wejściami, postęp historii i zadań, boczne drogi, próby, polskie/angielskie dialogi w 76×24, przerwane zapisy oraz oba frontendy SDL. `python tools/test_iceberg_tui.py` (pyte) sprawdza prawdziwy TUI w PTY. Dodatkowo: `./iceberg_tests 10000 1000 expert` i `./iceberg_branch_tests 2`.

Windows w MSYS2 UCRT64: `make windows windows-tests MINGW_CXX=g++ MINGW_WINDRES=windres`. Dla okna zainstaluj pakiety SDL2/FreeType UCRT64 i zbuduj `make terminal_velocity_sdl.exe MINGW_CXX=g++`. Cross-build na Linuxie z MinGW-w64: `tools/build_windows_sdl.sh`, z przypiętymi i sprawdzanymi zależnościami. [Licencje bibliotek](THIRD_PARTY.md).

CI testuje Linux i natywną logikę/terminal Windows. Jego dotychczasowy artefakt Windows zawiera terminal; osobno przygotowana paczka okienkowa zawiera też SDL EXE i DLL. Okno Windows sprawdzono przez cross-build i Wine; fizyczna klawiatura/GPU Windows wymagają osobnego sprawdzenia.

`--diagnostic` zachowuje dawny graficzny frontend. Event JP2, górskie rozdziały i Szczyt są w **Trasach klasycznych**; opisuje je [dawna instrukcja](docs/LEGACY.pl.md).

Wejście gościnne SSH: `ssh -t gra@prol.aridlin.pl`. Każda anonimowa wizyta ma osobny, tymczasowy zapis na maksymalnie dwie godziny; ponowne połączenie rozpoczyna nową wizytę.

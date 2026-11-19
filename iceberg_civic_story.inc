// The stops build the account of Stairwell Eleven from different witnesses.
inline std::vector<DialogueLine> civic_homecoming(int district,int leg) {
  switch(district) {
  case 0:
    if(leg==0)return {
      {0,{"Na pokwitowaniu jest moje pismo. Skreśliłam numer mieszkania, bo zwracane listy zajmowały całą ladę. Numer można skreślić jednym pociągnięciem. Nazwisko trudniej.", "That's my handwriting on the return slip. I crossed out the apartment number because returned letters had filled the counter. A number takes one stroke. A name is harder."}},
      {2,{"Pamiętam właścicielkę tego listu. Wracała ostatnim promem z nocnej zmiany i nosiła świeże skarpetki w kieszeni płaszcza. Nie wpisuj jej jako nieobecnej.", "I remember who that letter belonged to. She took the last ferry home from the night shift and carried clean socks in her coat pocket. Don't put her down as absent."}},
      {0,{"Dobrze. Zostawię dla niej miejsce w kolejce. Ale jeśli każde pokwitowanie nosi mój podpis, które mam ci pokazać jako pierwsze?", "All right. I'll keep her place in the queue. But if every return slip has my signature on it, which one do I show you first?"}},
    };
    return {
      {2,{"Na prom wrócili dziś ludzie z wiadrami. Ta sama woda, którą wczoraj wynosili z kuchni, niesie ich teraz do pracy. Nie umiałem im tego wpisać na bilecie.", "People came back to the ferry carrying buckets today. The same water they bailed out of their kitchens yesterday is taking them to work. I couldn't put that on a ticket."}},
      {0,{"Na liście znalazłam godzinę pierwszego zwrotu. Sześć minut i pięćdziesiąt dwie sekundy od początku próby. Witek rozpozna tę godzinę.", "I found the time of the first return. Six minutes and fifty-two seconds after the test began. Witek will recognize that time."}},
    };
  case 1:
    if(leg==0)return {
      {1,{"Zamykałem obieg zgodnie z kolejnością. Najpierw górna klatka, potem trzy dolne ulice. Tylko górna nie wróciła na manometr. Dalej robiłem to, co umiałem: pilnowałem pozostałych ulic.", "I closed the circuit in order. Upper stairwell first, then three lower streets. Only the upper one never came back on the gauge. I kept doing what I knew: keeping the other streets warm."}},
      {0,{"Myślałam, że nie oddzwoniłeś, bo było ci wszystko jedno. Teraz widzę cztery zgłoszenia pod tym samym numerem. Dlaczego żadnego nie podpiąłeś pod listę mieszkańców?", "I thought you didn't call back because you didn't care. Now I see four reports under the same number. Why didn't you attach one to the residents' list?"}},
      {1,{"Rozdzielnia pokazywała mi stopnie i bary. Nie pokazywała ludzi. Mogę ci oddać oba odczyty. Czy na tablicy ma wisieć także ten błędny?", "The switchboard showed degrees and bars. It didn't show people. I can give you both readings. Do we put the wrong one on the board as well?"}},
    };
    return {
      {1,{"Zostawiłem rysę przy kresce, do której doszło ciśnienie. Nie żebym chciał straszyć zmianę. Chcę, żeby następny mechanik wiedział, dlaczego zawór można przestawić ręcznie.", "I scratched a mark where the pressure peaked. I'm not trying to scare the next shift. I want the next mechanic to know why the valve has a manual override."}},
      {0,{"Z dołu każą podziękować za ciepło. Z góry proszą, żebyś oddzwonił osobiście. To nie jest ten sam komunikat.", "Downstairs asked me to thank you for the heat. Upstairs asked if you'd call them yourself. Those are two different messages."}},
    };
  case 2:
    if(leg==0)return {
      {3,{"Złożyłam wniosek, żeby uznano klatkę za czasowo pustą. Miało to otworzyć przejazd dla leków. Kiedy lekarstwa dotarły, przestałam pytać, czy cofnięto wniosek.", "I filed for temporary vacant status. It was supposed to open a route for medicine. Once the medicine arrived, I stopped asking whether they'd reversed it."}},
      {2,{"Ja wiozłem leki. Złamałem rozkład i byłem z siebie dumny. Na kurs powrotny przyszła dziewczyna z klatki jedenastej. Puste mieszkanie nie dostawało biletu.", "I carried the medicine. I broke the timetable and was proud of it. A girl from stairwell eleven came for a ride back. A vacant apartment didn't get a ticket."}},
      {3,{"W torbie mam niepodpisaną kopię. Jeśli jutro wsiądę do tramwaju, będą chcieli wiedzieć, co tam robiłam. Mam powiedzieć od razu czy najpierw pokazać listę?", "I have an unsigned copy in my bag. If I get on the tram tomorrow, they'll want to know what I did. Do I tell them first, or show them the list?"}},
    };
    return {
      {2,{"Wywiesiłem rozkład na klatce, a nie za szybą. Ktoś dopisał ołówkiem przystanek, do którego idzie się od środka. Zostawiłem go.", "I hung the timetable in the stairwell instead of behind glass. Somebody pencilled in a stop you reach from inside the building. I left it there."}},
      {3,{"Nie proś jej o przebaczenie w moim imieniu. Jeśli spotkasz tę dziewczynę w archiwum, zapytaj tylko, gdzie chce wysiąść.", "Don't ask her to forgive me on my behalf. If you meet that girl in the archive, just ask where she wants to get off."}},
    };
  case 3:
    if(leg==0)return {
      {4,{"Przepisałam wykaz na maszynie. Mój kierownik zostawił mnie wtedy na dodatkowej zmianie, bo wiedział, że nie zepsuję pięciu kopii. To była dla mnie pochwała.", "I typed up the register. My supervisor kept me on the extra shift because he knew I wouldn't spoil five copies. I took that as praise."}},
      {3,{"Ja zostawiłam pieczęć na biurku, kiedy wyszłam na tramwaj. Ty wydałaś kopię. Teraz obie możemy opowiedzieć, że tylko dopełniałyśmy cudzych decyzji.", "I left the stamp on my desk when I went to catch the tram. You released the copy. We can both tell the story as if we were just finishing somebody else's decisions."}},
      {4,{"Plan schowałam do osobnej szuflady. Przez lata uznawałam, że to wystarczy. Teraz możemy zabrać oryginał albo zrobić kopie, zanim otworzymy gablotę. Co najpierw?", "I put the plan in a separate drawer. For years I told myself that was enough. Now we can take the original, or make copies before we open the case. Which comes first?"}},
    };
    return {
      {4,{"W teczce został odcisk dłoni na kalce. To drobiazg, ale tylko na tym arkuszu ktoś docisnął papier za mocno. Chcę, żeby kopia w czytelni miała i ten ślad.", "There's a handprint in the carbon paper. It's a small thing, but on that one sheet somebody pressed too hard. I want the reading-room copy to show that mark too."}},
      {3,{"Gdy ktoś zapyta o mój podpis, przyślij go do mnie. Nie do spółdzielni. Do mnie.", "When somebody asks about my signature, send them to me. Not to the housing board. To me."}},
    };
  case 4:
    if(leg==0)return {
      {5,{"Moja matka wystawiała cebulę na parapet w klatce jedenastej. Kiedy zabrano ciepło z mieszkania, przeniosła ją do szklarni. Zostało mi pudełko nasion i rachunek za przeniesienie donic.", "My mother grew onions on a windowsill in stairwell eleven. When they cut the heat to her apartment, she moved them to the greenhouse. I'm left with a seed box and an invoice for moving the pots."}},
      {1,{"Umiem odkręcić obejście. Jeśli odkręcę za mocno, ciśnienie spadnie w jej starej ulicy. Dziś też ktoś tam mieszka.", "I know how to open the bypass. If I open it too far, pressure drops in her old street. Somebody lives there now, too."}},
      {5,{"To najpierw zobacz, co tu rośnie. Dopiero potem zapytaj, ile ciepła mogę oddać. Chcesz liczyć od szklarni czy od domów?", "Then look at what's growing here first. Ask what heat I can spare after that. Do you want to count from the greenhouse or from the homes?"}},
    };
    return {
      {5,{"Na noc podzielimy obieg. Dwie godziny dla sadzonek, cztery dla okien. Rano niech przyjdą ci z klatki i przestawią tablicę razem ze mną.", "We'll share the line tonight. Two hours for seedlings and four for windows. In the morning let the people from the stairwell come change the board with me."}},
      {1,{"Już zapisałem obie wartości. Tym razem nie ma pustego pola na adres.", "I've written down both readings. This time there isn't a blank field for the address."}},
    };
  default:
    if(leg==0)return {
      {6,{"Pierwsze zlecenie do Korony dostałem od Heleny. Odmówiła podpisania odbioru, bo miałem za małe buty. Pomyślałem, że znowu chodzi o regulamin. Następnego dnia przyniosła mi własne.", "My first delivery to the Crown came from Helena. She wouldn't sign my clearance because my boots were too small. I thought it was another rule. The next day she brought me her own."}},
      {6,{"Dlatego tak się śpieszę. Czekanie na pomoc przypomina mi tamte schody. Powiedz mi tylko, czy mam zostać przy tobie, czy wolisz, żebym prowadził asekurację z góry.", "That's why I hurry. Waiting for help reminds me of those stairs. Just tell me if you want me beside you, or up ahead keeping the line secure."}},
    };
    return {
      {6,{"Wysłałem kopie najwolniejszym tramwajem. Zatrzyma się na wszystkich przystankach. Pierwszy raz od dawna wolę kurs, który nie jest ekspresowy.", "I sent the copies on the slowest tram. It stops everywhere. For the first time in a while, I'd rather it didn't run express."}},
      {4,{"Zanim otworzysz śluzę, usłyszysz głos z protokołu. Słuchaj tego, co próbuje policzyć. To nam powie, czego do tej pory nie chciano widzieć.", "Before you open the airlock you'll hear a voice reading the report. Listen for what it's trying to count. That will tell us what people chose not to see."}},
    };
  }
}

struct CivicReply { Text choice, said; DialogueLine answer; };
inline const CivicReply &briefing_reply(int district,int option) {
  static const std::array<std::array<CivicReply,2>,DistrictCount> replies{{
    {{
      {{"Kto tam czeka?","Who's waiting up there?"},
       {"Kto zgłosił, że na górze nie ma ciepła?","Who called about the heat upstairs?"},
       {0,{"Kobieta z nocnej zmiany. Zamiast nazwiska podała godzinę ostatniego promu. Bogdan będzie wiedział.","A woman on the night shift. She gave me the time of the last ferry instead of her name. Bogdan will know."}}},
      {{"A jeśli nie wrócę na czas?","What if I'm late?"},
       {"A jeśli nie wrócę przed zamknięciem okienka?","What if I'm not back before you close the counter?"},
       {0,{"Zostawię lampę w oknie. Poczta może poczekać do rana.","I'll leave a lamp in the window. The post can wait till morning."}}},
    }},
    {{
      {{"Skąd ta rysa?","What's that scratch?"},
       {"Kto zaznaczył tę kreskę na manometrze?","Who scratched that line into the gauge?"},
       {1,{"Ja. Wtedy jeszcze wierzyłem, że wyżej wskazówka nie dojdzie.","I did. Back when I still thought the needle couldn't go any higher."}}},
      {{"Kto mieszka nad kotłem?","Who lives over the boiler?"},
       {"Mówiłeś o ścianie. Kto mieszka nad kotłem?","You mentioned the wall. Who lives over the boiler?"},
       {1,{"Moja siostra mieszkała tam na trzecim. Teraz na tym piętrze są trzy rodziny. Znam ich po krokach.","My sister used to live on the third. Three families live there now. I know them by their footsteps."}}},
    }},
    {{
      {{"Masz coś dla pasażerów?","What do I tell passengers?"},
       {"Jeśli ktoś będzie czekał przy rozjeździe, co mam mu powiedzieć?","If someone's waiting by the junction, what should I tell them?"},
       {2,{"Że po nich wrócę. Kiedyś kazałem wszystkim czekać przy słupku, nawet kiedy słupek już leżał w wodzie.","Tell them I'll come back for them. I used to tell people to wait by the stop sign even after it fell in the water."}}},
      {{"Kto odbierze leki?","Who'll take the medicine?"},
       {"A pani z czwartego? Czeka na dole czy w mieszkaniu?","What about the woman on the fourth floor? Is she waiting downstairs or at home?"},
       {2,{"W mieszkaniu. Grażyna rozpisała dawki na odwrocie rozkładu. Sam zaniosę jej leki.","At home. Grażyna wrote the doses on the back of the timetable. I'll take the medicine upstairs myself."}}},
    }},
    {{
      {{"Kto wydał kopie?","Who issued the copies?"},
       {"Pamiętasz, kto wydał pierwszy plan bez tej ulicy?","Do you remember who issued the first plan without that street?"},
       {4,{"Pamiętam własne inicjały pod rubryką «sprawdzono». Wtedy sprawdzałam, czy zgadza się liczba stron.","I remember my initials in the 'checked' box. Back then I checked the page count."}}},
      {{"Czemu został oryginał?","Why keep the original?"},
       {"Skoro nowe wydanie miało zastąpić stare, czemu masz oryginał?","If the new edition replaced the old one, why do you still have the original?"},
       {4,{"Potrzebowałam adresu do kartki urodzinowej. Gdybym go znała na pamięć, może wyrzuciłabym plan razem z resztą.","I needed an address for a birthday card. If I'd known it by heart, I might have thrown the plan out with the rest."}}},
    }},
    {{
      {{"Zaczekaj na mój sygnał","Wait for my signal"},
       {"Zaczekaj przy śluzie, aż dam znać, że woda wróciła.","Wait by the airlock till I tell you the water's back."},
       {6,{"Dobrze. Raz już ruszyłem, bo w rurze ucichło. Potem Witek mi powiedział, ile rzeczy może znaczyć cisza.","All right. I set off once because the pipe went quiet. Witek told me afterward how many things quiet can mean."}}},
      {{"Słyszysz tam ludzi?","Can you hear anyone up there?"},
       {"Po drugiej stronie śluzy słychać coś poza wiatrem?","Can you hear anything besides the wind on the other side?"},
       {6,{"Stuka blacha. Przez chwilę myślałem, że ktoś odstukuje na mój sygnał. Dlatego chcę tam wrócić.","There's a tapping on the sheet metal. For a second I thought somebody was tapping back. That's why I want to go back."}}},
    }},
    {{
      {{"Poproś ich, żeby słuchali","Ask them to listen"},
       {"Poproś ich, żeby zostali przy odbiornikach. Jeśli ktoś odpowie z góry, chcę, żeby usłyszało go więcej niż jedno biuro.","Ask them to stay by their radios. If somebody answers from up here, I want more than one office to hear them."},
       {0,{"Powiem im. Pierwszy raz puszczę sygnał na wszystkie okienka, także te zamknięte.","I'll tell them. For once I'll put the call through to every counter, even the closed ones."}}},
      {{"Zapytaj o adresy","Ask them for addresses"},
       {"Zapytaj, jak nazywali tę ulicę, zanim zniknęła z planu. Może każdy pamięta inny numer.","Ask what they called the street before it went off the map. Maybe everyone remembers a different number."},
       {0,{"Wezmę pustą księgę. Tym razem nie zacznę od wykazu.","I'll get a blank ledger. This time I won't start from the register."}}},
    }},
  }};
  return replies[size_t(district)][size_t(option)];
}

inline const CivicReply &harbor_mission_reply(int mission,int option) {
  static const std::array<std::array<CivicReply,2>,3> replies{{
    {{
      {{"Dlaczego nie wysiadłeś?","Why stay in the tram?"},
       {"Bogdan, możesz dojść do nabrzeża pieszo. Dlaczego jeszcze tam siedzisz?","Bogdan, you can walk back to the quay. Why are you still in the cab?"},
       {2,{"Bo kiedy woda zeszła, obiecałem ludziom, że pierwszy kurs ruszy o świcie. Gdy wrócę bez tramwaju, znowu będą szli do pracy przez zalane piwnice.","When the water went down, I promised the first tram would leave at dawn. If I come back without it, people will have to walk to work through flooded cellars again."}}},
      {{"Ilu ludzi czeka?","How many people are waiting?"},
       {"Ile osób zostało na przystanku?","How many people are still at the stop?"},
       {2,{"O pięć po czwartej było siedem. Potem oddałem płaszcz chłopakowi, który przyszedł po matkę. W rozkładzie mam kreskę, gdzie zwykle zapisuję liczbę.","At five past four there were seven. Then I gave my coat to a boy who came looking for his mum. There's a dash where I usually write the headcount."}}},
    }},
    {{
      {{"Czyj jest termometr?","Whose thermometer is it?"},
       {"Kto przyniósł ten termometr? Na rączce jest czyjeś nazwisko.","Who brought this thermometer in? There's a name on the handle."},
       {0,{"Witek. To nazwisko jego siostry. Po nocnej zmianie porównywali odczyt w kotłowni z tym w jej kuchni. Któregoś dnia przestały się zgadzać.","Witek. That's his sister's name. After the night shift they'd compare the boiler room reading against the one in her kitchen. One day the two stopped matching."}}},
      {{"Kto zobaczy odczyt?","Who sees the reading?"},
       {"Kiedy zaniosę wynik Witkowi, kto poza nim się o nim dowie?","Once I bring Witek the reading, who else will hear it?"},
       {0,{"Możesz zostawić mi kopię. Rano przychodzą tu ludzie po listy. Od tygodnia pytają, czemu ich liczniki chodzą, kiedy grzejniki są zimne.","Leave me a copy if you can. People come here for their post in the morning. All week they've been asking why their meters keep running with the radiators cold."}}},
    }},
    {{
      {{"Dlaczego ciągle płynie?","Why is it still flowing?"},
       {"Skoro śluza miała być nieczynna, kto zostawił ciepły obieg?","If the sluice was meant to be out of service, who left the hot circuit running?"},
       {1,{"Ja. Przy takim mrozie woda musi płynąć, inaczej rura zamarza i pęka. Miałem wrócić po godzinie. Potem zobaczyłem, że na wykazie nikt już tu nie mieszka.","I did. In that kind of cold the water has to keep moving, or the pipe freezes and bursts. I was meant to come back in an hour. Then I saw the register said nobody lived here anymore."}}},
      {{"Co z drugim brzegiem?","What about the far bank?"},
       {"Kiedy ruszy obieg, kto sprawdzi grzejniki na drugim brzegu?","When the line comes back, who'll check the radiators on the far bank?"},
       {1,{"Irena. Wie, który grzejnik w szkole odzywa się pierwszy. Zostawiła mi klucz. Głupio mi było przyznać, że od tygodnia noszę go w kieszeni.","Irena. She knows which radiator in the school makes a noise first. She left me her key. I've been too embarrassed to tell her it's been in my pocket all week."}}},
    }},
  }};
  return replies[size_t(mission)][size_t(option)];
}

inline const CivicReply &epilogue_reply(int option) {
  static const std::array<CivicReply,2> replies{{
    {{"Kto powie Mariannie?","Who’ll tell Marianna?"},
     {"Czy ktoś powie Mariannie, że może sprawdzić swój adres w nowym odpisie?","Will someone tell Marianna she can look up her address in the new copy?"},
     {0,{"Ja jej powiem. Pójdę po ostatnim promie do piekarni. Jeśli będzie chciała przeczytać, co wtedy podpisałam, usiądę z nią i przejdziemy przez to razem.","I will. I’ll go to the bakery after the last ferry. If she wants to read what I signed that day, I’ll sit with her and go through it together."}}},
    {{"Otworzysz nowe okienko?","Will you open the new counter?"},
     {"Heleno, ty otworzysz dziś nowe okienko?","Helena, will you open the new counter today?"},
     {0,{"Tak. Kiedy ostatnio przyszła tu kobieta z jedenastej, podałam jej formularz i zamknęłam szybę. Dziś wyniosę krzesło przed ladę. Jak ktoś przyniesie mi ten stary formularz, usiądę i go przeczytam.","Yes. The last time a woman from stairwell eleven came in, I gave her a form and closed the window. Today I’ll put a chair out in front. If anybody brings back that old form, I’ll sit down and read it."}}},
  }};
  return replies[size_t(option)];
}

inline const CivicReply &civic_reply(int scene,int option) {
  if(scene==IrrigationReplyScene) {
    static const std::array<CivicReply,2> replies{{
      {{"Jak wjechać na górę?","How do I get up there?"},
       {"Stoję przy kole. Jak dostać się na półkę z nasionami?","I'm at the wheel. How do I reach the seed shelf?"},
       {5,{"Poczekaj, aż czerpak zjedzie obok tarasu. Wskocz na niego w lewo. Kiedy cię podniesie, skocz w prawo na półkę.","Wait for a scoop to come down beside the terrace. Jump left onto it. Once it carries you up, jump right to the shelf."}}},
      {{"Gdzie znajdę Basię?","Where can I find Basia?"},
       {"A gdzie znajdę Basię, jeśli uda mi się zdjąć skrzynkę?","Where can I find Basia if I get the box down?"},
       {5,{"W szklarni. To moja siostra. Ostatniej wiosny rozdałam sąsiadom sadzonki, które miała zabrać. Sama jej zaniosę skrzynkę. Tym razem wybierze pierwsza.","In the greenhouse. She's my sister. Last spring I gave away the seedlings she'd set aside. I'll take the box to her myself. This time she gets first pick."}}},
    }};
    return replies[size_t(option)];
  }
  if(scene==HoistCrewReplyScene) {
    static const std::array<CivicReply,2> replies{{
      {{"Kiedy wskoczyć?","When should I board?"},
       {"Winda przejeżdża tuż obok. Wskoczyć, kiedy zapali się zielone?","The hoist passes close to this ledge. Do I jump when the light goes green?"},
       {1,{"Na zielonym wskocz na pasy. Patrz na mój panel, póki czekasz na dole. Gdy winda stanie u góry, skocz w prawo. Dalej patrz na pasy. Każda winda ma swój rytm.",
           "Jump onto the striped deck when my panel goes green. Once the lift stops at the top, jump right. Watch the striped decks ahead. Each lift runs on its own cycle."}}},
      {{"Dla kogo wolna winda?","Who needs the empty lift?"},
       {"Mówisz, że jedna winda ma wrócić pusta. Dla kogo ją zostawiasz?","You said one lift goes back empty. Who are you keeping it for?"},
       {1,{"Dla Ani. Po podwójnej zmianie zejście po schodach zajmuje jej pół godziny. Kiedyś wpisywałem ten pusty kurs jako stratę czasu. Teraz wpisuję go jako kurs dla załogi.",
           "For Ania. After a double shift it takes her half an hour to get down the stairs. I used to log the empty run as wasted time. Now I put it on the schedule as a crew run."}}},
    }};
    return replies[size_t(option)];
  }
  if(scene==EpilogueReplyScene)return epilogue_reply(option);
  if(scene>=HarborMissionReplyBase)return harbor_mission_reply(scene-HarborMissionReplyBase,option);
  if(scene>=BriefingReplyBase)return briefing_reply(scene-BriefingReplyBase,option);
  static const std::array<std::array<CivicReply,2>,7> replies{{
    {{
      {{"Pokaż pierwszy zwrot","Show me the first return"},
       {"Zacznijmy od najstarszego. Nie musisz za siebie odpowiadać przy całej kolejce.","Start with the oldest. You don't have to answer for yourself in front of the whole queue."},
       {0,{"Przyniosę go po zamknięciu okienka. Zostawiam swój podpis na kartce.","I'll bring it after I close the counter. I'm leaving my signature on it."}}},
      {{"Zacznij od nazwiska","Start with the person's name"},
       {"Zacznij od jej nazwiska. Numer nie wracał promem po zmianie.","Start with her name. A number never took the ferry home from the night shift."},
       {0,{"Marianna Kłos. Przepisałam jej adres już trzy razy. Dzisiaj pierwszy raz powiedziałam go na głos.","Marianna Kłos. I've copied her address three times. Today was the first time I said it aloud."}}},
    }},
    {{
      {{"Powieś oba odczyty","Post both readings"},
       {"Powieś oba. Chcę, żeby twoja zmiana widziała, dlaczego musiała pracować na złym odczycie.","Post both. The next shift should know why they were working from the wrong reading."},
       {1,{"Dobrze. Zostawię też pod nim swój numer zmiany. Nie chcę, żeby szukali winnego w następnej brygadzie.","All right. I'll leave my shift number under it. I don't want them looking for someone to blame on the next crew."}}},
      {{"Najpierw pokaż korektę brygadzie","Show your crew the correction"},
       {"Najpierw pokaż korektę swojej zmianie. Niech ci powiedzą, co w ich notatkach się z nią nie zgadza.","Show your crew the correction first. Ask them where their own notes disagree."},
       {1,{"Jasne. I wrócę z odpowiedzią, nawet jeżeli będzie brzmiała: pomyliłeś się, Witek.","I will. And I'll come back with the answer even if it's: you got it wrong, Witek."}}},
    }},
    {{
      {{"Powiedz im sama","Tell them yourself"},
       {"Powiedz im sama. Listę zostaw na stole, żeby mogli zadać pytania, kiedy skończysz.","Tell them yourself. Leave the list on the table so they can ask questions when you're done."},
       {3,{"Wsiądę przy pierwszym przystanku. Nie będę robić z Bogdana rzecznika swojej decyzji.","I'll get on at the first stop. I won't make Bogdan answer for my decision."}}},
      {{"Niech najpierw zobaczą wykaz","Let them read the list first"},
       {"Daj im chwilę z wykazem. Inaczej cały wagon będzie patrzył na ciebie zamiast szukać ludzi na kartce.","Give them a minute with the list. Otherwise the whole car will be watching you instead of looking for people on the page."},
       {3,{"Zrobię tak. Ale jak podniosą głowy, będę tam siedziała.","I'll do that. But when they look up, I'll still be there."}}},
    }},
    {{
      {{"Zabezpiecz oryginał","Secure the original"},
       {"Najpierw zabezpiecz oryginał. Na kopii i tak trzeba wskazać, co się z nim różni.","Secure the original first. The copies still need to show exactly what changed."},
       {4,{"Przeniosę go do czytelni, nie z powrotem do szuflady. We dwie go tam zaniesiemy.","I'll take it to the reading room, not back to the drawer. We'll carry it there together."}}},
      {{"Roześlij kopie od razu","Send copies now"},
       {"Zrób kopie teraz. Jeśli znowu zgasną światła, przynajmniej ktoś będzie miał skąd zacząć.","Make copies now. If the lights go out again, at least somebody will know where to start."},
       {4,{"Trzy arkusze już czekają w podajniku. Tym razem nie poproszę nikogo, żeby mi pokwitował odbiór, zanim je dostanie.","Three sheets are already in the tray. This time I won't ask anyone to acknowledge receipt before they get one."}}},
    }},
    {{
      {{"Policzmy od domów","Start with the homes"},
       {"Policzmy od domów. Potem poprośmy ludzi, żeby przyszli tu zobaczyć, co zostało w szklarni.","Start with the homes. Then ask people to come and see what's left in the greenhouse."},
       {5,{"Dobrze. Żadna sadzonka nie potrzebuje podpisu, ale potrzebuje kogoś, kto tu zajrzy rano.","All right. A seedling doesn't need a signature, but it does need somebody to check on it in the morning."}}},
      {{"Pokaż mi najpierw sadzonki","Show me the seedlings first"},
       {"Pokaż mi sadzonki. Witek może w tym czasie zapytać mieszkańców, które pokoje już są zimne.","Show me the seedlings. Witek can ask residents which rooms are already cold while we look."},
       {5,{"Dobrze. Nie chcę wybierać między matką a lokatorami. Chcę, żeby obie liczby były prawdziwe.","All right. I don't want to choose between my mother and the tenants. I want both numbers to be real."}}},
    }},
    {{
      {{"Zostań obok","Stay beside me"},
       {"Zostań obok. Jeśli zrobi się zbyt ślisko, wrócimy razem po lepszą linę.","Stay beside me. If it gets too slippery, we'll go back together for a better rope."},
       {6,{"Zostaję. I pierwszy raz od dawna nie zacznę od odliczania.","I'll stay. And for once I won't start counting down."}}},
      {{"Asekuruj z góry","Keep the line from above"},
       {"Prowadź linę z góry. Nie śpiesz się tylko dlatego, że nie widzisz, jak idę.","Keep the line from above. Don't hurry just because you can't see how fast I'm going."},
       {6,{"Dwa szarpnięcia, jeśli mam stanąć. Będę czekał, aż odpowiesz.","Two tugs if you want me to stop. I'll wait for your answer."}}},
    }},
    {{
      {{"Dołącz nazwiska świadków","Include the signed testimony"},
       {"Wywieśmy nazwiska tych, którzy są gotowi podpisać się pod zeznaniem. Pozostałych zapytajmy, zanim użyjemy ich głosów.","Post the names of people ready to sign their testimony. Ask the others before we use their voices."},
       {4,{"Zrobię miejsce na dopiski. Jeśli ktoś zmieni zdanie, przyjdzie do człowieka, nie do okienka na formularze.","I'll leave space for corrections. If somebody changes their mind, they can come to a person, not a forms counter."}}},
      {{"Opublikuj decyzje i zapytaj ludzi","Publish the orders and ask people"},
       {"Opublikujmy decyzje z podpisami. Głosy mieszkańców zostawmy przy oryginale, dopóki sami nie zdecydują.","Publish the signed orders. Keep the residents' voices with the original until they decide for themselves."},
       {4,{"Tak. Odpowiedzialność za decyzje nie może zależeć od tego, kto zechce po raz drugi opowiedzieć o swojej krzywdzie.","Yes. Accountability for the orders can't depend on who is willing to tell their worst story a second time."}}},
    }},
  }};
  return replies[size_t(scene)][size_t(option)];
}

inline DialogueLine civic_followup(int district,bool option) {
  switch(district) {
  case 0: return option ? DialogueLine{0,{"Na kopii nie zostawiłam pustej rubryki. Marianna Kłos. Bogdan będzie wiedział, do kogo zawieźć kartkę.","I left no empty box on the copy. Marianna Kłos. Bogdan will know who should get the slip."}} :
    DialogueLine{0,{"Po zamknięciu znalazłam najstarsze pokwitowanie. Jest pod nim znak drugiej zmiany. Zaniosę im kopię osobiście.","After closing I found the oldest receipt. It has the second shift's mark at the bottom. I'll take them a copy myself."}};
  case 1: return option ? DialogueLine{1,{"Brygada znalazła trzeci odczyt. Pod koniec próby ktoś otworzył zawór od środka. Bez ich notatek bym o tym nie wiedział.","The crew found a third reading. Somebody opened a valve from inside near the end of the test. Without their notes I wouldn't have known."}} :
    DialogueLine{1,{"Oba odczyty wiszą obok siebie. Pierwsza zmiana dopisała godzinę własnego wezwania, tę samą, którą Helena znalazła na pokwitowaniu.","Both readings are up side by side. The first shift added the time of their call, the same one Helena found on her receipt."}};
  case 2: return option ? DialogueLine{3,{"Na wykazie ktoś zakreślił nazwisko, o które nie zapytałam. Zaczekam na następnym kursie, żeby dowiedzieć się, kto to zrobił.","Someone circled a name I hadn't asked about. I'll wait on the next service to find out who did it."}} :
    DialogueLine{3,{"Powiedziałam im sama. Jedna osoba wysiadła wcześniej. Inna została i opowiedziała mi, kto brał leki. Nadal nie wiem, co mam zrobić z oboma tymi gestami.","I told them myself. One person got off early. Another stayed and told me who needed the medicine. I still don't know what to do with either gesture."}};
  case 3: return option ? DialogueLine{4,{"Pierwszą kopię odebrała dziewczyna z tramwaju. Narysowała kredką, którędy szła po leki. Wydrukowałam jej trasę na następnej.","The girl from the tram took the first copy. She drew the route she took for the medicine in pencil. I printed her route on the next one."}} :
    DialogueLine{4,{"Przeniosłyśmy oryginał razem. Kiedy Grażyna podtrzymywała drugą stronę, po raz pierwszy zobaczyłam, ile waży coś, co przez lata ukrywałam sama.","We carried the original together. With Grażyna holding the other end, I finally felt how heavy the thing I'd hidden alone for years was."}};
  case 4: return option ? DialogueLine{5,{"Witek przyszedł z temperaturami z domów. Ja dałam mu termometr ze szklarni. Wreszcie mierzymy w tym samym pokoju.","Witek brought the temperatures from the homes. I gave him the greenhouse thermometer. We're finally taking readings in the same room."}} :
    DialogueLine{5,{"Pierwszy przyszedł chłopiec z zimnego mieszkania i nie chciał herbaty. Chciał zobaczyć donice. Dałam mu kluczyk do bocznych drzwi.","The first to come was a boy from a cold apartment. He didn't want tea. He wanted to see the pots. I gave him the key to the side door."}};
  default: return option ? DialogueLine{6,{"Z góry widać, gdzie lina szoruje o blachę. Zaznaczyłem miejsce na ścianie. Jeśli wrócę z innym kurierem, już będzie wiedział.","From above I could see where the line rubs on the metal. I marked the wall. If I come back with another courier, they'll already know."}} :
    DialogueLine{6,{"Przy ostatniej poręczy oba kroki było słychać tak samo wyraźnie. Wtedy przestałem się zastanawiać, kto idzie pierwszy.","At the last rail I could hear both sets of steps just as clearly. That's when I stopped worrying about who was going first."}};
  }
}

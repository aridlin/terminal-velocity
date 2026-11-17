#pragma once
#include "iceberg_types.hpp"
namespace iceberg {
struct Npc {
  const char *name;
  Text title;
  uint32_t color;
  std::array<const char *, 8> portrait;
};
inline const std::array<Npc,8> Npcs{{
 {"HELENA",{"Dyspozytorka","Dispatcher"},0xe4ad68,
  {"     ▄████▄     ","   ▄█▀▀▀▀▀██▄   ","   ██ ▄  ▄ ██   ","  ▐██   ▵  ██▌  ","   ▀█▄ ── ▄█▀   ","    ▄██▄▄██▄    ","  ▄██╱ ▣ ╲██▄   ","  ███  ║  ███   "}},
 {"WITEK",{"Mistrz obiegu","Boiler mechanic"},0xe99462,
  {"    ▄██████▄    ","   ███▀▄▄▀███   ","   ▀█ ━  ━ █▀   ","   ▐█  ▰▰  █▌   ","    █▄████▄█    ","   ▄█▀████▀█▄   ","  ██╱ ╳  ╲███   ","  ██▣████▣███   "}},
 {"BOGDAN",{"Konduktor bez toru","Conductor without a track"},0xc9bb85,
  {"    ▄▄▄▄▄▄▄     ","   ▐██ ▣ ███▌   ","   ▀▀█████▀▀▀   ","    █ ◉  ◉ █    ","    █  ━━  █    ","    ▀█▄▄▄▄█▀    ","   ▄█╲ ▽ ╱█▄    ","  ██▣█ │ ████   "}},
 {"GRAŻYNA",{"Spółdzielnia Parapet","Windowsill Housing Board"},0xd7a8c8,
  {"    ▄██████▄    ","   ███▀▀▀▀███   ","   ██▣━━━━▣██   ","   ██  ▵  ███   ","   ▀█▄ ━ ▄██▀   ","    ▀██████▀    ","  ▄██╲ ◇ ╱██▄   ","  ███ ╲╱ ████   "}},
 {"IRENA",{"Archiwistka","Archivist"},0xabbcf4,
  {"      ▄██▄      ","   ▄███████▄    ","   ██▀▀▀▀▀██    ","   █ ▣━━▣ ▐█    ","   █   ▵  ▐█    ","   ▀█▄ ▬ ▄█▀    ","   ▄██╲╱███▄    ","  ███ ▤▤ ████   "}},
 {"CELINA",{"Ogrodniczka","Gardener"},0xb8dfb7,
  {"    ▄▄♣▄▄▄      ","   ▄██████▄     ","  ▐██▀▀▀▀██▌    ","   █ ◡  ◡ █     ","   █  ▵   █     ","   ▀█▄ ◡ ▄█▀    ","  ▄██╲╱████▄    ","  ███ ░░ ▣███   "}},
 {"KUBA",{"Kurier piętro wyżej","The upstairs courier"},0x88d3d9,
  {"    ▄▄████▄     ","   ███▀▀▀▀██    ","   ▀█ ▰  ▰ █    ","    █  ▵   █    ","    ▀█ ▬  █▀    ","   ▄██▓▓▓██▄    ","  ███╲▓▓╱████   ","  ███ ╲╱ ▣███   "}},
 {"ROZGŁOŚNIK 7",{"Głos porządku publicznego","Public order announcer"},0x91aebe,
  {"   ┌────────┐   ","   │ ┌────┐ │   ","   │ │ ▄▄ │ │   ","   │ └────┘ │   ","   │ ▤▤▤▤▤▤ │   ","   │ ▤▤▤▤▤▤ │   ","   │  ○  ▣  │   ","   └──┬──┬──┘   "}}
}};
struct Quest {
  Text title, objective, reward;
  int owner, district, required;
};
inline const std::array<Quest,8> Quests{{
 {{"Listy, które wypłynęły","Letters that surfaced"},{"Odszukaj trzy worki pocztowe na bocznych pomostach.","Find three mailbags on the side boardwalks."},{"Nowy szalik i wiadomości mieszkańców","A new scarf and letters from the residents"},0,0,3},
 {{"Ciśnienie społeczne","Social pressure"},{"Otwórz trzy boczne zawory bezpieczeństwa.","Open three auxiliary safety valves."},{"Skrót przez kotłownię","A shortcut through the boiler room"},1,1,3},
 {{"Bilet w jedną górę","One-way up"},{"Przywróć trzy sygnalizatory nieczynnego tramwaju.","Restore three signals on the abandoned tram line."},{"Tramwaj pospieszny przez osiedle","An express tram through the estate"},2,2,3},
 {{"Lokal do wyjaśnienia","An apartment to explain"},{"Sprawdź tabliczki na trzech zapomnianych balkonach.","Read the plaques on three forgotten balconies."},{"Klucz do dachu spółdzielni","The housing board's rooftop key"},3,2,3},
 {{"Odpis oryginału","Copy of the original"},{"Odzyskaj trzy teczki z zalanych półek archiwum.","Recover three files from the flooded archive shelves."},{"Dodatkowe zeznanie w dzienniku","An additional testimony in the journal"},4,3,3},
 {{"Na przyszłą wiosnę","For next spring"},{"Zbierz trzy kapsuły nasienne w górnych szklarniach.","Collect three seed capsules in the upper greenhouses."},{"Zielony płaszcz i ogród w przystani","A green coat and a harbor garden"},5,4,3},
 {{"Nie skrót, tylko doświadczenie","Experience, not a shortcut"},{"Wykonaj trzy skoki ze zrywu na trasie.","Perform three dash jumps on the route."},{"Próba mistrzowska Kuby","Kuba's mastery trial"},6,0,3},
 {{"Próba mikrofonu","Microphone test"},{"Dostrój trzy głośniki przy bocznych drogach.","Tune three speakers along the side routes."},{"Archiwalne komunikaty i emblemat","Archived announcements and an emblem"},7,1,3}
}};

inline std::vector<DialogueLine> introduction(int district) {
  switch (district) {
  case 0: return {
    {0,{"Jesteś. Zostawiłam ci suchą kurtkę pod ladą. Tę z urwanym guzikiem — drugiej nie mamy.","You're here. I left a dry coat under the counter. The one with the missing button — we haven't got another."}},
    {0,{"Górne dzielnice milczą od rana. Witek twierdzi, że ciepło znika z rur, zanim dotrze do mieszkań.","The upper districts have been silent all morning. Witek says heat is disappearing from the pipes before it reaches the homes."}},
    {0,{"Na tablicy na końcu nabrzeża są trzy odcinki: odpływ, przepompownia i droga do ciepłowni. Sprawdź połączenia po kolei.","The board at the far end of the quay has three routes: the drain, the pumping station, and the heating works approach. Check the connections in order."}},
    {0,{"Nie musisz być szybki. Musisz wrócić. Na pomostach świecą lampy powrotne. R zabierze cię do ostatniej.","You don't need to be fast. You need to come back. Return lamps mark the boardwalks. R takes you to the last one."}},
    {6,{"A jeśli jednak będziesz szybki, nie mów Helenie, że ci pokazałem. Skok po zrywie niesie dalej niż sam zryw.","And if you do happen to be fast, don't tell Helena I showed you. A jump out of a dash carries you farther than the dash alone."}},
    {0,{"Zajrzyj też na boczne pomosty. Została tam poczta. Nie będę cię poganiać, ale ludzie czekają.","Check the side boardwalks too. There's mail out there. I won't rush you, but people are waiting."}}
  };
  case 1: return {
    {1,{"O, poczta. Rachunek? Jeśli tak, wrzuć do pieca. Przynajmniej da trochę ciepła.","Oh, mail. A bill? Put it in the furnace. At least it will give us some heat."}},
    {1,{"Ktoś zamknął powrót z góry. Mogę zwiększyć ciśnienie, ale wtedy stopimy ścianę, na której stoi pół osiedla.","Someone shut the return line from above. I can raise the pressure, but we'd melt the wall holding half the estate."}},
    {1,{"Uruchom trzy odnogi awaryjne. Tablica stoi za kotłem. Windy parowe podnoszą cię z rozpędu. Skocz, kiedy platforma rusza w górę.","Start the three emergency branches. The board is beyond the boiler. Steam lifts carry momentum. Jump while the platform is rising."}}
  };
  case 2: return {
    {2,{"Tramwaj nie jeździ. Nie oznacza to, że nie obowiązuje rozkład.","The tram isn't running. That doesn't mean the timetable no longer applies."}},
    {3,{"Pani z czwartego piętra zostawiła leki u mnie. Bez tramwaju nie zejdzie po nie do wieczora. Rozkład możemy ustalić później.","The lady on the fourth floor left her medicine with me. Without the tram she can't collect it before tonight. We can sort out the timetable later."}},
    {2,{"Przywróć rozjazdy i otwórz drogę do archiwum. Na starym planie jest przewód, którego Witek nigdy nie widział.","Restore the junctions and open the archive route. The old plan shows a pipe Witek has never seen."}}
  };
  case 3: return {
    {4,{"Oryginał? Oczywiście. Proszę podać numer kopii, z której mam odtworzyć oryginał.","The original? Of course. Please provide the copy number from which I should reconstruct the original."}},
    {4,{"Żartuję. Odkąd zgasło ogrzewanie, żartuję coraz gorzej. Ale te braki w planach nie są żartem.","I'm joking. Since the heating stopped, my jokes have been getting worse. But these gaps in the plans are no joke."}},
    {4,{"Podczas dawnego załamania lodu zamknięto cały obieg. Dokumenty mówią: nikogo nie było. Listy lokatorów mówią co innego.","During the old ice collapse, an entire circuit was sealed. The reports say nobody was there. The resident lists disagree."}}
  };
  case 4: return {
    {5,{"Nie depcz koperku. Ratowanie cywilizacji nie zwalnia z podstawowej kultury.","Don't step on the dill. Saving civilization doesn't excuse bad manners."}},
    {5,{"Korzenie znalazły stary kanał. Ciepło wraca przez ogród, ale potrzebuje ujścia przy latarni.","The roots found an old channel. Heat is returning through the garden, but it needs an outlet at the beacon."}},
    {5,{"W archiwum wpisali zamknięcie. Rośliny nie czytają archiwów. Dlatego jeszcze tu jesteśmy.","The archive declared it closed. Plants don't read archives. That's why we're still here."}}
  };
  default: return {
    {6,{"Myślałem, że na górze będzie cieplej. To chyba najdroższa pomyłka w mojej karierze.","I thought it would be warmer up here. Probably the most expensive mistake of my career."}},
    {4,{"Masz pełny plan. Otwórz trzy przesłony latarni. Ciśnienie rozłoży się na wszystkie obiegi.","You have the complete plan. Open the beacon's three shutters. The pressure will spread across every circuit."}},
    {0,{"Słyszymy cię, kurierze. Cała przystań słucha. Tym razem nikogo nie wykreślimy z listy.","We can hear you, courier. The whole harbor is listening. This time nobody gets crossed off the list."}}
  };
}
}
inline std::vector<DialogueLine> conclusion(int district) {
  switch (district) {
  case 0: return {{0,{"Pompy działają. Pierwszy raz od rana z kranu leci woda, a nie argument przeciwko zamieszkaniu tutaj.","The pumps work. For the first time today the tap produces water instead of an argument against living here."}},
    {1,{"Tu Witek. Słyszę was. To nie awaria kotła. Ktoś zamknął powrót. Przyjdź do Łazarza, zanim zrobię coś głupiego.","Witek here. I hear you. It isn't the boiler. Someone shut the return line. Come to Lazarus before I do something stupid."}},
    {0,{"Przystań jest bezpieczna. Możesz odpocząć, wrócić po listy albo iść dalej. To już twoja trasa.","The harbor is safe. You can rest, go back for the letters, or move on. It's your route now."}}};
  case 1: return {{1,{"Obieg trzyma. Woda nie powinna płynąć tą stroną, ale najwyraźniej nie czyta instrukcji.","The circuit is holding. Water shouldn't flow this way, but apparently it doesn't read the manual."}},
    {1,{"Idź na Parapet. Bogdan pamięta wszystkie trasy. Nawet te, których podobno nigdy nie było.","Go to Windowsill. Bogdan remembers every route. Even the ones that supposedly never existed."}}};
  case 2: return {{2,{"Pierwszy wagon odjechał. Znalazłem pod pulpitem dawny rozkład: po przystanku jedenastym był jeszcze jeden. Ktoś przeciął papier dokładnie przez jego nazwę.","The first carriage has left. I found an old timetable under the controls: there was another stop after number eleven. Someone cut the paper straight through its name."}},
    {3,{"Zrobiłam dwie kopie. Jedną zabierzesz do Ireny, drugą zachowam tutaj. Pani z czwartego już odebrała leki. Kazała ci podziękować, zanim zacznę mówić o protokole.","I made two copies. Take one to Irena; I'll keep the other here. The lady on the fourth floor has collected her medicine. She told me to thank you before I start talking about the report."}}};
  case 3: return {{4,{"Mam nazwiska. Nie naprawisz tego, co wtedy zrobili. Ale możemy przestać udawać, że nikt tu nie mieszkał.","I have the names. You can't repair what they did then. But we can stop pretending nobody lived here."}},
    {4,{"Kanał przez ogród omija uszkodzoną ścianę. Celina miała rację od lat. Nikt jej nie pytał.","The garden channel bypasses the damaged wall. Celina has been right for years. Nobody asked her."}}};
  case 4: return {{5,{"Korzenie już grzeją. Została latarnia. Weź nasiona. To nie talizman. Po prostu ktoś musi je zanieść wyżej.","The roots are warming. Only the beacon remains. Take the seeds. They aren't a charm. Someone just needs to carry them higher."}},
    {6,{"Pójdę za tobą. Tym razem bez wyścigu.","I'll follow you. No race this time."}}};
  default: return {{0,{"Świeci. Widzimy światło przez lód. Nawet tutaj, na samym dole.","It's shining. We can see the light through the ice. Even here, at the very bottom."}},
    {1,{"Ciśnienie stabilne. Pierwszy raz w życiu nie mam nic do poprawienia młotkiem.","Pressure stable. For the first time in my life, there is nothing I need to fix with a hammer."}},
    {4,{"Spis mieszkańców został uzupełniony. Bez przypisu, bez skreśleń. Wszystkie nazwiska.","The resident register has been restored. No footnotes, no deletions. Every name."}},
    {0,{"Wracaj, kiedy chcesz. Mamy ciepło, mamy herbatę i niestety nadal mamy pocztę.","Come back whenever you like. We have heat, we have tea, and unfortunately we still have mail."}}};
  }
}
inline std::vector<DialogueLine> small_talk(int npc, unsigned quest_state) {
  if(quest_state==3) {
    static const Text after[]{
      {"W okienku znów stoi kolejka. Nigdy nie sądziłam, że powiem to z ulgą. Herbata jest po lewej. Reklamacje też, ale herbatę polecam bardziej.","There's a queue at the counter again. I never thought I'd say that with relief. Tea is on the left. So are complaints, but I recommend the tea."},
      {"Założyłem tabliczkę: działa. Grażyna kazała dopisać: na odpowiedzialność użytkownika. Ciepło na szczęście nie umie czytać.","I put up a sign: working. Grażyna made me add: at the user's own risk. Fortunately heat can't read."},
      {"Pierwszy kurs miał osiem minut opóźnienia. Pasażerowie bili brawo. Chyba nigdy już nie będę miał lepszego dnia w pracy.","The first service was eight minutes late. The passengers applauded. I may never have a better day at work."},
      {"Na dachu suszą koce. Oficjalnie to niedozwolona zmiana funkcji powierzchni wspólnej. Nieoficjalnie ja je tam zaniosłam.","They're drying blankets on the roof. Officially, that's an unauthorised change of common-space use. Unofficially, I carried them up there."},
      {"Otworzyłam czytelnię. Ktoś przyniósł zdjęcie rodziny. Pierwszy raz od dawna podpis pod zdjęciem był dłuższy niż numer teczki.","I opened the reading room. Someone brought a family photograph. For the first time in ages, its caption was longer than the file number."},
      {"W przystani wykiełkowała fasola. Najpierw ją zmierzyli, potem sporządzili protokół. Fasola w tym czasie urosła jeszcze trochę.","The beans sprouted in the harbor. First they measured them, then filed a report. The beans grew a little more meanwhile."},
      {"Ścigajmy się jeszcze kiedyś. Ale najpierw usiądźmy. Można robić obie rzeczy w życiu; sprawdziłem regulamin.","Let's race again sometime. But first let's sit down. You're allowed to do both in life; I checked the regulations."},
      {"KOMUNIKAT NIEARCHIWALNY. PAN WITEK PROSI O ZWROT KUBKA. PAN WITEK ZAZNACZA, ŻE KUBEK NIE JEST CZĘŚCIĄ INSTALACJI.","NON-ARCHIVED NOTICE. MR WITEK REQUESTS THE RETURN OF HIS MUG. MR WITEK STRESSES THAT THE MUG IS NOT PART OF THE INSTALLATION."}
    };return {{npc,after[std::clamp(npc,0,7)]}};
  }
  switch(npc) {
  case 0:return {{0,{"Trzy worki zostały na bocznych pomostach. Jeden miał iść na jedenastkę. Od rana ktoś stamtąd pyta, czy jest wiadomość.","Three bags are still on the side boardwalks. One was for number eleven. Someone there has been asking all morning whether there's any news."}}};
  case 1:return {{1,{"Za głównymi pomostami są trzy zawory bezpieczeństwa. Otwórz je. Po naprawie uruchomię ci wejście serwisowe — krótsza droga, mniej drabin.","There are three safety valves beyond the main platforms. Open them. Then I'll unlock the service entrance for you — a shorter route, fewer ladders."}}};
  case 2:return {{2,{"Nie potrzebuję nowego tramwaju. Potrzebuję, żeby trzy sygnalizatory przestały twierdzić, że jesteśmy na urlopie.","I don't need a new tram. I need three signals to stop claiming we're on holiday."}}};
  case 3:return {{3,{"Na trzech balkonach zostały stare tabliczki. Ktoś zamalował numery przy remoncie. Przynieś mi odpisy — bez nich znowu odeślą te rodziny z okienka.","Three balconies still have their old plaques. Someone painted over the numbers during repairs. Bring me copies — otherwise they'll turn those families away at the counter again."}}};
  case 4:return {{4,{"Trzy teczki zostały na mokrych półkach. Weź je w całości. Kolejność kartek pozwoli nam ustalić, co zmieniono.","Three files are still on the wet shelves. Bring them intact. The order of the pages will help us establish what was changed."}}};
  case 5:return {{5,{"W górnych szklarniach zostały trzy kapsuły nasion. Na dole nie urosną od razu, ale to nie powód, żeby nie sadzić.","Three seed capsules remain in the upper greenhouses. They won't grow downstairs immediately, but that's no reason not to plant them."}}};
  case 6:return {{6,{"Zrób zryw po ziemi, a zaraz potem skocz. Zachowasz rozpęd. Trzy udane próby i uznam, że możesz mnie wyprzedzać legalnie.","Dash along the ground, then jump immediately. You'll keep the momentum. Three good attempts and I'll consider your overtaking legal."}}};
  default:return {{7,{"TEST MIKROFONU. Proszę dostroić trzy urządzenia boczne. Po zakończeniu testu cisza zostanie przywrócona do normy.","MICROPHONE TEST. Please tune three auxiliary units. Upon completion, silence will return to regulation levels."}}};
  }
}
} // namespace iceberg

namespace iceberg {
inline std::vector<DialogueLine> hidden_testimony(int id) {
  if(id==0)return {
    {-1,{"Na kartce jest dziecięcy plan mieszkania. Kuchnia, pokój mamy, schowek. Za ścianą schowka dorysowano jeszcze jeden pokój. Bez drzwi.","The paper shows a child's apartment plan. Kitchen, Mum's room, cupboard. Another room is drawn behind the cupboard wall. No door."}},
    {-1,{"Na odwrocie: Jeśli ogłoszą próbę syreny, nie zamykaj okna. Babcia nie słyszy. Numer mieszkania poprawiano tak długo, aż papier pękł.","On the back: If they announce a siren test, don't close the window. Grandma can't hear. The apartment number was corrected until the paper tore."}},
    {-1,{"Podpis konserwatora: Głośnik szkoleniowy, górny pomost. Sygnał mieszkańców nadal wraca tą linią. Nie kasować przed odczytem.","Maintenance note: Training loudspeaker, upper landing. The residents' signal still returns along that line. Do not erase before reading."}}
  };
  if(id==1)return {
    {7,{"Próba komunikatu. Mieszkańcy klatki jedenastej proszeni są o pozostanie w mieszkaniach. To tylko próba. To tylko próba.","Announcement test. Residents of stairwell eleven are asked to remain in their apartments. This is only a test. This is only a test."}},
    {7,{"Odpowiedzi: czterdzieści jeden. Potwierdzenia odbioru: czterdzieści. Oczekuję na potwierdzenie. Oczekuję na potwierdzenie.","Replies: forty-one. Acknowledgements: forty. Awaiting acknowledgement. Awaiting acknowledgement."}},
    {7,{"Błąd spisu. Mieszkańców: zero. Odpowiedzi: czterdzieści jeden. Nie umiem odjąć odpowiedzi od mieszkańców.","Register error. Residents: zero. Replies: forty-one. I cannot subtract replies from residents."}},
    {7,{"Linia kończy się nad kołem wyrównawczym. Proszę sprawdzić górną platformę przepompowni. Nie rozłączać mnie, zanim ktoś odbierze.","The line ends above the equalization wheel. Please check the pumping station's upper platform. Do not disconnect me before someone answers."}}
  };
  return {
    {-1,{"Protokół mówi, że zamknięcie miało potrwać siedem minut. Pierwszy podpis jest stanowczy. Drugi drży. Pod trzecim zostało tylko wgłębienie długopisu.","The log says the closure was meant to last seven minutes. The first signature is firm. The second shakes. Of the third, only the pen's impression remains."}},
    {-1,{"Mieszkańcy odpowiadali przez czujniki drgań w rurach. System zapisywał ich jako zakłócenia. Potem ktoś zmienił kategorię: odgłosy eksploatacyjne.","Residents replied through vibration sensors in the pipes. The system logged them as interference. Then someone changed the category: operational noise."}},
    {7,{"Ostatnia odpowiedź nie przyszła z zamkniętej klatki. Przyszła stąd. Siedem minut po zabraniu ostatniego pracownika.","The last reply did not come from the sealed stairwell. It came from here. Seven minutes after the last worker was taken out."}},
    {-1,{"Na wewnętrznej stronie drzwi ktoś wyrył: Nie trzeba otwierać. Trzeba pamiętać. Zabierasz kopię protokołu. Oryginał zostaje.","Someone scratched the inside of the door: You don't have to open it. You have to remember. You take a copy of the log. The original stays."}}
  };
}
}

namespace iceberg {
inline std::vector<DialogueLine> homecoming(int district,int leg) {
  switch(district) {
  case 0:
    if(leg==0)return {
      {0,{"Pierwszy kabel odpowiada. Trzydzieści sześć mieszkań znów wie, która jest godzina. Niestety wszystkie myślą, że szósta rano.","The first cable responds. Thirty-six homes know the time again. Unfortunately they all think it's six in the morning."}},
      {6,{"Widziałem prom pod lodem. Ładownia jest sucha, ale kabina siedzi w bryle jak mucha w bursztynie. Ktoś nadal zapala tam lampę.","I saw the ferry under the ice. Its hold is dry, but the cabin sits in a block like a fly in amber. Someone still lights a lamp there."}},
      {0,{"To Bogdan. Pilnuje tramwajów, promów i ostatnich resztek rozkładu. Teraz przejdź przez nabrzeże do regulatora odpływu.","That's Bogdan. He guards the trams, the ferries, and the last scraps of the timetable. Cross the quay to the drainage regulator next."}}};
    return {
      {0,{"Woda przestała rosnąć. Sąsiad z parteru odwołał rezerwację na mieszkanie w łódce.","The water stopped rising. The ground-floor neighbour cancelled his reservation for an apartment in a boat."}},
      {6,{"Przy przepompowni pracuje stary automat kontrolny. Nie rozpoznaje nowej przepustki. Przeskocz go albo użyj zrywu, kiedy będzie blisko.","An old inspection drone is working by the pumping station. It doesn't recognise the new pass. Jump over it or dash when it's close."}},
      {0,{"Trzeci odcinek prowadzi do głównego koła. Otwórz je ręcznie. Witek odbierze cię po drugiej stronie, jeśli jeszcze pamięta, co to znaczy odbierać pocztę.","The third route leads to the main wheel. Open it by hand. Witek will meet you on the other side, if he still remembers what receiving mail means."}}};
  case 1:
    if(leg==0)return {{1,{"Ciśnienie wróciło. Przyłożyłem ucho do rury i usłyszałem sąsiada. Twierdzi, że ma zimny kaloryfer. Kanał informacyjny działa.","The pressure's back. I put my ear to the pipe and heard a neighbour. He says his radiator is cold. The information channel works."}},
      {7,{"OBIEG POMOCNICZY: SPRAWNY. OBIEG ODPOWIEDZIALNOŚCI: ZAMKNIĘTY. WNIOSEK PRZEKAZANO DO INNEGO DZIAŁU.","AUXILIARY CIRCUIT: OPERATIONAL. RESPONSIBILITY CIRCUIT: CLOSED. REQUEST FORWARDED TO ANOTHER DEPARTMENT."}}};
    return {{1,{"Pod izolacją znalazłem plomby sprzed dwudziestu lat. Ktoś odcinał górę już wcześniej. I zrobił to bardzo starannie.","I found twenty-year-old seals under the insulation. Someone cut off the upper levels before. And they did it very carefully."}},
      {1,{"Nie będę zgadywał, po co. Najpierw otwórz wyjście. Potem zapytamy ludzi, którzy zachowali papier.","I won't guess why. Open the outlet first. Then we'll ask the people who kept the paperwork."}}};
  case 2:
    if(leg==0)return {{3,{"Połączyłeś dwie ulice. Mieszkańcy zdążyli już pokłócić się o nazwę wspólnego przystanku. To dobry znak.","You connected two streets. The residents have already argued about the name of the shared stop. That's a good sign."}},
      {2,{"Na pierwszym kursie pojadą leki. Na drugim dzieci do ogrodu. Na trzecim ja, jeśli ktoś mnie wreszcie zastąpi.","The first service carries medicine. The second takes the children to the garden. The third carries me, if someone finally covers my shift."}}};
    return {{3,{"Mam listę lokali. Archiwum ma drugą. Różnią się o całą klatkę schodową. Nie, to nie jest problem zaokrąglenia.","I have a list of apartments. The archive has another. They differ by an entire stairwell. No, this is not a rounding issue."}},
      {2,{"Jedź do Ireny. Znasz już drogę przez przewieszone ulice. Dalej będzie cicho, ale to wciąż czyjeś miasto.","Go to Irena. You know the suspended streets now. It gets quiet beyond them, but it's still somebody's city."}}};
  case 3:
    if(leg==0)return {{4,{"Plan instalacji ma sześć warstw. Pięć pokazuje rury. Szósta pokazuje, kto kazał udawać, że ich nie ma.","The installation plan has six layers. Five show pipes. The sixth shows who ordered everyone to pretend they weren't there."}},
      {7,{"KOMUNIKAT ARCHIWALNY. W CZASIE AWARII WSZYSCY MIESZKAŃCY ZOSTALI EWAKUOWANI. ŹRÓDŁO: KOMUNIKAT ARCHIWALNY.","ARCHIVED NOTICE. ALL RESIDENTS WERE EVACUATED DURING THE EMERGENCY. SOURCE: ARCHIVED NOTICE."}}};
    return {{4,{"Mam zeznania ratowników. Zamknięcie obiegu uratowało dolne piętra. Ale potem nikt nie wrócił, żeby sprawdzić górę. Wygodniej było zamknąć również sprawę.","I found the rescuers' testimony. Closing the circuit saved the lower levels. But nobody returned to check upstairs. It was easier to close the case too."}},
      {4,{"Nie poprawimy tamtego dnia. Możemy przestać go ukrywać. Kopie pojadą do każdej dzielnicy razem z ciepłem.","We can't repair that day. We can stop hiding it. Copies will travel to every district along with the heat."}}};
  case 4:
    if(leg==0)return {{5,{"Pierwsza szklarnia oddycha. Nie wiem, czy wiesz, ale pomidor nie ma obowiązku ratować ludzkości. Ten akurat się zgodził.","The first greenhouse is breathing. You may not know this, but a tomato has no obligation to save humanity. This one agreed."}},
      {0,{"Na dole znowu pachnie ziemią. Ludzie pytają, czy to przeciek. Powiedziałam, że tym razem to przyszłość.","It smells of soil downstairs again. People ask if it's a leak. I told them this time it's the future."}}};
    return {{5,{"Korzenie wchodzą w szczeliny, których nie ma na planach. Poszły za ciepłem. Gdyby urzędnicy robili to samo, oszczędzilibyśmy sporo papieru.","The roots enter cracks that aren't on the plans. They followed the warmth. If officials did the same, we'd save a lot of paper."}},
      {5,{"Przy wyjściu został drugi automat. Omijaj jego ramię. Za nim jest przewód do Korony i pierwszy prawdziwy wiatr.","The second drone is by the outlet. Avoid its arm. Beyond it is the pipe to the Crown, and the first real wind."}}};
  default:
    if(leg==0)return {{6,{"Ścigałem się z tobą, żeby nie myśleć o tym, jak wysoko jesteśmy. Teraz jesteśmy za wysoko, żeby to działało.","I raced you so I wouldn't think about how high we were. Now we're too high for that to work."}},
      {6,{"Pójdę sprawdzić boczny pomost. Ty trzymaj główną linię. Tym razem naprawdę nie musisz mnie doganiać.","I'll check the side landing. You stay on the main line. This time you really don't need to catch me."}}};
    return {{4,{"Wszystkie dzielnice potwierdziły odbiór. Nie zostało już jedno biuro, które może schować dokumenty. Każdy ma własną kopię.","Every district acknowledged receipt. There's no single office left that can hide the documents. Everyone has a copy."}},
      {6,{"Widzisz ten jasny pas? To nie lampa. Ja też przez chwilę myślałem, że trzeba ją wymienić.","See that bright strip? It isn't a lamp. For a moment I also thought we'd have to replace it."}}};
  }
}
inline std::vector<DialogueLine> task_thanks(int quest) {
  switch(quest) {
  case 0:return {{0,{"Ten list leżał w wodzie czternaście lat. Adresat nadal mieszka naprzeciwko. Nie pytaj, jak to wpisać w termin doręczenia.","This letter spent fourteen years in the water. The recipient still lives opposite. Don't ask how to enter that in the delivery schedule."}},
    {0,{"Weź szalik. Został w okienku, kiedy ostatni raz ktoś przyszedł tu bez pośpiechu. Teraz wybierzesz go w ustawieniach stroju.","Take this scarf. It was left at the counter the last time someone came here without rushing. You can select it in the outfit settings now."}}};
  case 1:return {{1,{"Wszystkie trzy puszczają. Słyszysz? Nic nie gwiżdże. Najładniejszy dźwięk w tej robocie.","All three are releasing. Hear that? Nothing's whistling. Best sound in this job."}},
    {1,{"Otworzyłem ci wejście serwisowe na początku odcinków ciepłowni. Ominiesz kilka pomostów, kiedy będziesz wracać.","I opened the service entrance at the start of the heating works routes. You can skip a few landings when you return."}}};
  case 2:return {{2,{"Zielone światło. Po tylu latach wygląda jak błąd. Od początku każdego odcinka osiedla pojedziesz teraz pospiesznym. Omija trzy przystanki.","A green light. After all these years it looks like an error. An express service now leaves from the start of each estate route. It skips three stops."}},
    {2,{"Bilet zachowaj. Nie dlatego, że sprawdzam. Po prostu jest na nim dzisiejsza data.","Keep the ticket. Not because I'll check. It just has today's date on it."}}};
  case 3:return {{3,{"Trzy mieszkania, trzy nazwiska. Formularz nie przewiduje takiego sukcesu. Dopiszę rubrykę ręcznie.","Three apartments, three names. The form doesn't allow for this much success. I'll add a field by hand."}},
    {3,{"Klucz do dachu jest twój. Pelargonie proszę omijać. Nawet podczas bohaterskiego czynu.","The roof key is yours. Please avoid the geraniums. Even during heroic acts."}}};
  case 4:return {{4,{"W jednej teczce była prośba o dodatkowy koc. Podpisano ją po dacie rzekomej ewakuacji. Zostawiłam odpis w twoim dzienniku.","One file contained a request for an extra blanket. It was signed after the supposed evacuation. I left a copy in your journal."}},
    {4,{"Dziękuję, że przyniosłeś papier, a nie odpowiedź, którą najłatwiej byłoby nam przyjąć.","Thank you for bringing the paper, rather than the answer we'd find easiest to accept."}}};
  case 5:return {{5,{"Trzy kapsuły. Na pierwszej napiszę: cebula. Na drugiej: fasola. Na trzeciej: nie wyrzucać, nawet jeśli wygląda jak błąd.","Three capsules. I'll label the first: onion. The second: beans. The third: do not discard, even if it looks like a mistake."}},
    {5,{"Helena dała mi skrawek nabrzeża. Posadzimy tam mały ogród. A dla ciebie mam zielony płaszcz, żebyś nie straszył sadzonek.","Helena gave me a little patch of quay. We'll plant a garden there. I have a green coat for you, so you don't frighten the seedlings."}}};
  case 6:return {{6,{"Dobra. Oficjalnie jesteś szybki. Nieoficjalnie byłem szybszy, kiedy nikt nie patrzył.","Fine. Officially, you're fast. Unofficially, I was faster when nobody was looking."}},
    {6,{"Próby mistrzowskie masz w menu. Osobno liczymy czas, powroty i znajdźki. Nie musisz robić wszystkiego w jednym biegu.","Mastery trials are in the menu. Time, retries, and treasures have separate records. You don't have to do everything in one run."}}};
  default:return {{7,{"TEST ZAKOŃCZONY. GŁOŚNIKI SPRAWNE. DODATKOWY KOMUNIKAT: DZISIEJSZE DZIAŁANIE POMOGŁO LUDZIOM. BRAK FORMULARZA DLA TEGO KOMUNIKATU.","TEST COMPLETE. SPEAKERS OPERATIONAL. ADDITIONAL NOTICE: TODAY'S ACTION HELPED PEOPLE. NO FORM EXISTS FOR THIS NOTICE."}},
    {7,{"DZIĘKUJĘ. TO SŁOWO BYŁO W PAMIĘCI OD DAWNA. NIE MIAŁEM WŁAŚCIWEJ OKAZJI.","THANK YOU. THAT WORD HAS BEEN IN MEMORY FOR A LONG TIME. I HADN'T FOUND THE RIGHT OCCASION."}}};
  }
}
}

namespace iceberg {
inline std::vector<DialogueLine> task_record(int quest) {
  switch(quest) {
  case 0:return {
    {-1,{"Pani Heleno, jeśli list dotrze po urodzinach, proszę powiedzieć mamie, że śnieg zatrzymał tramwaj. Nie trzeba dodawać, że pod ziemią. Ona wie, jak tu jest. — Antek", "Helena, if this arrives after the birthday, please tell Mum the snow stopped the tram. No need to add that it was underground. She knows what it's like here. — Antek"}},
    {-1,{"Do lokalu 12: pożyczony garnek stoi pod drzwiami. Zupa została w środku. Nie jest to odsetka za opóźnienie; po prostu ugotowaliśmy za dużo. — sąsiedzi spod 15", "To flat 12: the borrowed pot is outside your door. The soup is still inside. It isn't interest on the delay; we simply made too much. — your neighbours at 15"}},
    {-1,{"Dopisek Heleny: dwa listy doręczone. Trzeci ma adres zamazany wodą. Powiesiłam kopię w okienku. Ktoś rozpozna pismo. Pismo czasem pamięta więcej niż spis lokatorów.", "Helena's note: two letters delivered. Water erased the third address. I put a copy in the window. Someone will recognise the handwriting. Sometimes handwriting remembers more than a resident register."}}
  };
  case 1:return {{-1,{"Przepustka serwisowa, podpis: Witek. Na początku tras ciepłowni stoi teraz wejście z zawiniętą strzałką. Prowadzi trzy pomosty dalej. Proszę nie oddawać klucza administracji. Zrobią z niego procedurę.", "Service pass, signed Witek. A doorway with a curved arrow now stands at the start of heating works routes. It leads three landings ahead. Please don't give the key to administration. They'll turn it into a procedure."}}};
  case 2:return {{-1,{"Bilet ważny do końca dzisiejszego dnia, niezależnie od tego, ile razy dziś zaczniemy od nowa. Pospieszny odjeżdża z początku odcinków osiedla i omija trzy przystanki. Dopisek Bogdana: kontrola biletów zawieszona z powodu ogólnej przyzwoitości.", "Valid until the end of today, however many times we start today again. The express leaves from the start of estate routes and skips three stops. Bogdan's note: ticket inspections suspended on grounds of general decency."}}};
  case 3:return {{-1,{"Potwierdza się istnienie balkonów 4, 8 i 11. Potwierdza się istnienie osób, które na nich wieszają pranie. Nie wymaga się dalszych dowodów. Podpis: Grażyna. Dopisek: proszę tę wersję zostawić w obiegu.", "Balconies 4, 8 and 11 are confirmed to exist. So are the people hanging laundry on them. No further evidence is required. Signed: Grażyna. Note: please keep this version in circulation."}}};
  case 4:return {{-1,{"Wniosek o dodatkowy koc. Data: trzy dni po zakończeniu ewakuacji. Uzasadnienie: córka śpi przy ścianie zewnętrznej. Adnotacja urzędowa: lokalu nie ma w wykazie. Dopisek Ireny: wykaz poprawiono. Wniosku już nie cofniemy.", "Request for an extra blanket. Date: three days after the evacuation ended. Reason: my daughter sleeps by the outer wall. Official annotation: apartment absent from the register. Irena's note: register corrected. We cannot undo the request."}}};
  case 5:return {{-1,{"Przepis Celiny na wiosnę: trochę ziemi, trochę ciepła, coś do posadzenia. Nie czekać na zgodę komisji. Pierwszą fasolę oddać sąsiadom; wtedy reszta urośnie spokojniej, bo wszyscy będą pilnować.", "Celina's recipe for spring: some soil, some warmth, something to plant. Don't wait for the committee's permission. Give the first beans to the neighbours; the rest will grow more peacefully with everyone watching over them."}}};
  case 6:return {
    {-1,{"Notatka Kuby: zryw po ziemi, potem skok. Rozpęd poniesie cię przez niski przelot. Przy ścianie: zryw w górę, potem skok odbijający. Na windzie: skok, gdy jedzie w górę. Nie wszystko naraz. Najpierw znajdź bezpieczny pomost.", "Kuba's note: dash along the ground, then jump. Momentum carries you through a low crossing. Beside a wall: dash up, then jump away. On a lift: jump while it rises. Not all at once. Find a safe landing first."}},
    {-1,{"W próbie liczymy trzy osobne rzeczy: najlepszy czas, najmniej powrotów i najwięcej znajdziek. Zrób spokojny bieg bez upadku, potem szybki, potem poszukaj bocznych dróg. Jeden rekord nie kasuje pozostałych.", "Trials track three separate things: best time, fewest retries, and most treasures. Try a calm clean run, then a fast one, then explore the branches. One record doesn't erase the others."}}
  };
  default:return {
    {7,{"ARCHIWUM KOMUNIKATÓW, WPIS 184. PROSIMY NIE SUSZYĆ SKARPET NA MANOMETRZE. CIŚNIENIE POWINNO BYĆ WIDOCZNE TAKŻE W DNI NIEPARZYSTE.", "ANNOUNCEMENT ARCHIVE, ENTRY 184. PLEASE DO NOT DRY SOCKS ON THE PRESSURE GAUGE. PRESSURE MUST REMAIN VISIBLE ON ODD-NUMBERED DAYS TOO."}},
    {7,{"WPIS 185. WSZYSCY MIESZKAŃCY OPUŚCILI ODCINEK PÓŁNOCNY. WERYFIKACJA: PRZENIESIONA DO NASTĘPNEGO PROTOKOŁU. NASTĘPNY PROTOKÓŁ: BRAK ZAŁĄCZNIKA.", "ENTRY 185. ALL RESIDENTS HAVE LEFT THE NORTHERN CIRCUIT. VERIFICATION: DEFERRED TO THE NEXT REPORT. NEXT REPORT: ATTACHMENT MISSING."}},
    {7,{"WPIS 186. ZNALEZIONO KOTA. KOT NIE POTWIERDZA, ŻE SIĘ ZGUBIŁ. ODBIÓR W CIEPŁOWNI, PRZY NAJCIEPLEJSZEJ RURZE.", "ENTRY 186. A CAT HAS BEEN FOUND. THE CAT DOES NOT CONFIRM IT WAS LOST. COLLECTION AT THE HEATING WORKS, BESIDE THE WARMEST PIPE."}}
  };
}
}
} // namespace iceberg

namespace iceberg {
inline std::vector<DialogueLine> mission_arrival(int mission) {
  if(mission==0)return {
    {0,{"Bogdan utknął w tramwaju pod nabrzeżem. Twierdzi, że to postój techniczny. Od dwóch godzin. Uruchom wyciąg przy czerwonej korbie, a potem daj mu znać.", "Bogdan is stranded in the tram below the quay. He calls it a technical stop. It's been two hours. Start the winch at the red crank, then let him know."}},
    {2,{"Słyszę was. Proszę bez paniki. Rozkład dopuszcza opóźnienia wynikające z obecności oceanu. Z powrotem idź dolnym pomostem; górne przejście się zapadło.", "I can hear you. Please stay calm. The timetable allows for delays caused by the presence of an ocean. Take the lower boardwalk back; the upper passage has collapsed."}}
  };
  if(mission==1)return {
    {0,{"Witek potrzebuje starego termometru wzorcowego. Nowe pokazują temperaturę zgodną z planem. Ten pokazuje prawdziwą. Weź skrzynkę przy wejściu i zanieś do śluzy pocztowej.", "Witek needs the old reference thermometer. The new ones show the planned temperature. This one shows the real one. Take the box by the entrance to the postal airlock."}},
    {0,{"Trzymaj pionowo. Ze skrzynką nie zrobisz zrywu, ale skakać możesz. Droga ma krótkie odstępy. Lampy nadal działają; po potknięciu przesyłka wraca razem z tobą.", "Keep it upright. You can't dash with the box, but you can jump. The route has short gaps. Return lamps still work; if you fall, the parcel returns with you."}}
  };
  return {
    {1,{"Znalazłem stare śluzy. W dokumentacji są opisane jako nieczynne od zawsze. Silniki są jeszcze ciepłe. Otwórz oba obiegi i przejdź na drugi brzeg.", "I found the old sluices. The paperwork calls them permanently inactive. The motors are still warm. Open both circuits and cross to the other bank."}},
    {6,{"Barki same jeżdżą między pomostami. Przejdź po pokładzie i skocz z końca, kiedy jesteś blisko brzegu. Nie musisz rzucać się od razu po lądowaniu.", "The barges move between the landings by themselves. Walk along the deck and jump from its end when you're near the bank. You don't have to leap the moment you land."}}
  };
}
inline std::vector<DialogueLine> rescued_conductor() {
  return {
    {2,{"Jest zasilanie. Mogę wyjechać. Miałem na pokładzie jedną pasażerkę i osiemnaście pustych doniczek. Pasażerka wysiadła godzinę temu. Doniczki wykazują większą cierpliwość.", "Power's back. I can get out. I had one passenger and eighteen empty flowerpots aboard. The passenger got off an hour ago. The flowerpots have been more patient."}},
    {2,{"Dziękuję. Spotkamy się na Parapecie. Zachowałem stary plan torów; może się przydać, jeśli będziesz szukać drogi, której urzędowo nie ma.", "Thank you. We'll meet at Windowsill. I kept the old track map; it may help if you ever need a route that officially doesn't exist."}}
  };
}
inline std::vector<DialogueLine> parcel_delivered() {
  return {
    {1,{"Mam go. Osiem stopni różnicy. Przez cały dzień grzaliśmy zgodnie z papierem, a ludzie marzli zgodnie z rzeczywistością. Dobrze, że ktoś jeszcze dostarcza rzeczy osobiście.", "Got it. Eight degrees off. All day we've heated according to the paperwork while people froze according to reality. Good thing someone still delivers things in person."}},
    {0,{"Przesyłka doręczona. Podpis odbiorcy: plama smaru. Przyjmuję. Znam ten charakter pisma.", "Parcel delivered. Recipient's signature: a grease stain. Accepted. I recognise the handwriting."}}
  };
}
}

#include "iceberg_jubilee_story.hpp"

namespace iceberg {
inline const Text &training_sign(int id) {
  static const std::array<Text,8> signs{{
        {"A/D: ruch. Spacja: skok. Przytrzymaj, żeby skoczyć wyżej. Czerwone krawędzie są niebezpieczne. Lampy zapisują miejsce powrotu.","A/D: move. Space: jump. Hold for a higher jump. Red edges are dangerous. Lamps save your return point."},
        {"Zryw: X albo Shift z kierunkiem. Żółta postać ma gotowy zryw. Na ziemi: zryw, potem szybko skok — zachowasz rozpęd.","Dash: X or Shift with a direction. A yellow character has a dash ready. On the ground: dash, then quickly jump to keep your momentum."},
        {"Przy ścianie zrób zryw w górę, a potem skocz. Na windzie skacz w chwili ruchu w górę. Górna znajdźka jest opcjonalna.","Dash upward beside a wall, then jump. On a lift, jump as it rises. The upper treasure is optional."},
        {"Winda daje ci swój rozpęd. Skacz w chwili, gdy rusza w górę. Możesz zaczekać na pełnym pomoście. Boczne lampy nie są częścią obowiązkowej trasy.","A lift shares its momentum with you. Jump while it is rising. You can wait on the solid landings. Side lamps are not part of the required route."},
        {"Żółte kropki oznaczają, że zawór zaraz wypuści parę. Czerwony słup parzy. Zaczekaj albo przeskocz nad nim; zryw nie chroni przed ciepłem.","Yellow dots mean a vent is about to release steam. The red column burns. Wait or jump over it; a dash doesn't protect against heat."},
        {"Górne platformy są dla ciekawskich. Skok z wznoszącej windy, potem zryw w górę. Jeśli się nie uda, lampa jest tuż obok. Niczego nie tracisz.","The upper platforms are for the curious. Jump from a rising lift, then dash upward. If it doesn't work, the lamp is nearby. You lose nothing."},
        {"Tramwaj oddaje rozpęd przy skoku. Skacz, gdy jedzie w stronę następnego pomostu. Jeśli miniesz przystanek, S albo dół pozwoli zejść przez podłogę wagonika.","A tram shares its momentum when you jump. Leap while it moves toward the next landing. If you miss a stop, S or down lets you drop through its floor."},
        {"Balkony po drugiej stronie są opcjonalne. Główna trasa biegnie przez oświetlone przystanki. Nad klatkami można łączyć skok ze zrywu z odbiciem od ściany.","The balconies on the far side are optional. The main route follows the lit stops. Above the stairwells, you can combine dash jumps with wall bounces."}
  }};
  return signs[size_t(std::clamp(id,0,7))];
}
}

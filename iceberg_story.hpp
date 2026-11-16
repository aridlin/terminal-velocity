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
 {{"Bilet w jedną górę","One-way up"},{"Przywróć trzy sygnalizatory nieczynnego tramwaju.","Restore three signals on the abandoned tram line."},{"Przejazd między dzielnicami","Travel between districts"},2,2,3},
 {{"Lokal do wyjaśnienia","An apartment to explain"},{"Sprawdź tabliczki na trzech zapomnianych balkonach.","Read the plaques on three forgotten balconies."},{"Klucz do dachu spółdzielni","The housing board's rooftop key"},3,2,3},
 {{"Odpis oryginału","Copy of the original"},{"Odzyskaj trzy teczki z zalanych półek archiwum.","Recover three files from the flooded archive shelves."},{"Dodatkowe zeznanie w dzienniku","An additional testimony in the journal"},4,3,3},
 {{"Na przyszłą wiosnę","For next spring"},{"Zbierz trzy kapsuły nasienne w górnych szklarniach.","Collect three seed capsules in the upper greenhouses."},{"Zielony płaszcz i ogród w przystani","A green coat and a harbor garden"},5,4,3},
 {{"Nie skrót, tylko doświadczenie","Experience, not a shortcut"},{"Wykonaj trzy skoki ze zrywu na trasie.","Perform three dash jumps on the route."},{"Próba mistrzowska Kuby","Kuba's mastery trial"},6,0,3},
 {{"Próba mikrofonu","Microphone test"},{"Dostrój trzy głośniki przy bocznych drogach.","Tune three speakers along the side routes."},{"Archiwalne komunikaty i emblemat","Archived announcements and an emblem"},7,3,3}
}};

inline std::vector<DialogueLine> introduction(int district) {
  switch (district) {
  case 0: return {
    {0,{"Jesteś. Dobrze. Miałam wysłać po ciebie kuriera, ale to byś znowu był ty.","You're here. Good. I was going to send a courier for you, but that would be you again."}},
    {0,{"Górne dzielnice milczą od rana. Witek twierdzi, że ciepło znika z rur, zanim dotrze do mieszkań.","The upper districts have been silent all morning. Witek says heat is disappearing from the pipes before it reaches the homes."}},
    {0,{"Na tablicy są trzy odcinki: odpływ, przepompownia i droga do ciepłowni. Sprawdź połączenia po kolei.","The board has three routes: the drain, the pumping station, and the heating works approach. Check the connections in order."}},
    {0,{"Nie musisz być szybki. Musisz wrócić. Na pomostach świecą lampy powrotne. R zabierze cię do ostatniej.","You don't need to be fast. You need to come back. Return lamps mark the boardwalks. R takes you to the last one."}},
    {6,{"A jeśli jednak będziesz szybki, nie mów Helenie, że ci pokazałem. Skok po zrywie niesie dalej niż sam zryw.","And if you do happen to be fast, don't tell Helena I showed you. A jump out of a dash carries you farther than the dash alone."}},
    {0,{"Wszyscy mieszkamy w lodzie. Nie wszyscy musimy mieć lód w głowie. Idź ostrożnie.","We all live in ice. We don't all need ice in our heads. Take care."}}
  };
  case 1: return {
    {1,{"O, poczta. Rachunek? Jeśli tak, wrzuć do pieca. Przynajmniej da trochę ciepła.","Oh, mail. A bill? Put it in the furnace. At least it will give us some heat."}},
    {1,{"Ktoś zamknął powrót z góry. Mogę zwiększyć ciśnienie, ale wtedy stopimy ścianę, na której stoi pół osiedla.","Someone shut the return line from above. I can raise the pressure, but we'd melt the wall holding half the estate."}},
    {1,{"Uruchom trzy odnogi awaryjne. Windy parowe podnoszą cię z rozpędu. Skocz, kiedy platforma rusza w górę.","Start the three emergency branches. Steam lifts carry momentum. Jump while the platform is rising."}}
  };
  case 2: return {
    {2,{"Tramwaj nie jeździ. Nie oznacza to, że nie obowiązuje rozkład.","The tram isn't running. That doesn't mean the timetable no longer applies."}},
    {3,{"Mamy dwadzieścia siedem skarg na zimno i jedną na zbyt głośną ciszę. Tę ostatnią złożyłam osobiście.","We have twenty-seven complaints about the cold and one about the silence being too loud. I filed that last one myself."}},
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
  case 2: return {{2,{"Następny przystanek: Archiwum. Proszę uważać na odstęp między prawdą a komunikatem.","Next stop: the Archive. Please mind the gap between the truth and the announcement."}},
    {3,{"Dziękuję. Nie wpiszę tego do protokołu, bo wtedy musiałabym też wpisać, że potrzebowaliśmy pomocy.","Thank you. I won't put that in the minutes, because then I'd have to record that we needed help."}}};
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
  if (quest_state == 3) return {{npc,{"Dobra robota. Nie każda droga musi prowadzić wyżej. Czasem warto wrócić do ludzi.","Good work. Not every road needs to go higher. Sometimes it's worth coming back to people."}}};
  switch(npc) {
  case 0:return {{0,{"Trzy worki pocztowe zostały na bocznych pomostach. Suche listy są na wagę złota. Mokre też, tylko więcej ważą.","Three mailbags were left on the side boardwalks. Dry letters are worth their weight in gold. Wet ones too, but they weigh more."}}};
  case 1:return {{1,{"Boczne zawory tkwią za głównymi pomostami. Otwórz trzy. Narzędzie uniwersalne? E. Wreszcie ktoś je ustandaryzował.","The side valves are beyond the main platforms. Open three. Universal tool? E. Someone finally standardized it."}}};
  case 2:return {{2,{"Nie potrzebuję nowego tramwaju. Potrzebuję, żeby trzy sygnalizatory przestały twierdzić, że jesteśmy na urlopie.","I don't need a new tram. I need three signals to stop claiming we're on holiday."}}};
  case 3:return {{3,{"Trzy balkony nie mają mieszkań w ewidencji. Mają za to pelargonie. Pelargonie nie kłamią w sprawie meldunku.","Three balconies have no registered apartments. They do have geraniums. Geraniums don't lie about residency."}}};
  case 4:return {{4,{"Szukam trzech teczek. Nie błyszczą. Wiem, fatalna wada dokumentów w terenie.","I'm looking for three files. They don't glow. I know, a terrible flaw in field documentation."}}};
  case 5:return {{5,{"W górnych szklarniach zostały trzy kapsuły nasion. Na dole nie urosną od razu, ale to nie powód, żeby nie sadzić.","Three seed capsules remain in the upper greenhouses. They won't grow downstairs immediately, but that's no reason not to plant them."}}};
  case 6:return {{6,{"Zrób zryw po ziemi, a zaraz potem skocz. Zachowasz rozpęd. Trzy udane próby i uznam, że możesz mnie wyprzedzać legalnie.","Dash along the ground, then jump immediately. You'll keep the momentum. Three good attempts and I'll consider your overtaking legal."}}};
  default:return {{7,{"TEST MIKROFONU. Proszę dostroić trzy urządzenia boczne. Po zakończeniu testu cisza zostanie przywrócona do normy.","MICROPHONE TEST. Please tune three auxiliary units. Upon completion, silence will return to regulation levels."}}};
  }
}
} // namespace iceberg

namespace iceberg {
inline std::vector<DialogueLine> hidden_testimony(int id) {
  if(id==0)return {
    {0,{"Na kartce jest dziecięcy plan mieszkania. Kuchnia, pokój mamy, schowek. Za ścianą schowka dorysowano jeszcze jeden pokój. Bez drzwi.","The paper shows a child's apartment plan. Kitchen, Mum's room, cupboard. Another room is drawn behind the cupboard wall. No door."}},
    {0,{"Na odwrocie: Jeśli ogłoszą próbę syreny, nie zamykaj okna. Babcia nie słyszy. Numer mieszkania poprawiano tak długo, aż papier pękł.","On the back: If they announce a siren test, don't close the window. Grandma can't hear. The apartment number was corrected until the paper tore."}},
    {0,{"Podpis konserwatora: Głośnik szkoleniowy, górny pomost. Sygnał mieszkańców nadal wraca tą linią. Nie kasować przed odczytem.","Maintenance note: Training loudspeaker, upper landing. The residents' signal still returns along that line. Do not erase before reading."}}
  };
  if(id==1)return {
    {7,{"Próba komunikatu. Mieszkańcy klatki jedenastej proszeni są o pozostanie w mieszkaniach. To tylko próba. To tylko próba.","Announcement test. Residents of stairwell eleven are asked to remain in their apartments. This is only a test. This is only a test."}},
    {7,{"Odpowiedzi: czterdzieści jeden. Potwierdzenia odbioru: czterdzieści. Oczekuję na potwierdzenie. Oczekuję na potwierdzenie.","Replies: forty-one. Acknowledgements: forty. Awaiting acknowledgement. Awaiting acknowledgement."}},
    {7,{"Błąd spisu. Mieszkańców: zero. Odpowiedzi: czterdzieści jeden. Nie umiem odjąć odpowiedzi od mieszkańców.","Register error. Residents: zero. Replies: forty-one. I cannot subtract replies from residents."}},
    {7,{"Linia kończy się nad kołem wyrównawczym. Proszę sprawdzić górną platformę przepompowni. Nie rozłączać mnie, zanim ktoś odbierze.","The line ends above the equalization wheel. Please check the pumping station's upper platform. Do not disconnect me before someone answers."}}
  };
  return {
    {4,{"Protokół mówi, że zamknięcie miało potrwać siedem minut. Pierwszy podpis jest stanowczy. Drugi drży. Pod trzecim zostało tylko wgłębienie długopisu.","The log says the closure was meant to last seven minutes. The first signature is firm. The second shakes. Of the third, only the pen's impression remains."}},
    {4,{"Mieszkańcy odpowiadali przez czujniki drgań w rurach. System zapisywał ich jako zakłócenia. Potem ktoś zmienił kategorię: odgłosy eksploatacyjne.","Residents replied through vibration sensors in the pipes. The system logged them as interference. Then someone changed the category: operational noise."}},
    {7,{"Ostatnia odpowiedź nie przyszła z zamkniętej klatki. Przyszła stąd. Siedem minut po zabraniu ostatniego pracownika.","The last reply did not come from the sealed stairwell. It came from here. Seven minutes after the last worker was taken out."}},
    {4,{"Na wewnętrznej stronie drzwi ktoś wyrył: Nie trzeba otwierać. Trzeba pamiętać. Zabierasz kopię protokołu. Oryginał zostaje.","Someone scratched the inside of the door: You don't have to open it. You have to remember. You take a copy of the log. The original stays."}}
  };
}
}

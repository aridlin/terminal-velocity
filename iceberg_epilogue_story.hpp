#pragma once
namespace iceberg {
inline Text crown_sign() {return {
 "Wiatr niesie między podporami. Zryw przebija podmuch; możesz też poczekać na osłabienie wiatru. Prawa korba otwiera osłonę grani i wyrównuje ciśnienie. Skocz z wózka, kiedy jedzie w górę.",
 "Wind carries between the supports. A dash cuts through a gust; you can also wait for the wind to ease. The right crank opens the ridge screen and balances the pressure. Jump from a cradle while it rises."};}

inline std::vector<DialogueLine> epilogue_arrival() {return {
  {0,{"Przespałeś cały kurs powrotny. Bogdan omijał hamowanie przy twoim wagonie. Twierdzi, że z powodów technicznych.","You slept through the entire return journey. Bogdan avoided braking near your carriage. He says it was for technical reasons."}},
  {0,{"Jest rano. Zanieś odpis spisu do nowego okienka, a nasiona do skrzynki przy przystanku. Potem naprawdę możesz odpocząć.","It's morning. Take the register copy to the new counter, and the seeds to the box by the stop. Then you really can rest."}}
};}
inline DialogueLine epilogue_resident(int npc) {
 static const Text words[]={
  {"Dostałam potwierdzenia z każdej dzielnicy. Nie mamy już jednej jedynej kopii, której ktoś może zapomnieć wyjąć z szuflady.","Every district confirmed delivery. We no longer have just one copy that someone can forget to take out of a drawer."},
  {"Odciąłem stary zawór od pulpitu. Teraz żeby zamknąć cały obieg, trzeba zejść do mieszkańców i powiedzieć im dlaczego. Klucz wisi tutaj.","I disconnected the old valve from the control desk. To close the whole circuit now, you have to go down to the residents and explain why. The key hangs here."},
  {"Dodałem przystanek do rozkładu. Ten, którego niby nie było. Dziś wysiadły tam trzy osoby. Nie będę udawał, że zawsze o nim pamiętałem.","I added the missing stop to the timetable. Three people got off there today. I won't pretend I always remembered it."},
  {"Podpisałam sprostowanie swoim nazwiskiem. W poprzednim spisie też jest mój podpis. Tego nie usunę. Sprostowanie leży obok, żeby można było porównać.","I signed the correction with my own name. My signature is on the earlier register too. I won't remove it. The correction is beside it, so people can compare."},
  {"Zamknięcie miało trwać siedem minut. Uratowało dolne piętra, ale nie usprawiedliwia tego, co zrobiono później. W czytelni są teraz rozkazy, zeznania i nazwiska. Razem.","The closure was meant to last seven minutes. It saved the lower levels, but that doesn't excuse what happened afterwards. The reading room now holds the orders, the testimony and the names. Together."},
  {"Zostawiłam ziemię przy przystanku. Tu jest chłodniej niż w ogrodzie. Witek obiecał rurę pod skrzynką, a dzieci już podzieliły się podlewaniem.","I left soil beside the stop. It's colder here than in the garden. Witek promised a pipe beneath the box, and the children have already shared out the watering."},
  {"Dziś poszedłem pieszo. Przegrałem z tramwajem, z tobą i z panią niosącą zakupy. Chyba dobrze mi zrobi taki jeden dzień.","I walked today. I lost to the tram, to you, and to a lady carrying shopping. I think one day like this will do me good."},
  {"KOMUNIKAT BIEŻĄCY. Lista odbiorców została uzupełniona. Dawny komunikat zachowano w archiwum. Sprostowanie będzie odczytywane tak długo, jak będzie potrzebne.","CURRENT ANNOUNCEMENT. The recipient list has been restored. The former announcement remains in the archive. The correction will be read for as long as it is needed."}
 };
 return {npc,words[std::clamp(npc,0,7)]};
}
inline std::vector<DialogueLine> epilogue_action(int action) {
 if(action==0)return {{5,{"Nie wszystkie wykiełkują. Zostaw między nimi miejsce. Tak, właśnie tyle. Resztą zajmiemy się jutro.","They won't all sprout. Leave space between them. Yes, about that much. We'll take care of the rest tomorrow."}}};
 return {{4,{"Odpis przyjęty. Postawimy go tutaj, przy rozkładzie, nie za ladą. Każdy może sprawdzić własny adres.","Copy received. We'll put it here, beside the timetable, not behind the counter. Everyone can check their own address."}},
  {3,{"I zgłosić brakujące nazwisko. Bez opłaty. Dopisałam to dużymi literami.","And report a missing name. No charge. I wrote that in large letters."}}};
}
inline std::vector<DialogueLine> journey_end() {return {
 {0,{"Na dzisiaj wszystko. Zostaw torbę przy piecu. Herbatę już nalałam.","That's everything for today. Leave your bag by the stove. I've poured the tea."}},
 {-1,{"Za oknem przejeżdża pierwszy zwykły tramwaj. Nikt nie sprawdza, czy jest ostatni. W skrzynce przy przystanku zostają trzy małe zagłębienia w ziemi.","The first ordinary tram passes the window. Nobody checks whether it's the last. Three small hollows remain in the soil beside the stop."}},
 {-1,{"KONIEC PODRÓŻY. Możesz zostać w przystani, odwiedzać dzielnice z mapy, dokończyć sprawy mieszkańców albo wyruszyć na kolejne wyprawy.","JOURNEY COMPLETE. You can stay in the harbor, visit districts from the map, finish the residents' tasks, or set out on further expeditions."}}
};}
}

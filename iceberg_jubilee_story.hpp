#pragma once
namespace iceberg {
inline std::vector<DialogueLine> jubilee_arrival() {
  return {
    {1,{"To jest pompa imienia Jana Pawła II. Miała grzać trzy klatki. Potem rada zamówiła pomnik. Potem fontannę. Na rurę do mieszkań już nie starczyło.","This is the John Paul II memorial pump. It was meant to heat three stairwells. Then the board ordered a monument. Then a fountain. There was nothing left for the pipe to the homes."}},
    {1,{"Pompa od ośmiu lat grzeje własny cokół. Sprawność układu: sto procent. Wszystko wraca do papieża.","For eight years the pump has heated its own pedestal. System efficiency: one hundred percent. Everything returns to the Pope."}},
    {7,{"OBIEKT POSIADA MOC 21,37 MEGAWATA. ZMIANA PRZEZNACZENIA CIEPŁA WYMAGA ZGODY KOMITETU. KOMITET ULEGŁ ZAMARZNIĘCIU.","RATED OUTPUT: 21.37 MEGAWATTS. REDIRECTING THE HEAT REQUIRES COMMITTEE APPROVAL. THE COMMITTEE HAS FROZEN OVER."}},
    {1,{"Najpierw wyłącz wodotrysk na dole. Potem obejdź pomnik i przestaw pompę. Zawór do mieszkań jest nad nią. Nie ruszaj tabliczki. To jedyne, co tu odebrano technicznie.","Shut off the fountain below. Go around the monument and reroute the pump. The residential valve is above it. Leave the plaque alone. It's the only part that passed inspection."}}
  };
}
inline std::vector<DialogueLine> jubilee_dedication() {
  return {
    {-1,{"POMPA IM. ŚW. JANA PAWŁA II. Dar mieszkańców dla mieszkańców. Moc: 21,37 MW. Odbiorcy: fontanna, ogrzewanie pomnika, podświetlenie tablicy darczyńców.","THE SAINT JOHN PAUL II PUMP. A gift from the residents to the residents. Output: 21.37 MW. Recipients: fountain, monument heating, donor plaque lighting."}},
    {-1,{"Na odwrocie faktury Witek dopisał: papież nie potrzebuje ogrzewania podłogowego, a pani z jedenastki potrzebuje. Przykręcono drugą tablicę, żeby zasłonić dopisek.","On the invoice's reverse, Witek wrote: the Pope doesn't need underfloor heating; the lady in number eleven does. A second plaque was bolted on to cover the note."}}
  };
}
inline std::vector<DialogueLine> jubilee_complete() {
  return {
    {7,{"PRZEPŁYW DO MIESZKAŃ: 21,37. PRZEPŁYW DO POMNIKA: ZERO. WYKRYTO NIEZATWIERDZONY CUD.","FLOW TO HOMES: 21.37. FLOW TO MONUMENT: ZERO. UNAUTHORISED MIRACLE DETECTED."}},
    {1,{"Słyszysz te rury? Trzy klatki mają ciepło. Pomnik poczeka do wiosny. Jak ktoś zapyta, powiedz, że oszczędzamy na kanonizację drugiej pompy.","Hear those pipes? Three stairwells have heat. The monument can wait until spring. If anyone asks, say we're saving up to canonise the second pump."}},
    {1,{"Weź jasną kurtkę z szafki. Od lat trzymali ją na uroczyste otwarcie. Wreszcie się przydała.","Take the pale jacket from the locker. They've kept it for the grand opening for years. At last it has a use."}}
  };
}
}

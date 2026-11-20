#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace iceberg {
inline constexpr unsigned ContentVersion = 4;
inline constexpr int DistrictCount = 6;
// Content registry boundary: increment when another district is fully authored.
inline constexpr int AuthoredDistrictCount = 6;
inline constexpr int RoomsPerLeg = 8;
inline constexpr int UnnumberedFloor=3, NightPlatform=4;
inline constexpr int NightPlatformTemplate=200+NightPlatform;
inline constexpr double PlatformGlassLeft=18., PlatformGlassRight=59.;
inline constexpr double PlatformSignalX=73., PlatformShutterX=61.;
inline constexpr double PlatformShutterWait=3.4, SeedPodBurstTime=1.7;
inline constexpr uint64_t BasementCircuitCut=1;
inline constexpr double NightPlateLeft=99., NightPlateRight=103., NightPlateTime=2.1;
inline constexpr double GardenSnailWalkTime=4.;
enum class SideStory { HarborStub, EstatePlate, ArchiveCopy, Casting, Intercom, Tape, Roster, EmergencyBell, HeatworksMug, GardensPot, CrownPunch, RosterDesk, BasementBreaker, GardenSnailTap, GardenSnailShelter, HeatworksMugBench, ArchiveLine, NightWindow, GardenButton, EstateBirdhouse, HarborTicket, NightWeighbridge, CarbonDial, CarbonReader, HarborWhistle, HeatGauge, GardenSeedDrawer, CrownViewer, PlatformBoard, PlatformBench, PlatformSignal, PlatformShutter, ArchiveLostProperty, GardenSeedPod, EstateDoormat };
// Side-story state uses the top of the second task word. Existing room tasks
// use their original indices and retain the same save representation.
enum class SideFlag : uint64_t {
  LastCoatFound=1ull<<45, LastCoatFiled=1ull<<46,
  PlatformWaited=1ull<<47, PlatformCleared=1ull<<48, ArchiveCarbonPrinted=1ull<<49,
  HeatworksMugTaken=1ull<<50, HeatworksMugReturned=1ull<<51,
  GardenSnailFound=1ull<<52, GardenSnailWatered=1ull<<53,
  RosterPosted=1ull<<54, RosterSealed=1ull<<55,
  HarborStub=1ull<<56, EstatePlate=1ull<<57, ArchiveCopy=1ull<<58, Casting=1ull<<59,
  CastingReturned=1ull<<60, BellRung=1ull<<61, Roster=1ull<<62, BasementPowerCut=1ull<<63
};
inline constexpr uint64_t side_flag(SideFlag flag) {return static_cast<uint64_t>(flag);}
enum class Curio : uint64_t { BasementCall=1ull<<24, GardenButton=1ull<<25,
  GardenButtonReturned=1ull<<26, ArchiveCall=1ull<<27, CrownTicket=1ull<<28, HarborTicket=1ull<<29,
  NightWindowSeen=1ull<<30, NightPlateRecorded=1ull<<31 };
inline constexpr uint64_t curio_bit(Curio curio) {return static_cast<uint64_t>(curio);}
enum class Language { Polish, English };
enum class Mode { Campaign, Endless, Trial };
enum class RoomRole { Traverse, Lesson, Finale, Settlement, Secret, Interlude };
enum class Rhythm { Flow, Precision, Vertical, Timing, Explore, Recovery };
enum class Mechanic { None, Current, Steam, Conveyor, Echo, Growth, Wind };
enum class InspectionArt { Parcel, Valve, Signal, Address, File, Seeds, Speaker, Thermometer, Sign, Dedication, Board, Door, Coin, Refill, Lamp, Character, Delivery, Chart, Traversal, Achievement };
enum class TraversalStudy { Sling, Piston, Turbine, Vent, Balcony, Cornice, Rack, Scanner, Vine, Flytrap, Ice, Wind, Count };
struct Text {
  const char *pl = "";
  const char *en = "";
  std::string get(Language language) const {
    return language == Language::English && en && *en ? en : pl;
  }
};
inline constexpr std::array<Text,3> CarbonDialLabels{{
  {"Bęben czasu","Time drum"},{"Bęben obciążenia","Load drum"},{"Bęben strony","Side drum"}
}};
inline constexpr std::array<std::array<Text,3>,3> CarbonDialReadings{{
  {{{"00:00","00:00"},{"00:07","00:07"},{"06:00","06:00"}}},
  {{{"0","0"},{"1","1"},{"2","2"}}},
  {{{"TAMTA","FAR"},{"TA","THIS"},{"OBIE","BOTH"}}}
}};
inline constexpr std::array<unsigned,3> CarbonReaderCode{1,2,1};
inline constexpr Text CarbonReaderRecord{
  "00:07 · Klatka 11. KURIER: wyjście potwierdzone. DRUGI: wciąż po tej stronie. Kopia miała trafić do pieca. Na odwrocie dopisek Ireny: «Ktoś musi pamiętać, że czekamy». Na planie: wejście za szafą 11, pod czytnikiem.",
  "00:07 · Stairwell 11. COURIER: exit confirmed. SECOND: still on this side. This copy was meant for the furnace. Irena wrote on the back, ‘Someone has to remember we’re still waiting.’ The plan marks an entrance behind cabinet 11, below the reader."};
struct District {
  Text name, subtitle, objective;
  uint32_t stone, light, shadow, accent;
  Mechanic mechanic;
};
inline const std::array<District, DistrictCount> Districts{{
  {{"DOLNA PRZYSTAŃ", "LOWER HARBOR"},
   {"Pod oceanem też jest poniedziałek.", "It is Monday below the ocean, too."},
   {"Połącz odcięte dzielnice z siecią ciepła.", "Reconnect the isolated districts to the heat network."},
   0x345866, 0x9dd4dd, 0x142b36, 0xe4ad68, Mechanic::Current},
  {{"CIEPŁOWNIA ŁAZARZ", "LAZARUS HEATING WORKS"},
   {"Ciepło wspólne. Odpowiedzialność indywidualna.", "Shared heating. Individual responsibility."},
   {"Uruchom obieg awaryjny bez stopienia fundamentów.", "Restore the emergency loop without melting the foundations."},
   0x675143, 0xf0c990, 0x2e252b, 0xe99462, Mechanic::Steam},
  {{"OSIEDLE PARAPET", "WINDOWSILL ESTATE"},
   {"Widok na przyszłość. Winda nieczynna.", "A view of the future. Lift out of order."},
   {"Przywróć komunikację między zawieszonymi ulicami.", "Reconnect the suspended streets."},
   0x66566e, 0xd2bedc, 0x292636, 0xe5bc66, Mechanic::Conveyor},
  {{"ARCHIWUM CISZY", "ARCHIVE OF SILENCE"},
   {"Nie znaleziono mieszkańca. Proszę nie powtarzać.", "Resident not found. Please do not repeat."},
   {"Odszukaj oryginalny plan sieci i brakujące nazwiska.", "Find the original network plans and the missing names."},
   0x526978, 0xc7e4ed, 0x1a283d, 0x9fb6f6, Mechanic::Echo},
  {{"OGRODY PODSZRONU", "UNDERFROST GARDENS"},
   {"Słońce zastępcze. Pomidory prawdziwe.", "Artificial sun. Real tomatoes."},
   {"Poprowadź ciepło przez żywe kanały ogrodu.", "Route the heat through the garden's living channels."},
   0x45665b, 0xb8dfb7, 0x182e2c, 0xd7cb79, Mechanic::Growth},
  {{"KORONA", "THE CROWN"},
   {"Dalej nie ma już sufitu.", "There is no ceiling beyond this point."},
   {"Otwórz latarnię i połącz wszystkie dzielnice.", "Open the beacon and unite the districts."},
   0x6c7b8d, 0xf0edf0, 0x263347, 0xf3ce82, Mechanic::Wind}
}};

inline Text route_title(int district,int stage) {
  int leg=stage<=3?0:stage<=5?1:2;
  static const Text routes[6][3]={
    {{"Kanał odpływowy","Drainage channel"},{"Pomosty przepompowni","Pumping station boardwalks"},{"Rurociąg do Łazarza","Pipeline to Lazarus"}},
    {{"Dopływ pary","Steam supply"},{"Obieg mieszkań","Residential circuit"},{"Powrót awaryjny","Emergency return"}},
    {{"Ulica na linach","The street on cables"},{"Klatki bez numerów","Unnumbered stairwells"},{"Pętla archiwalna","The archive loop"}},
    {{"Czytelnia pod szkłem","The glass-roofed reading room"},{"Magazyn odpisów","The copies repository"},{"Szafa oryginałów","The original-plan cabinet"}},
    {{"Kanał korzeniowy","The root channel"},{"Górne szklarnie","The upper greenhouses"},{"Żywy obieg","The living circuit"}},
    {{"Droga ponad dachy","Above the rooftops"},{"Pęknięty grzbiet","The fractured ridge"},{"Podejście do latarni","The beacon approach"}}
  };
  if(stage==3)leg=1;
  if(stage==5)leg=2;
  return routes[std::clamp(district,0,DistrictCount-1)][leg];
}
struct Record {
  double best_seconds = 0;
  int best_deaths = -1;
  int best_treasures = 0;
  bool complete = false;
};
struct SeedRecord {uint32_t seed=1;int district=0;unsigned version=ContentVersion;Record record;};
struct Expedition {
  unsigned layout_version = ContentVersion;
  bool active = false, at_exit = false;
  uint32_t seed = 1, origin_seed = 1;
  int district = 0, room = 0, depth = 0, checkpoint = 0;
  uint64_t collected = 0, switches = 0;
  double room_seconds = 0, seconds = 0;
  int room_deaths = 0, deaths = 0, treasures = 0;
  std::vector<int> itinerary;
  std::vector<std::array<int,RoomsPerLeg>> completed_legs;
};
struct Revisit {
  bool active=false;
  int district=0, scene=0, checkpoint=0, deaths=0;
  uint64_t collected=0,switches=0;
  double seconds=0;
};
struct Profile {
  Language language = Language::Polish;
  bool started = false, finished = false, at_content_boundary = false, interlude = false;
  uint32_t seed = 1;
  unsigned content_version = ContentVersion;
  unsigned layout_version = ContentVersion;
  int district = 0, stage = 0, room = 0;
  int unlocked = 0;
  uint64_t story = 0;
  std::array<unsigned, 8> quests{};
  std::array<unsigned, 8> quest_counts{};
  std::array<uint64_t, 2> task_items{};
  uint32_t discoveries = 0, cosmetics = 0;
  int outfit = 0, quest_focus = -1;
  int hidden_room = -1, visit_district = -1;
  double campaign_seconds = 0;
  int campaign_deaths = 0;
  double district_seconds = 0;
  int district_deaths = 0, district_treasures = 0;
  std::array<Record, DistrictCount> records{};
  std::array<Record, DistrictCount> trials{};
  int endless_best = 0;
  std::array<Expedition, 2> expeditions;
  Revisit revisit;
  // Exact visited room cells; never infer a visit from a shortcut or a secret.
  std::array<uint32_t, DistrictCount> route_visited{};
  std::array<unsigned, DistrictCount> route_choices{}, route_decisions{};
  std::vector<SeedRecord> challenges;
  // Persist the assembled itinerary as well as its seed: content additions
  // cannot silently reroll an expedition that somebody has already started.
  std::vector<int> itinerary;
  int checkpoint = 0;
  uint64_t collected = 0, switches = 0;
  double room_seconds = 0;
  int room_deaths = 0;
  float music_volume = .65f, effects_volume = .75f, ambience_volume = .55f;
  bool pause_on_focus_loss = true;
  bool smooth_camera = false;
};

inline constexpr int MasteryTokenFare = 12;
// The save already stores permanent unlocks in this field.
inline constexpr uint32_t MasteryTransitPass = 1u << 24;

inline constexpr int SurveyTokenFare = 24;
inline constexpr uint32_t SurveyTransitPass = 1u << 25;
inline constexpr uint32_t SurveyCourseComplete = 1u << 26;
inline constexpr int SurveyReplay = 3;
inline constexpr int BreakwaterSurvey = 321;

inline int transit_token_progress(const Profile &profile,int fare) {
  int campaign=0;
  for(int i=0;i<DistrictCount;++i) {
    const int historical=std::min(fare,profile.records[size_t(i)].best_treasures);
    const int current=i==profile.district?std::min(fare,profile.district_treasures):0;
    campaign+=std::max(historical,current);
  }
  return std::clamp(std::max(campaign,profile.expeditions[0].treasures),0,fare);
}

// Bits 8–15 carry the best earned balance, and bits 27–29 carry shop purchases.
inline constexpr unsigned OutfitterEarnedShift=8;
inline constexpr uint32_t OutfitterEarnedMask=255u<<OutfitterEarnedShift;
inline constexpr uint32_t HarborScarf=1u<<27, DockBoots=1u<<28, SignalSatchel=1u<<29;
struct OutfitterItem { Text name,effect; int price; uint32_t flag; };
inline constexpr std::array OutfitterItems{
  OutfitterItem{{"Szalik portowy","Harbor scarf"},{"Wygląd","Appearance"},18,HarborScarf},
  OutfitterItem{{"Buty dokerskie","Dock boots"},{"Lepsza przyczepność","Better grip on ice"},30,DockBoots},
  OutfitterItem{{"Torba sygnałowa","Signal satchel"},{"Większy zasięg ogniw","Wider dash-cell reach"},42,SignalSatchel}
};
inline int outfitter_earned(const Profile &profile) {
  return std::max(int((profile.cosmetics&OutfitterEarnedMask)>>OutfitterEarnedShift),
    transit_token_progress(profile,255));
}
inline int outfitter_balance(const Profile &profile) {
  int balance=outfitter_earned(profile);
  for(const auto &item:OutfitterItems)if(profile.cosmetics&item.flag)balance-=item.price;
  return std::max(0,balance);
}
inline void remember_outfitter_tokens(Profile &profile) {
  profile.cosmetics=(profile.cosmetics&~OutfitterEarnedMask)|
    (uint32_t(outfitter_earned(profile))<<OutfitterEarnedShift);
}

inline int mastery_token_progress(const Profile &profile) {
  return transit_token_progress(profile,MasteryTokenFare);
}
inline bool survey_pass(const Profile &profile) {
  return (profile.cosmetics&SurveyTransitPass)||transit_token_progress(profile,SurveyTokenFare)==SurveyTokenFare;
}

enum class Achievement {
  FirstToken, DashJump, Gear, Survey, Journey, FirstDoor, DockChat, FirstDistrict,
  FirstFall, RoughRoom, MadeItAnyway, CleanDistrict, Mastery, CleanMastery, CrownClean,
  WallBounce, FerryRide, VineRide, BalconyRide, SaltBath, AllGear, FirstTestimony,
  ThreeTestimonies, BeltReverse, SiliconeReturn, LostFloor, WitnessTape, BelowTheRoster, RingAgain,
  NextShift, CrownReached, TrailLeg, LedgeClimb, MailBag, CrownDown, CleanSurvey, CleanLeg, FullRoom, AllMail, MasonrySpring, Detour, CircuitSaved, RoughDistrict, CrownRecovery, LongWayBack, MugReturned, SnailSafe, PunchTomorrow, PowerCut, RosterChoice, PaperTrail, NightLine, LastPassenger, WrenHouse, LostReceipt, NightWeight, SecondImpression,
  AirRefill, HundredTokens, CleanJourney, QuietLeg, EmptyDelivery, HardLesson, HarborWhistle, HeatGauge,
  GardenSeedDrawer, CrownViewer, PlatformFound, PlatformEcho, PlatformReleased,
  CargoLift, PressureLift, LeafFlume, ArchiveRide, CoastClear, LastCoat, HeldForCollection, DashingHeadfirst, SeedShower, Count
};
enum class AwardGroup { All, Journey, Skill, Mishaps, Oddities, Secrets };
struct AchievementInfo { Text title,condition; AwardGroup group; const char *icon; bool hidden=false,rare=false; };
inline constexpr std::array Achievements{
  AchievementInfo{{"Pierwszy żeton","First token"},{"Zbierz żeton.","Collect a token."},AwardGroup::Journey,"◉"},
  AchievementInfo{{"Zryw w górę","Dash and lift"},{"Wykonaj skok ze zrywu.","Perform a dash jump."},AwardGroup::Skill,"↗"},
  AchievementInfo{{"Na miarę","Fitted out"},{"Kup wyposażenie.","Buy a piece of gear."},AwardGroup::Journey,"▣"},
  AchievementInfo{{"Do latarni","To the breakwater"},{"Ukończ objazd latarni.","Finish Breakwater Survey."},AwardGroup::Secrets,"♖"},
  AchievementInfo{{"Ponad Koroną","Beyond the Crown"},{"Ukończ podróż.","Complete the Journey."},AwardGroup::Journey,"▲"},
  AchievementInfo{{"Pierwszy próg","First threshold"},{"Opuść nabrzeże i wejdź na trasę.","Leave the quay and enter the route."},AwardGroup::Journey,"▷"},
  AchievementInfo{{"Na zmianie","On the shift"},{"Porozmawiaj z dokerem.","Talk to a dockworker."},AwardGroup::Journey,"◌"},
  AchievementInfo{{"Dalej w głąb","Further in"},{"Ukończ pierwszą dzielnicę.","Finish your first district."},AwardGroup::Journey,"▤"},
  AchievementInfo{{"Pierwsze potknięcie","First setback"},{"Wróć do lampy po upadku lub trafieniu.","Return to a lamp after a fall or hit."},AwardGroup::Mishaps,"↓"},
  AchievementInfo{{"Znajoma lampa","Familiar lamp"},{"Daj się złapać przeszkodzie w pokoju, w którym masz już pięć powrotów.","Get caught by a hazard in a room with at least five returns."},AwardGroup::Mishaps,"↺"},
  AchievementInfo{{"I tak poszło","Made it anyway"},{"Ukończ pokój po co najmniej pięciu powrotach.","Finish a room after at least five returns."},AwardGroup::Mishaps,"↰"},
  AchievementInfo{{"Bez potknięcia","Clean district"},{"Ukończ dzielnicę bez powrotów.","Finish a district without a retry."},AwardGroup::Skill,"✓"},
  AchievementInfo{{"Z pieczęcią","Stamped and filed"},{"Ukończ próbę mistrzowską.","Complete a Mastery Trial."},AwardGroup::Skill,"◆"},
  AchievementInfo{{"Bez poprawek","No corrections"},{"Ukończ próbę bez powrotów.","Complete a Mastery Trial without a retry."},AwardGroup::Skill,"✦",false,true},
  AchievementInfo{{"W nienaruszonym stanie","Pristine delivery"},{"Pokonaj Strażnika Korony bez powrotu.","Defeat the Crown Sentinel without a retry."},AwardGroup::Skill,"♜",false,true},
  AchievementInfo{{"Rykoszet","Ricochet"},{"Odbij się od ściany ze zrywu.","Perform a dash wall bounce."},AwardGroup::Skill,"↱"},
  AchievementInfo{{"Na pokładzie","All aboard"},{"Wejdź na prom w Dolnym Porcie.","Board a Lower Harbor ferry."},AwardGroup::Journey,"≋"},
  AchievementInfo{{"Zielona linia","The green line"},{"Chwyć się pnącza w Ogrodach.","Catch a swing vine in the Gardens."},AwardGroup::Journey,"❧"},
  AchievementInfo{{"Winda pod drzwi","Doorstep service"},{"Podjedź na ruchomym balkonie w Osiedlu.","Ride a cable balcony in the Estate."},AwardGroup::Journey,"↕"},
  AchievementInfo{{"Pranie w porcie","Port laundry"},{"Zrób plusk w Dolnym Porcie.","Make a splash in Lower Harbor."},AwardGroup::Oddities,"≈"},
  AchievementInfo{{"Pełny ekwipunek","Fully kitted"},{"Kup wszystkie trzy rzeczy od handlarza.","Buy all three outfitter items."},AwardGroup::Skill,"▦"},
  AchievementInfo{{"Poza protokołem","Off the record"},{"Odszukaj pierwsze ukryte świadectwo.","Find your first hidden testimony."},AwardGroup::Secrets,"▧",true},
  AchievementInfo{{"Trzy wersje","Three accounts"},{"Odkryj wszystkie trzy ukryte świadectwa.","Find all three hidden testimonies."},AwardGroup::Secrets,"▥",true,true},
  AchievementInfo{{"W drugą stronę","Special delivery in reverse"},{"Odwróć środkową taśmę w Archiwum.","Reverse the middle conveyor in the Archive."},AwardGroup::Oddities,"↔"},
  AchievementInfo{{"Nie do tego formularza","Wrong form entirely"},{"Zanieś Grażynie osobliwy odlew.","Bring the peculiar casting to Grażyna."},AwardGroup::Oddities,"▨",true},
  AchievementInfo{{"Piętro bez numeru","The unnumbered floor"},{"Otwórz przejście pod budynkiem.","Open the passage below the building."},AwardGroup::Secrets,"▽",true},
  AchievementInfo{{"Poczekaj na trzeci głos","Wait for the third voice"},{"Odsłuchaj całą ukrytą taśmę.","Listen to the full hidden tape."},AwardGroup::Secrets,"◍",true},
  AchievementInfo{{"Pod spisem","Below the roster"},{"Wróć z podziemia z kopią listy.","Return from below with a copy of the roster."},AwardGroup::Secrets,"▤",true},
  AchievementInfo{{"Chyba nie słyszą","Maybe they didn't hear"},{ "Zadzwoń ponownie po usłyszeniu komunikatu dzwonka alarmowego.","Ring the emergency bell again after hearing its announcement."},AwardGroup::Oddities,"♪",true},
  AchievementInfo{{"Na następnej zmianie","Next shift"},{"Dotrzyj do Ciepłowni Łazarz.","Reach Lazarus Heating Works."},AwardGroup::Journey,"↻"},
  AchievementInfo{{"Pod Koroną","Under the Crown"},{"Dotrzyj do dzielnicy na Koronie.","Reach the Crown district."},AwardGroup::Journey,"▲"},
  AchievementInfo{{"Pierwszy odcinek","First leg"},{"Ukończ odcinek Wyprawy bez końca.","Finish a leg of Endless Expedition."},AwardGroup::Journey,"▸"},
  AchievementInfo{{"Tuż nad krawędzią","Over the edge"},{"Wciągnij się na krawędź.","Pull yourself up onto a ledge."},AwardGroup::Journey,"┐"},
  AchievementInfo{{"Poczta działa","The post goes through"},{"Doręcz worek listów w Dolnej Przystani.","Deliver a Lower Harbor mailbag."},AwardGroup::Journey,"▱"},
  AchievementInfo{{"Zmiana warty","Changing of the guard"},{"Pokonaj Strażnika Korony.","Defeat the Crown Sentinel."},AwardGroup::Journey,"♜"},
  AchievementInfo{{"Suchy powrót","Back dry"},{"Ukończ objazd latarni bez powrotu.","Finish the Breakwater Survey without a retry."},AwardGroup::Skill,"◎",false,true},
  AchievementInfo{{"Czysty odcinek","Clean leg"},{"Ukończ pierwszy odcinek wyprawy bez powrotu.","Finish the first leg of an expedition without a retry."},AwardGroup::Skill,"✓"},
  AchievementInfo{{"Nic do oclenia","Nothing to declare"},{"Zbierz wszystkie żetony w pokoju z co najmniej trzema i ukończ go bez powrotu.","Collect every token in a room with at least three and finish it without a retry."},AwardGroup::Skill,"◉"},
  AchievementInfo{{"Ostatni worek","Last post"},{"Doręcz wszystkie trzy portowe worki listów.","Deliver all three Lower Harbor mailbags."},AwardGroup::Skill,"▱",false,true},
  AchievementInfo{{"Markiza wytrzyma","The awning held"},{"Odbij się od markizy na Osiedlu.","Bounce off an Estate awning."},AwardGroup::Oddities,"↥"},
  AchievementInfo{{"Droga okrężna","Taking the long way"},{"Wybierz odnogę trasy na rozwidleniu.","Choose the alternate path at a route fork."},AwardGroup::Secrets,"↪"},
  AchievementInfo{{"Z morza do domów","From sea to homes"},{"Źle skieruj wodę w porcie, a potem zawróć ją do mieszkań.","Correct the Harbor pump after sending the heat out to sea."},AwardGroup::Mishaps,"↯"},
  AchievementInfo{{"Jednak na miejscu","Still made it"},{"Ukończ jedną dzielnicę mimo co najmniej dwudziestu powrotów.","Finish a district despite at least twenty retries."},AwardGroup::Mishaps,"↺"},
  AchievementInfo{{"Jeszcze jedna runda","One more round"},{"Pokonaj Strażnika Korony po co najmniej pięciu powrotach.","Defeat the Crown Sentinel after at least five retries."},AwardGroup::Mishaps,"♜"},
  AchievementInfo{{"Znów ta lampa","That lamp again"},{"Wróć do lampy po zagrożeniu w pokoju, w którym zginąłeś co najmniej dziesięć razy.","Get sent back by a hazard or fall after at least ten deaths in one room."},AwardGroup::Mishaps,"↶"},
  AchievementInfo{{"Zmiana przy kubku","Tea for the next shift"},{"Oddaj Witkowi kubek.","Return Witek’s mug."},AwardGroup::Oddities,"▢",true},
  AchievementInfo{{"Lokal pod funkią","Room under the hosta"},{"Znajdź nową kryjówkę ślimaka.","Find the snail in its new shelter."},AwardGroup::Oddities,"@",true},
  AchievementInfo{{"Bilet na jutro","Tomorrow’s ticket"},{"Sprawdź kasownik na Koronie.","Try the ticket punch on the Crown."},AwardGroup::Oddities,"▫",true},
  AchievementInfo{{"Światło awaryjne","Emergency lighting"},{"Odetnij dopływ prądu do piwnicy.","Cut power to the basement."},AwardGroup::Secrets,"⊘",true},
  AchievementInfo{{"Czternasta lokatorka","The fourteenth tenant"},{"Zdecyduj z Grażyną, co zrobić ze spisem.","Decide what to do with the roster alongside Grażyna."},AwardGroup::Secrets,"▤",true},
  AchievementInfo{{"Trzy odpisy","Paper trail"},{"Zbierz trzy rozproszone wskazówki o wejściu pod klatkę.","Collect all three scattered clues to the entrance under the stairwell."},AwardGroup::Secrets,"▧",true},
  AchievementInfo{{"Linia nocna","Night line"},{"Po rozmowie z portiernią odbierz połączenie z linii 11 w Archiwum.","After speaking to the basement intercom, answer Line 11 in the Archive."},AwardGroup::Secrets,"♪",true},
  AchievementInfo{{"Ostatni pasażer","Last passenger"},{"Zweryfikuj meldunek z linii 11 przy wyłączonej sieci.","Verify Line 11’s report with the circuit cut."},AwardGroup::Secrets,"◉",true,true},
  AchievementInfo{{"Dom za jeden guzik","A house for one button"},{"Zwróć z Ogrodów guzik do osiedlowego karmnika.","Return the button from the Gardens to the Estate birdhouse."},AwardGroup::Oddities,"❧",true},
  AchievementInfo{{"Między trzecią a czwartą","Between three and four"},{"Odbierz skasowany bilet w porcie.","Collect the punched ticket at the quay."},AwardGroup::Secrets,"▫",true},
  AchievementInfo{{"Ciężar własny","Tare weight"},{"Po spotkaniu przy szybie zważ się na wadze peronowej.","After the encounter at the window, step on the platform scale and let it settle."},AwardGroup::Secrets,"▥",true},
  AchievementInfo{{"Drugi odcisk","Second impression"},{"Odczytaj kalkę w Archiwum, korzystając z wydruku wagi.","Read the Archive carbon copy using the scale receipt."},AwardGroup::Secrets,"▧",true},
  AchievementInfo{{"Drugi oddech","Second wind"},{"Naładuj zryw ogniwem w powietrzu.","Recharge your dash at a refill cell while airborne."},AwardGroup::Journey,"✦"},
  AchievementInfo{{"Stówka w drobnych","A hundred in change"},{"Zarób 100 żetonów na wyposażenie.","Earn 100 outfitter tokens."},AwardGroup::Journey,"◉"},
  AchievementInfo{{"Kurier bez skreśleń","Courier in ink"},{"Zapisz ukończenie każdej dzielnicy bez żadnego powrotu.","Record a clean finish for each district."},AwardGroup::Skill,"✦",false,true},
  AchievementInfo{{"Lekki bagaż","Travelling light"},{"Ukończ próbę mistrzowską bez żetonów i bez powrotów.","Clear a Mastery Trial with no tokens and no retries."},AwardGroup::Skill,"◇",false,true},
  AchievementInfo{{"Pusty kurs","Empty delivery"},{"Po co najmniej pięciu powrotach przejdź pokój z co najmniej trzema żetonami, nie zabierając żadnego.","After five retries, clear a room with at least three tokens without collecting any."},AwardGroup::Mishaps,"▱"},
  AchievementInfo{{"Droga przez piekło","Learning the hard way"},{"Ukończ próbę mistrzowską mimo co najmniej 25 powrotów.","Finish a Mastery Trial after at least 25 retries."},AwardGroup::Mishaps,"↺"},
  AchievementInfo{{"Mgła potwierdza","Fog acknowledges"},{"Zagwiżdż przy szafce sygnałowej na nabrzeżu.","Blow the whistle at the quay signal box."},AwardGroup::Oddities,"♪",true},
  AchievementInfo{{"Filiżanka ciśnienia","One cup of pressure"},{"Sprawdź zapasowy manometr w Ciepłowni.","Test the spare pressure gauge at the Heating Works."},AwardGroup::Oddities,"◔"},
  AchievementInfo{{"Nasiona na wymianę","Seed exchange"},{"Otwórz szufladę z nasionami w Ogrodach.","Open the seed exchange drawer in the Gardens."},AwardGroup::Secrets,"❧",true},
  AchievementInfo{{"Widok na widok","The view of the view"},{"Skorzystaj z lunety na Koronie.","Look through the Crown telescope."},AwardGroup::Oddities,"⊙",true},
  AchievementInfo{{"Peron dla dwojga","Platform for two"},{"Wejdź na peron wpisany na kalce.","Enter the platform listed on the carbon copy."},AwardGroup::Secrets,"▰",true},
  AchievementInfo{{"Spóźnione odbicie","Late reflection"},{"Usiądź na ławce i poczekaj, aż szyba cię dogoni.","Sit at the bench and let the glass catch up."},AwardGroup::Secrets,"◈",true},
  AchievementInfo{{"Sygnał wolnej drogi","Clear signal"},{"Nadaj sygnał odjazdu ostatniemu pasażerowi.","Give the last passenger the departure signal."},AwardGroup::Secrets,"⚑",true,true},
  AchievementInfo{{"Ładunek osobowy","Human cargo"},{"Wjedź na podnośniku towarowym.","Ride a cargo hoist."},AwardGroup::Journey,"↥"},
  AchievementInfo{{"Na pełnym ciśnieniu","Under pressure"},{"Wjedź na tłoku ciśnieniowym.","Ride a pressure piston."},AwardGroup::Journey,"↟"},
  AchievementInfo{{"Liść polecony","Leaf by registered post"},{"Popłyń na liściu w rynnie Ogrodów.","Ride a leaf down a Gardens flume."},AwardGroup::Oddities,"❧"},
  AchievementInfo{{"Na górną półkę","Top shelf"},{"Podjedź na ruchomym regale w Archiwum.","Ride a moving Archive rack."},AwardGroup::Journey,"▥"},
  AchievementInfo{{"Od brzegu do brzegu","Coast clear"},{"Ukończ pierwsze dwa odcinki Wyprawy bez końca bez ani jednego powrotu.","Clear the first two Endless Expedition legs without a single retry."},AwardGroup::Skill,"≋",false,true},
  AchievementInfo{{"Płaszcz po ostatnim kursie","Coat after closing"},{"Poczekaj przy okienku na Peronie 11 po odjeździe ostatniego kursu i odbierz płaszcz.","Wait by the Platform 11 hatch after the last departure and collect the coat."},AwardGroup::Secrets,"▣",true},
  AchievementInfo{{"Do odbioru","Held for collection"},{"Zanieś płaszcz z peronu do punktu rzeczy znalezionych.","Take the coat from the platform to lost property."},AwardGroup::Secrets,"▦",true,true},
  AchievementInfo{{"Z rozpędu","Headfirst"},{"Traf na zagrożenie w trakcie zrywu.","Hit a hazard while dashing."},AwardGroup::Mishaps,"»"},
  AchievementInfo{{"Rozsiej wiadomość","Spread the word"},{"Potrząśnij suchym strąkiem w Ogrodach.","Shake the dry seed pod in the Gardens."},AwardGroup::Oddities,"✣",true}
};
static_assert(Achievements.size()==size_t(Achievement::Count));
inline constexpr unsigned LegacyAwardCount=29;
inline constexpr unsigned ExtendedAwardStart=unsigned(Achievement::AirRefill);
static_assert(ExtendedAwardStart-LegacyAwardCount<=32);
static_assert(Achievements.size()-ExtendedAwardStart<=32);
using AwardProgress=std::array<uint64_t,2>;

inline constexpr size_t achievement_word(Achievement id) {
  return unsigned(id)>=ExtendedAwardStart?1:0;
}
// The lower word preserves the original TVICE6 discovery bits. New awards
// occupy the upper half of the first task word, above task slots 0–23 and curios.
inline constexpr uint64_t achievement_bit(Achievement id) {
  // Bits 0–2 hold the original hidden testimonies. Keep all five released award bits.
  const unsigned index=unsigned(id);
  if(achievement_word(id))return uint64_t(1)<<(index-ExtendedAwardStart);
  if(index>=LegacyAwardCount)return uint64_t(1)<<(32+index-LegacyAwardCount);
  return uint64_t(1)<<(index<16?index+16:index-13);
}
inline constexpr uint64_t AchievementMask=[] {
  uint64_t mask=0;
  for(size_t i=0;i<ExtendedAwardStart;++i)mask|=achievement_bit(Achievement(i));
  return mask;
}();
inline constexpr double AchievementToastTime=3.8;
inline uint64_t stored_achievements(const Profile &profile) {
  return (profile.discoveries|(profile.task_items[0]&0xffffffff00000000ull))&AchievementMask;
}
// Room task ids are 0–23 in the first task word. The second word keeps
// side-story flags at 47–63 and uses its low word for the expanded catalogue.
inline constexpr uint64_t ExtendedAwardMask=(uint64_t(1)<<(Achievements.size()-ExtendedAwardStart))-1;
inline AwardProgress stored_award_progress(const Profile &profile) {
  return {stored_achievements(profile),profile.task_items[1]&ExtendedAwardMask};
}
inline bool has_award(const AwardProgress &flags,Achievement id) {
  return (flags[achievement_word(id)]&achievement_bit(id))!=0;
}
inline AwardProgress achievement_progress(const Profile &profile) {
  auto flags=stored_award_progress(profile);
  const auto include=[&](Achievement id,bool earned) {if(earned)flags[achievement_word(id)]|=achievement_bit(id);};
  include(Achievement::FirstToken,outfitter_earned(profile)>0);
  include(Achievement::DashJump,profile.quest_counts[6]>0);
  include(Achievement::Gear,profile.cosmetics&(HarborScarf|DockBoots|SignalSatchel));
  include(Achievement::Survey,profile.cosmetics&SurveyCourseComplete);
  include(Achievement::Journey,profile.finished);
  include(Achievement::FirstDoor,profile.stage>0||profile.district>0||profile.finished);
  include(Achievement::FirstDistrict,profile.records[0].complete);
  include(Achievement::CleanDistrict,std::any_of(profile.records.begin(),profile.records.end(),
    [](const Record &record){return record.complete&&record.best_deaths==0;}));
  const auto any_mastery=[&](const auto &matches) {
    return std::any_of(profile.trials.begin(),profile.trials.end(),matches)||
      std::any_of(profile.challenges.begin(),profile.challenges.end(),
        [&](const SeedRecord &record){return matches(record.record);});
  };
  include(Achievement::Mastery,any_mastery([](const Record &record){return record.complete;}));
  include(Achievement::CleanMastery,any_mastery([](const Record &record){return record.complete&&record.best_deaths==0;}));
  constexpr uint32_t gear=HarborScarf|DockBoots|SignalSatchel;
  include(Achievement::AllGear,(profile.cosmetics&gear)==gear);
  include(Achievement::FirstTestimony,(profile.discoveries&7)!=0);
  include(Achievement::ThreeTestimonies,(profile.discoveries&7)==7);
  include(Achievement::NextShift,profile.unlocked>=1);
  include(Achievement::CrownReached,profile.unlocked>=5);
  include(Achievement::TrailLeg,profile.endless_best>=RoomsPerLeg);
  include(Achievement::MailBag,(profile.task_items[0]&7)!=0);
  include(Achievement::AllMail,(profile.task_items[0]&7)==7);
  include(Achievement::CrownDown,has_award(flags,Achievement::CrownClean)||profile.finished);
  include(Achievement::MugReturned,profile.task_items[1]&side_flag(SideFlag::HeatworksMugReturned));
  include(Achievement::PowerCut,profile.task_items[1]&side_flag(SideFlag::BasementPowerCut));
  include(Achievement::RosterChoice,profile.task_items[1]&(side_flag(SideFlag::RosterPosted)|side_flag(SideFlag::RosterSealed)));
  constexpr uint64_t clues=side_flag(SideFlag::HarborStub)|side_flag(SideFlag::EstatePlate)|side_flag(SideFlag::ArchiveCopy);
  include(Achievement::PaperTrail,(profile.task_items[1]&clues)==clues);
  include(Achievement::Detour,std::any_of(profile.route_choices.begin(),profile.route_choices.end(),
    [](auto choices){return choices!=0;}));
  include(Achievement::HundredTokens,outfitter_earned(profile)>=100);
  include(Achievement::CleanJourney,std::all_of(profile.records.begin(),profile.records.end(),
    [](const Record &record){return record.complete&&record.best_deaths==0;}));
  return flags;
}

enum class DialogueCue { None, CrownAirlock };
enum class CrownShot { Chamber, Stairwell, Hearing, Quay, Daybreak };
inline constexpr uint64_t CrownReplyCheckpoint=uint64_t(CrownShot::Daybreak)+1;
inline constexpr int CrownFilmShift=2;
inline constexpr double CrownAirlockOpeningSeconds=2.;
struct DialogueLine { int speaker = 0; Text text; DialogueCue cue = DialogueCue::None; CrownShot shot = CrownShot::Chamber; int voice = -1; };
inline constexpr int CourierSpeaker = -2;
inline constexpr int ReplyPrompt = -3;
inline constexpr int BriefingReplyBase = DistrictCount + 1; // After the homecomings and Crown hearing.
// Pair the reply with its district so a save advanced on an older client
// cannot replay an unfinished exchange in the next district.
inline constexpr int BriefingReplyShift = 20;
inline constexpr uint64_t BriefingReplyMask = uint64_t(15) << BriefingReplyShift;
static_assert(DistrictCount * 2 <= 15);
inline constexpr uint64_t briefing_reply_bits(int district,int option) {
  return uint64_t(district*2+option+1) << BriefingReplyShift;
}
inline constexpr int pending_briefing_option(uint64_t story,int district) {
  const int option=int((story & BriefingReplyMask) >> BriefingReplyShift)-district*2-1;
  return option>=0&&option<2?option:-1;
}
// This field keeps the district and the last checkpoint before a reply.
inline constexpr int BriefingOpeningShift = 28;
inline constexpr uint64_t BriefingOpeningMask = uint64_t(15) << BriefingOpeningShift;
inline constexpr uint64_t briefing_opening_bits(int district,bool at_prompt) {
  return uint64_t(district+1+(at_prompt?DistrictCount:0)) << BriefingOpeningShift;
}
inline constexpr uint64_t reply_value(int scene) { return uint64_t(1) << (48+scene); }
inline constexpr uint64_t reply_recorded(int scene) { return uint64_t(1) << (55+scene); }
inline constexpr uint64_t HomecomingReplyPending = uint64_t(1) << 62;
inline constexpr int ArrivalShift = 24;
inline constexpr uint64_t ArrivalMask = uint64_t(15) << ArrivalShift;
inline constexpr int EpilogueArrival = DistrictCount + 1;
inline constexpr int FirstHarborMissionArrival = EpilogueArrival + 1;
inline constexpr int HarborMissionReplyBase = BriefingReplyBase + DistrictCount;
inline constexpr int EpilogueReplyScene = HarborMissionReplyBase + 3;
inline constexpr int IrrigationReplyScene = EpilogueReplyScene + 1;
inline constexpr int HoistCrewReplyScene = IrrigationReplyScene + 1;
inline constexpr int EstateTramReplyScene = HoistCrewReplyScene + 1;
inline constexpr int ArchiveCopiesReplyScene = EstateTramReplyScene + 1;
inline constexpr int ArchiveShelterReplyScene = ArchiveCopiesReplyScene + 1;
inline constexpr int EstateRosterReplyScene = ArchiveShelterReplyScene + 1;
inline constexpr int HeatworksMugReplyScene = EstateRosterReplyScene + 1;
inline constexpr bool expedition_reply_scene(int scene) {
  return scene==IrrigationReplyScene||scene==HoistCrewReplyScene||
    scene==EstateTramReplyScene||scene==ArchiveCopiesReplyScene||scene==ArchiveShelterReplyScene;
}
// Only meaningful while ArrivalMask names the current interactive arrival.
inline constexpr int ArrivalExchangeShift = 34;
inline constexpr uint64_t ArrivalExchangeMask = uint64_t(3) << ArrivalExchangeShift;
enum class ArrivalExchange : uint64_t { Opening, Prompt, FirstReply, SecondReply };
inline constexpr uint64_t arrival_exchange_bits(ArrivalExchange exchange) {
  return uint64_t(exchange) << ArrivalExchangeShift;
}
static_assert(FirstHarborMissionArrival + 2 <= 15);
inline constexpr uint64_t arrival_bits(int scene) { return uint64_t(scene) << ArrivalShift; }
inline constexpr int ForkResumeShift = 32;
inline constexpr uint64_t ForkResumeMask = uint64_t(3) << ForkResumeShift;
inline constexpr uint64_t fork_resume_bits(int leg) { return uint64_t(leg+1) << ForkResumeShift; }
struct Dialogue {
  std::vector<DialogueLine> lines;
  size_t page = 0;
  int action = 0;
  int segment = 0;
  bool instant = false;
  size_t visible_characters=0, byte_cursor=0;
  double character_delay=0, speech_delay=0;
  bool revealing=true;
  std::optional<size_t> reflow_anchor=std::nullopt;
  size_t reflow_revealed=0;
  int reply_scene=-1, reply_option=0;
  int outfitter_item=-1, outfitter_tokens=0;
  bool cinematic=false;
  double shot_seconds=0, line_seconds=0;
  void reset_reveal(){visible_characters=byte_cursor=reflow_revealed=0;character_delay=speech_delay=0;revealing=true;reflow_anchor.reset();}
  void reveal_all(){visible_characters=size_t(-1);revealing=false;}
  bool active() const { return page < lines.size(); }
  bool choosing() const {return active()&&lines[page].speaker==ReplyPrompt;}
};
struct RouteView {
  std::vector<Text> rooms;
  std::vector<Text> browsed_rooms;
  std::optional<Rhythm> current_rhythm,detail_rhythm;
  Text current_feature,detail_feature;
  int completed_legs=0, browsed_leg=0;
  std::array<Text,3> alternatives{};
  uint32_t visited=0;
  unsigned choices=0, decisions=0;
  int leg=0, current=-1;
  bool branching=false, available=false;
  Text place;
};
enum class TourStopSource { Course, Settlement, Basement, NightWindow, ArchiveReader, NightPlatform };
struct TourStop {
  int district,room;
  double entry_x, entry_floor;
  Text change;
  TourStopSource source=TourStopSource::Course;
  Text title{};
};
inline constexpr int CrownShaftPlate = 41;
inline constexpr int ArchiveTrolleyNotice = 42;
inline constexpr int ArchiveDryingLetter = 43;
inline constexpr int ArchiveGlassRoom = 36;
inline constexpr int IrrigationCallBox = 44;
inline constexpr int GardenSeedBank = 56;
inline constexpr int CrownSentinel = 701;
inline constexpr uint64_t CrownArenaReached = uint64_t(1) << 46;
inline constexpr uint64_t CrownSentinelDefeated = uint64_t(1) << 47;
inline constexpr std::array DebugTour{
  TourStop{0,504,25,52,{"Promy, zanurzone kadłuby i ślady na wodzie","Ferries, submerged hulls and wakes"}},
  TourStop{1,18,54,48,{"Rytm pary i boczna półka","Steam timing and the side shelf"}},
  TourStop{2,24,16,54,{"Tramwaje zatrzymują się na obu końcach","Trams stop at both landings"}},
  TourStop{2,25,53,44,{"Ściany między bocznymi balkonami","Wall-kick route between side balconies"}},
  TourStop{3,38,52,38,{"Dwie wyrzutnie poczty pneumatycznej","Two pneumatic post launchers"}},
  TourStop{3,39,57,44,{"Wysuwane szuflady kartoteki","Extending catalogue drawers"}},
  TourStop{3,44,28,50,{"Odwróć środkową taśmę przekaźnikiem","Use the relay to reverse the middle belt"}},
  TourStop{4,49,67,48,{"Strąk odbija do bocznej półki","A seedpod launches to the side shelf"}},
  TourStop{4,51,54,44,{"Wiszące donice pękają pod stopami","Hanging planters crack underfoot"}},
  TourStop{4,57,52,50,{"Wjedź czerpakiem koła na półkę z nasionami","Ride a scoop around the wheel to the seed shelf"}},
  TourStop{5,60,69,48,{"Podmuchy i rękawy wiatrowe","Gusts and windsocks"}},
  TourStop{5,68,77,48,{"Wejdź na maszt i skocz z zieloną strzałką","Climb the mast and dash-jump when the arrow lights"}},
  TourStop{4,55,73,45,{"Kruche kry na zamarzniętym przelewie","Brittle ice over the reservoir spillway"}},
  TourStop{3,47,54,50,{"Regały na wspólnej linie podnoszą się na zmianę","Cable-linked racks rise in turn"}},
  TourStop{5,70,72,41,{"Nawis osłania dolną trasę przed wiatrem","The overhang shelters the lower route"}},
  TourStop{2,26,25,49,{"Przejdź po linach i zeskocz na balkon","Cross the clotheslines and drop to a balcony"}},
  TourStop{5,69,64,51,{"Winda serwisowa prowadzi do górnych półek","A service lift reaches the upper shelves"}},
  TourStop{4,52,77,41,{"Podlej pnącza i przejdź nad kanałem","Water the vines and cross above the channel"}},
  TourStop{3,45,84,46,{"Wózek pod rynną prowadzi na górną półkę","The drainage trolley reaches the upper shelf"}},
  TourStop{5,61,85,45,{"Odbijaj się od ścian, by wrócić na grań","Wall-kick through the shaft to rejoin the ridge"}},
  TourStop{1,13,9,56,{"Wsiadaj, gdy winda zatrzyma się na dole","Board the cargo hoist when it pauses at the lower landing"}},
  TourStop{4,50,19,52,{"Wskocz na liść i przepłyń przez basen","Jump onto a leaf and ride it across the pool"}},
  TourStop{5,63,68,45,{"Skacz po belkach, zanim się załamią","Jump across the beams before they buckle"}},
  TourStop{3,41,19,54,{"Wsiadaj, gdy wagonik zatrzyma się przy pomoście","Board the rail car when it stops at the landing"}},
  TourStop{1,20,54,43,{"Otwórz dopływ i wskocz w strumień pary","Open the supply and jump into the steam"}},
  TourStop{1,23,61.0,32,{"Gdy turbina zapali strzałkę, skocz wzdłuż podmuchu","Jump with the turbine draft when the arrow lights"}},
  TourStop{2,33,66.3,41,{"Odbijaj się między rynnami, by zebrać górny żeton","Wall-kick between the downspouts to reach the upper token"}},
  TourStop{4,58,50,38,{"Gdy zapali się strzałka, wskocz na liść w rynnie","Jump onto the flume leaf when the arrow lights"}},
  TourStop{2,35,42,49,{"Odbijaj się od markiz, by sięgnąć żetonów","Bounce off the awnings to reach the tokens"}},
  TourStop{3,ArchiveGlassRoom,50,46,{"Skacz, zanim szyby pękną pod stopami","Jump again before the glass breaks"}},
  TourStop{1,421,58.5,47,{"Wskocz na tłok, gdy zapali się dolna lampka","Board a piston when its lower lamp lights"}},
  TourStop{4,54,57.5,48,{"Wskocz do donicy i wysiądź przy górnej grządce","Ride the planter and jump to the upper bed"}},
  TourStop{4,GardenSeedBank,56.5,50,{"Wjedź tacą na półki z nasionami","Ride the seed tray up to the shelves"}},
  TourStop{4,48,55.4,39,{"Przytrzymaj skok, by złapać pnącze. Skocz ponownie, by się puścić.",
    "Hold jump to grab the vine. Jump again to release."}},
  TourStop{1,422,111,39,{"Wjedź z podmuchem i zeskocz na boczną półkę","Ride the fan, then steer onto a side shelf"}},
  TourStop{1,424,84,51,{"Odczekaj lub przeskocz po sygnale dyszy","Wait out a vent cycle or jump the warning plume"}},
  TourStop{2,27,34,38,{"Wsiądź na ruchomy balkon, gdy zjedzie na dół",
    "Step onto the balcony when it reaches the lower landing"}},
  TourStop{2,28,112.5,21,{"Pył zapowiada spadający gruz","Watch for dust before the masonry falls"}},
  TourStop{3,37,62,48,{"Wjedź na regale i przebiegnij przed skanerem", "Ride the rack and cross before the scanner sweeps"}},
  TourStop{3,40,82,20,{"Poczekaj, aż skaner minie biurko", "Wait for the scanner to clear the desk"}},
  TourStop{5,64,58.,26,{"Poczekaj na spokojny wiatr. Lód pod stopami pęka.",
    "Wait for a lull. The ice shelf cracks underfoot."}},
  TourStop{0,501,45,39,{"Wejdź na zawiesie i zeskocz na wyższą rampę","Ride the cargo sling and jump to the upper loading ramp"}},
  TourStop{0,504,25,52,{"Piana przy nabrzeżu zapowiada falę", "Foam at the quay warns of a surge"}},
  TourStop{3,103,24,38,{"Ustaw bębny według wydruku wagi","Set the drums from the scale receipt"},TourStopSource::ArchiveReader,{"Czytnik kalek","Carbon reader"}},
  TourStop{3,103,103,48,{"Archiwum: odbierz telefon z linii 11","Archive: answer Line 11"},TourStopSource::Settlement,{"Słuchawka linii 11","Line 11 receiver"}},
  TourStop{2,200+UnnumberedFloor,118,51,{"Piwnica: sprawdź szyb obserwacyjny","Basement: check the observation window"},TourStopSource::NightWindow,{"Szyba obserwacyjna","Observation window"}},
  TourStop{2,200+UnnumberedFloor,79,51,{"Piwnica: przełącz odłącznik przed przewodem","Basement: try the breaker before the live cable"},TourStopSource::Basement,{"Odłącznik piwnicy","Basement breaker"}},
  TourStop{2,102,61,48,{"Pokaż Grażynie odpis listy lokatorów","Show Grażyna the copy of the roster"},TourStopSource::Settlement,{"Lista Grażyny","Grażyna’s roster"}},
  TourStop{4,104,104,50,{"Podnieś doniczkę i pomóż ślimakowi","Lift the pot and help the snail"},TourStopSource::Settlement,{"Ślimak w Ogrodach","Gardens snail"}},
  TourStop{1,101,29,48,{"Zabierz kubek z rury i oddaj go Witkowi","Take the mug from the pipe and return it to Witek"},TourStopSource::Settlement,{"Kubek Witka","Witek’s mug"}},
  TourStop{3,NightPlatformTemplate,8,48,{"Poczekalnia linii 11","Line 11 waiting room"},TourStopSource::NightPlatform,{"Peron 11","Platform 11"}},
  TourStop{5,CrownSentinel,18,48,{"Strażnik Korony — finał","Crown Sentinel — finale"},TourStopSource::Course,{"Strażnik Korony","Crown Sentinel"}},
};
// Keep the quick action pointed at the newest featured sample, independently of list order.
inline constexpr auto DebugTourLatest = [] {
  for(size_t i=0;i<DebugTour.size();++i)
    if(DebugTour[i].source==TourStopSource::NightPlatform)return i;
  return DebugTour.size();
}();
static_assert(DebugTourLatest<DebugTour.size());
inline int tour_page_size(int rows) { return std::max(1,rows-12); }
struct View {
  RouteView route;
  int save_slot=1,picker_choice=0,tour_scene=-1;
  bool save_picker=false,tour_picker=false;
  std::array<int,3> slot_status{};
  Language language = Language::Polish;
  Mode mode = Mode::Campaign;
  int district = 0, stage = 0, room = 0, depth = 0, selected = 0;
  int unlocked = 0, outfit = 0;
  int hidden_room = -1, visit_district = -1;
  int run_district = 0, endless_best = 0;
  bool revisit = false;
  bool can_travel = false, map_route = false, map_room = false;
  int journal_tab = 0, quest_focus = -1, journey_district = 0, route_page = -1, route_room_selection = -1;
  uint64_t story = 0;
  std::string current_objective;
  uint32_t seed = 1;
  uint32_t cosmetics = 0;
  bool map = false, settings = false, confirm_new = false, finished = false, at_content_boundary = false;
  bool save_error = false, can_continue = false;
  bool endless_resume = false, trial_resume = false, trial_replay = false;
  bool branch_at_door = false;
  uint64_t switches = 0;
  uint32_t discoveries = 0;
  AwardProgress awards{};
  bool platform_listening=false;
  bool platform_shutter_open=false;
  double platform_shutter_time=0.;
  double platform_wait=0., platform_departure=0., pressure_test_left=0.;
  std::vector<std::pair<double,double>> platform_reflection;
  double platform_sample_left=0.;
  int night_beat=0;
  double night_left=0., night_plate_time=0.;
  std::array<unsigned,3> carbon_dials{};
  std::array<unsigned, 8> quests{};
  std::array<unsigned, 8> quest_counts{};
  std::array<uint64_t,2> task_items{};
  std::array<Record, DistrictCount> records{}, trials{};
  bool inspecting=false;
  std::string inspect_title, inspect_body, inspect_icon;
  InspectionArt inspect_art=InspectionArt::Parcel;
  int inspect_detail=0;
  size_t inspect_offset=0;
  double pickup_left=0, pickup_x=0, pickup_y=0;
  int pickup_kind=0;
  double door_left=0, door_open=0;
  double machinery_left=0, machinery_duration=0, machinery_x=0, machinery_y=0;
  int machinery_kind=0;
  uint64_t machinery_changed=0;
  double presentation_time=0, npc_reaction_left=0, lamp_left=0, death_left=0;
  bool achievement_menu=false;
  int achievement_selection=0;
  AwardGroup achievement_group=AwardGroup::All;
  int achievement_toast=-1;
  double achievement_left=0;
  static constexpr double water_splash_duration=.42;
  double water_splash_left=0, water_splash_x=0, water_splash_y=0;
  double garden_snail_left=0, seed_pod_left=0;
  int reacting_npc=-1;
  Dialogue dialogue;
  std::string prompt, notice, room_name;
  Text traversal_hint;
  double notice_left = 0;
  bool entry_notice = false;
  float music_volume = .65f, effects_volume = .75f, ambience_volume = .55f;
  bool pause_on_focus_loss = true;
  bool smooth_camera = false;
};
struct SessionOptions {
  uint32_t seed = 1;
  Language language = Language::Polish;
  bool language_override = false, district_override = false, mute = false, remote_audio = false;
  bool remote = false, no_save = false, seed_override = false;
  Mode mode = Mode::Campaign;
  std::string ghost_host,ghost_port="27888",ghost_room="default";
  int district = 0, save_slot = 1;
};
inline std::string tr(Language l, const char *pl, const char *en) {
  return l == Language::English ? en : pl;
}
inline uint32_t mix(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352d; x ^= x >> 15; x *= 0x846ca68b;
  return x ^ (x >> 16);
}
// Specified RNG output, unlike uniform_int_distribution, is stable across
// standard-library implementations and the Windows/Linux release builds.
struct Random {
  uint32_t value;
  uint32_t next() { value += 0x9e3779b9u; return mix(value); }
  int below(int n) { return int(next() % unsigned(n)); }
};
} // namespace iceberg

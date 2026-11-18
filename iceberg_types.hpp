#pragma once
#include <array>
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
enum class Language { Polish, English };
enum class Mode { Campaign, Endless, Trial };
enum class RoomRole { Traverse, Lesson, Finale, Settlement, Secret, Interlude };
enum class Rhythm { Flow, Precision, Vertical, Timing, Explore, Recovery };
enum class Mechanic { None, Current, Steam, Conveyor, Echo, Growth, Wind };
enum class InspectionArt { Parcel, Valve, Signal, Address, File, Seeds, Speaker, Thermometer, Sign, Dedication, Board, Door, Coin, Refill, Lamp, Character, Delivery };
struct Text {
  const char *pl = "";
  const char *en = "";
  std::string get(Language language) const {
    return language == Language::English && en && *en ? en : pl;
  }
};
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
  bool active = false;
  uint32_t seed = 1, origin_seed = 1;
  int district = 0, room = 0, depth = 0, checkpoint = 0;
  uint64_t collected = 0, switches = 0;
  double room_seconds = 0, seconds = 0;
  int room_deaths = 0, deaths = 0, treasures = 0;
  std::vector<int> itinerary;
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

enum class DialogueCue { None, CrownAirlock };
struct DialogueLine { int speaker = 0; Text text; DialogueCue cue = DialogueCue::None; };
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
inline constexpr uint64_t reply_value(int scene) { return uint64_t(1) << (48+scene); }
inline constexpr uint64_t reply_recorded(int scene) { return uint64_t(1) << (55+scene); }
inline constexpr uint64_t HomecomingReplyPending = uint64_t(1) << 62;
inline constexpr int ArrivalShift = 24;
inline constexpr uint64_t ArrivalMask = uint64_t(15) << ArrivalShift;
inline constexpr int EpilogueArrival = DistrictCount + 1;
static_assert(EpilogueArrival <= 15);
inline constexpr uint64_t arrival_bits(int scene) { return uint64_t(scene) << ArrivalShift; }
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
  void reset_reveal(){visible_characters=byte_cursor=reflow_revealed=0;character_delay=speech_delay=0;revealing=true;reflow_anchor.reset();}
  void reveal_all(){visible_characters=size_t(-1);revealing=false;}
  bool active() const { return page < lines.size(); }
  bool choosing() const {return active()&&lines[page].speaker==ReplyPrompt;}
};
struct RouteView {
  std::vector<Text> rooms;
  std::array<Text,3> alternatives{};
  uint32_t visited=0;
  unsigned choices=0, decisions=0;
  int leg=0, current=-1;
  bool branching=false, available=false;
  Text place;
};
struct TourStop {
  int district,room;
  double entry_x, entry_floor;
  Text change;
};
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
  TourStop{3,44,28,50,{"Taśmy odpisów zmieniają kierunek","Copy belts alternate direction"}},
  TourStop{4,49,67,48,{"Strąk odbija do bocznej półki","A seedpod launches to the side shelf"}},
  TourStop{4,51,54,44,{"Wiszące donice pękają pod stopami","Hanging planters crack underfoot"}},
  TourStop{4,57,20.5,57,{"Ruchome wiadra koła nawadniającego","The irrigation wheel's bucket lifts"}},
  TourStop{5,60,69,48,{"Podmuchy i rękawy wiatrowe","Gusts and windsocks"}},
  TourStop{5,68,74,37,{"Wiatr zmienia kierunek nad górnymi półkami","Crosswinds reverse above the upper ledges"}},
  TourStop{4,55,73,45,{"Kruche kry na zamarzniętym przelewie","Brittle ice over the reservoir spillway"}},
  TourStop{3,47,54,50,{"Regały na wspólnej linie podnoszą się na zmianę","Cable-linked racks rise in turn"}},
  TourStop{5,70,72,41,{"Nawis osłania dolną trasę przed wiatrem","The overhang shelters the lower route"}},
  TourStop{2,26,25,49,{"Przejdź po linach i zeskocz na balkon","Cross the clotheslines and drop to a balcony"}},
  TourStop{5,69,64,51,{"Winda serwisowa prowadzi do górnych półek","A service lift reaches the upper shelves"}},
  TourStop{4,52,77,41,{"Podlej pnącza i przejdź nad kanałem","Water the vines and cross above the channel"}},
  TourStop{5,CrownSentinel,18,48,{"Strażnik Korony — finał","Crown Sentinel — finale"}},
};
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
  bool endless_resume = false, trial_resume = false;
  bool branch_at_door = false;
  uint64_t switches = 0;
  uint32_t discoveries = 0;
  std::array<unsigned, 8> quests{};
  std::array<unsigned, 8> quest_counts{};
  std::array<uint64_t,2> task_items{};
  std::array<Record, DistrictCount> records{}, trials{};
  bool inspecting=false;
  std::string inspect_title, inspect_body, inspect_icon;
  InspectionArt inspect_art=InspectionArt::Parcel;
  int inspect_detail=0, inspect_page=0;
  double pickup_left=0, pickup_x=0, pickup_y=0;
  int pickup_kind=0;
  double door_left=0, door_open=0;
  double machinery_left=0, machinery_duration=0, machinery_x=0, machinery_y=0;
  int machinery_kind=0;
  uint64_t machinery_changed=0;
  double presentation_time=0, npc_reaction_left=0, lamp_left=0;
  static constexpr double water_splash_duration=.42;
  double water_splash_left=0, water_splash_x=0, water_splash_y=0;
  int reacting_npc=-1;
  Dialogue dialogue;
  std::string prompt, notice, room_name;
  double notice_left = 0;
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

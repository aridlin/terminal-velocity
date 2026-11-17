#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace iceberg {
inline constexpr unsigned ContentVersion = 3;
inline constexpr int DistrictCount = 6;
// Content registry boundary: increment when another district is fully authored.
inline constexpr int AuthoredDistrictCount = 2;
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
  static const Text routes[2][3]={
    {{"Kanał odpływowy","Drainage channel"},{"Pomosty przepompowni","Pumping station boardwalks"},{"Rurociąg do Łazarza","Pipeline to Lazarus"}},
    {{"Dopływ pary","Steam supply"},{"Obieg mieszkań","Residential circuit"},{"Powrót awaryjny","Emergency return"}}
  };
  if(stage==3)leg=1;
  if(stage==5)leg=2;
  return district<2?routes[district][leg]:Districts[size_t(district)].objective;
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
  int outfit = 0;
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
  std::vector<SeedRecord> challenges;
  // Persist the assembled itinerary as well as its seed: content additions
  // cannot silently reroll an expedition that somebody has already started.
  std::vector<int> itinerary;
  int checkpoint = 0;
  uint64_t collected = 0, switches = 0;
  double room_seconds = 0;
  int room_deaths = 0;
  float music_volume = .65f, effects_volume = .75f, ambience_volume = .55f;
};

struct DialogueLine { int speaker = 0; Text text; };
struct Dialogue {
  std::vector<DialogueLine> lines;
  size_t page = 0;
  int action = 0;
  bool active() const { return page < lines.size(); }
};
struct View {
  Language language = Language::Polish;
  Mode mode = Mode::Campaign;
  int district = 0, stage = 0, room = 0, depth = 0, selected = 0;
  int unlocked = 0, outfit = 0;
  int hidden_room = -1, visit_district = -1;
  int run_district = 0, endless_best = 0;
  bool revisit = false;
  uint32_t seed = 1;
  uint32_t cosmetics = 0;
  bool map = false, settings = false, confirm_new = false, finished = false, at_content_boundary = false;
  bool save_error = false, can_continue = false;
  bool endless_resume = false, trial_resume = false;
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
  Dialogue dialogue;
  std::string prompt, notice, room_name;
  double notice_left = 0;
  float music_volume = .65f, effects_volume = .75f, ambience_volume = .55f;
};
struct SessionOptions {
  uint32_t seed = 1;
  Language language = Language::Polish;
  bool language_override = false, district_override = false, mute = false, remote_audio = false;
  bool remote = false, no_save = false, seed_override = false;
  Mode mode = Mode::Campaign;
  std::string ghost_host,ghost_port="27888",ghost_room="default";
  int district = 0;
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

#pragma once
// Included after EngineGrid: content produces the same collision world used
// by the terminal, the SDL cell view, diagnostics, and simulation tests.
namespace iceworld {
using namespace ::iceberg;
struct Pad { int x, y, w; char kind = 'p'; };
struct Pattern {
  int id;
  Text name;
  Rhythm rhythm;
  std::vector<Pad> path;
  std::vector<Pad> optional;
  int district = -1;
};
inline const std::vector<Pattern> &patterns() {
  static const std::vector<Pattern> values{
    {0,{"Pomosty na kredyt","Borrowed boardwalks"},Rhythm::Flow,
      {{2,52,9},{16,50,6},{27,47,7},{39,49,5},{49,46,8},{62,43,6},{73,45,7},{85,42,6},{96,39,8},{109,41,5},{119,38,7},{131,35,9}},
      {{33,40,4},{45,37,4},{58,34,5}}},
    {1,{"Studnia odpływowa","Drainage well"},Rhythm::Vertical,
      {{3,56,10},{19,53,7},{30,50,7},{41,47,8},{55,44,6},{65,41,8},{54,38,6},{42,35,7},{30,32,7},{18,29,7},{7,26,7},{19,23,7},{31,20,7},{43,17,10}},
      {{60,31,4},{66,24,5}}},
    {2,{"Sortownia listów mokrych","Wet-letter sorting office"},Rhythm::Precision,
      {{3,52,9},{17,49,4},{26,47,4},{35,45,4},{44,48,8},{57,45,4},{66,42,4},{75,40,4},{84,43,9},{98,40,4},{107,37,5},{118,34,12}},
      {{48,39,5},{61,33,5}}},
    {3,{"Droga bez zimowego utrzymania","No winter maintenance"},Rhythm::Timing,
      {{3,52,10},{18,50,7},{30,47,7,'m'},{43,44,8},{56,47,7},{68,44,7,'m'},{81,41,9},{95,38,7},{108,35,7,'m'},{122,32,12}},
      {{48,36,5},{62,31,6}}},
    {4,{"Podbrzusze promu","Under the ferry"},Rhythm::Explore,
      {{3,51,12},{21,48,8},{35,45,8},{49,42,8},{63,39,9},{78,42,7},{90,45,9},{104,42,8},{118,39,8},{132,36,11}},
      {{49,34,6},{61,31,7},{76,30,7},{90,32,6}}},
    {5,{"Kanał służbowy","Service channel"},Rhythm::Flow,
      {{3,54,10},{19,51,8},{33,48,8},{47,45,9},{62,42,8},{76,39,8},{90,36,9},{105,33,8},{119,30,8},{133,27,12}},
      {{37,40,4},{51,36,4},{66,32,4}}},
    {6,{"Węzeł trzech sygnałów","Three-signal junction"},Rhythm::Vertical,
      {{3,56,10},{19,53,8},{33,50,8},{47,47,8},{61,44,9},{48,41,7},{35,38,7},{22,35,8},{9,32,8},{23,29,8},{37,26,8},{51,23,8},{65,20,12}},
      {{76,35,5},{84,29,5}}},
    {7,{"Nabrzeże z drugiej ręki","Second-hand quay"},Rhythm::Recovery,
      {{3,52,16},{23,50,14},{41,48,14},{59,46,14},{77,44,14},{95,42,14},{113,40,16}},
      {{31,41,5},{46,36,5},{61,31,7},{77,34,5},{92,37,5}}},
    {8,{"Cieśnina lamp","Lantern strait"},Rhythm::Precision,
      {{3,53,9},{17,50,5},{27,47,5},{37,44,5},{47,41,7},{59,44,5},{69,47,7},{81,44,5},{91,41,5},{101,38,5},{111,35,5},{121,32,12}},
      {{54,33,5},{67,29,5},{80,33,5}}},
    {9,{"Pokład kołyszący","Swaying deck"},Rhythm::Timing,
      {{3,53,10},{19,50,8,'m'},{34,47,10},{50,44,8,'m'},{65,41,10},{81,38,8,'m'},{96,35,10},{112,32,8,'m'},{127,29,13}},
      {{39,38,5},{55,32,5},{71,28,6}}},
    {10,{"Szyb po latarniku","Lamplighter's shaft"},Rhythm::Vertical,
      {{3,57,10},{18,54,7},{30,51,7},{42,48,9},{30,45,7},{18,42,7},{6,39,8},{19,36,7},{31,33,7},{43,30,9},{31,27,7},{19,24,7},{7,21,8},{20,18,9},{34,15,12}},
      {{53,38,5},{60,31,5},{54,23,5}}},
    {11,{"Ostatnia mila poczty","The post's last mile"},Rhythm::Explore,
      {{3,54,12},{21,51,9},{36,48,8},{50,45,9},{65,42,8},{79,39,9},{94,36,8},{108,33,9},{123,30,12}},
      {{27,42,5},{40,37,5},{54,32,5},{68,27,7},{84,28,5},{98,25,6}}}

#include "iceberg_rooms_heat.inc"
#include "iceberg_rooms_revision.inc"
#include "iceberg_rooms_estate.inc"
#include "iceberg_rooms_archive.inc"
#include "iceberg_rooms_gardens.inc"
#include "iceberg_rooms_crown.inc"
#include "iceberg_rooms_harbor.inc"
  };
  return values;
}

inline int district_of(const Pattern &p) {return p.district<0?p.id/12:p.district;}

inline const Pattern &pattern(int id) {
  const auto &all = patterns();
  for (const auto &p : all) if (p.id == id) return p;
  return all.front();
}
enum MoveAbility : unsigned { Run=1, Jump=2, Dash=4, WallJump=8, DashJump=16, WallBounce=32, LiftLaunch=64 };
struct TraversalPort {double x,y;int landing_width;bool dash_available;};
struct RoomMetadata {
  TraversalPort entrance,exit;
  unsigned required=Run|Jump|Dash, optional_required=Run|Jump|Dash;
  int difficulty=1;
  std::vector<std::pair<double,double>> resting_points;
  std::vector<std::pair<double,double>> optional_branch;
  bool permits_wider_landings=true, permits_platform_phase=true;
};
inline RoomMetadata describe(const Pattern &p) {
  auto a=p.path.front(),b=p.path.back();
  RoomMetadata m{};m.entrance={a.x+2.,double(a.y),a.w,true};m.exit={b.x+b.w-2.,double(b.y),b.w,true};
  m.permits_wider_landings=p.rhythm!=Rhythm::Vertical;
  m.difficulty=p.rhythm==Rhythm::Precision?3:p.rhythm==Rhythm::Vertical||p.rhythm==Rhythm::Timing?2:1;
  for(auto pad:p.path)if(pad.w>=7&&pad.kind!='m'&&pad.kind!='h'&&pad.kind!='v')m.resting_points.push_back({pad.x+pad.w*.5,double(pad.y)});
  for(auto pad:p.optional)m.optional_branch.push_back({pad.x+pad.w*.5,double(pad.y)});
  if(!p.optional.empty())m.optional_required|=WallJump|DashJump;
  return m;
}
inline bool connects(const Pattern &from,const Pattern &to) {
  auto a=describe(from),b=describe(to);
  // Door transitions arrive on a resting ledge with a restored dash. The
  // ports are explicit so future continuous connections can use the same test.
  return a.exit.landing_width>=4&&b.entrance.landing_width>=4&&
    (a.exit.dash_available||!(b.required&Dash));
}
inline bool has_expedition_mechanic(int district,int id) {
  // Rooms with district-specific traversal, including mechanics added after
  // the original Endless reserve. A naturally selected room satisfies it.
  switch(district) {
  case 1:return id==18||id==22;                                             // Steam jets
  case 2:return id==24||id==25;                                             // Trams, stairwells
  case 3:return id==38||id==39||id==44||id==47;                             // Post, drawers, belts, racks
  case 4:return id==48||id==49||id==50||id==51||id==55||id==57||id==58;       // Ice, pods, ferries, lifts
  case 5:return id==60||id==62||id==63||id==66||id==68||id==69||id==70||id==71; // Wind and cradles
  default:return false;
  }
}
inline std::vector<int> make_itinerary(uint32_t seed, int district, int count = RoomsPerLeg * 3,
                                      unsigned layout_version=ContentVersion, Mode mode=Mode::Campaign) {
  Random random{mix(seed ^ unsigned(district * 7919))};
  std::vector<int> pool;
  for (const auto &p : patterns()) if (district_of(p) == district &&
      (district!=0 || (layout_version>=4 ? p.id>=500&&p.id<=511 : p.id<500)) &&
      (layout_version>=3||district>=2||p.id<24)) pool.push_back(p.id);
  if (pool.empty()) for (const auto &p : patterns()) if (district_of(p) == 0) pool.push_back(p.id);
  std::vector<int> result;
  int previous = -1;
  const bool reserve_mechanic=mode==Mode::Endless&&layout_version>=4&&district>=1&&district<AuthoredDistrictCount;
  // Some legs end on an exploration room, so a district with only one
  // recovery room doesn't keep returning to the same ending.
  const bool explore_closer=reserve_mechanic&&(mix(seed ^ 0x62f4a1bdu)&1u);
  int early_mechanic=-1;
  const auto has_mechanic=[&](int id){return has_expedition_mechanic(district,id);};
  while (int(result.size()) < count) {
    auto bag = pool;
    for (size_t i = bag.size(); i > 1; --i) std::swap(bag[i - 1], bag[size_t(random.below(int(i)))]);
    if(reserve_mechanic&&result.empty()) {
      const auto local=std::find_if(bag.begin(),bag.end(),has_mechanic);
      if(local!=bag.end())early_mechanic=*local;
    }
    // Within every leg alternate flow/precision/vertical/timing with relief.
    while (!bag.empty() && int(result.size()) < count) {
      const bool harbor_graph=district==0&&layout_version>=4;
      // The seeded shuffle chooses the featured mechanic. The room's rhythm
      // can place it naturally; otherwise it gets the fourth slot.
      const bool mechanic_due=early_mechanic>=0&&result.size()==3;
      size_t choice=0;int best=harbor_graph?INT_MAX:10000;
      static const Rhythm phrase[]={Rhythm::Flow,Rhythm::Explore,Rhythm::Precision,Rhythm::Vertical,
        Rhythm::Timing,Rhythm::Flow,Rhythm::Vertical,Rhythm::Recovery};
      const auto slot=result.size()%RoomsPerLeg;
      const auto desired=explore_closer&&slot==RoomsPerLeg-1?Rhythm::Explore:phrase[slot];
      for(size_t i=0;i<bag.size();++i) {
        if(mechanic_due&&bag[i]!=early_mechanic)continue;
        const auto &candidate=pattern(bag[i]);
        int score=(candidate.rhythm==desired?0:3)+(bag[i]==previous?100:0);
        // Reserve collection-task rooms before the final settlement visit.
        // Expanding the library must not move a task's last object beyond its
        // owner's last ordinary turn-in opportunity.
        auto reserved=[&](int id){return district==0?(layout_version>=4?(id==500||id==503||id==506):(id==0||id==4||id==8)):
          district==1?(id==12||id==13||id==16||id==17||id==20||id==21):district==2?(id==24||id==26||id==28||id==30||id==32||id==34):district==3?(id==36||id==40||id==44):district==4?(id==48||id==52||id==56):false;};
        // A fork replaces only this traversal cell, never a parcel landmark.
        if(harbor_graph) {
          const bool fork=result.size()%8==2;
          if(fork&&reserved(bag[i]))continue;
          // Eligibility, not a score penalty: once the remaining normal cells
          // are all needed by parcel landmarks, this normal cell must hold a
          // parcel too. Fork cells always retain a non-landmark candidate.
          int future_allowed=0;
          for(size_t remaining=1;remaining<bag.size();++remaining)
            if((result.size()+remaining)%8!=2)++future_allowed;
          int outstanding=int(std::count_if(bag.begin(),bag.end(),reserved));
          if(!fork&&outstanding>future_allowed&&!reserved(bag[i]))continue;
        }
        if(layout_version>=3&&count>=16&&result.size()<16) {
          int outstanding=int(std::count_if(bag.begin(),bag.end(),reserved));
          if(16-int(result.size())<=outstanding&&!reserved(bag[i]))score+=10000;
        }
        if(previous>=0) {
          if(!connects(pattern(previous),candidate))score+=1000;
          if(pattern(previous).rhythm==candidate.rhythm)score+=5;
        }
        if(score<best){best=score;choice=i;}
      }
      previous = bag[choice];
      if(previous==early_mechanic)early_mechanic=-1;
      result.push_back(previous);
      bag.erase(bag.begin() + long(choice));
    }
  }
  return result;
}

inline int harbor_alternate(const std::vector<int> &itinerary,int leg,uint32_t seed) {
  if(leg<0||leg>2||itinerary.size()<size_t(leg*8+4))return -1;
  std::vector<int> candidates;
  for(int id:{501,502,504,505,507,508,509,510,511})
    if(id!=itinerary[size_t(leg*8+1)]&&id!=itinerary[size_t(leg*8+2)]&&id!=itinerary[size_t(leg*8+3)])candidates.push_back(id);
  return candidates[size_t(mix(seed^unsigned(leg*7919+441)))%candidates.size()];
}
inline void decorate_harbor_room(EngineGrid &g,const Pattern &spec);
inline EngineGrid build_room(int id, uint32_t seed, int district, bool expert = false, bool stable = false) {
  const auto &spec = pattern(id);
  Random variation{mix(seed^unsigned(id*65537))};
  int width = 80;
  for (auto p : spec.path) width = std::max(width, p.x + p.w + 7);
  for (auto p : spec.optional) width = std::max(width, p.x + p.w + 7);
  EngineGrid g(width, 64);
  g.event_level = false;
  g.adventure_level = true;
  g.district = district;
  g.template_id = id;
  g.seed = seed;
  g.room_role = RoomRole::Traverse;
  g.room_name = spec.name;
  auto first = spec.path.front(), last = spec.path.back();
  g.spawn_x = first.x + 2;
  g.spawn_y = first.y - kPlayerHeight;
  g.goal = {double(last.x + last.w - 3), double(last.y - 4), 2, 4};
  g.checkpoint_points.push_back({g.spawn_x, g.spawn_y});
  g.route_points.push_back({g.spawn_x, double(first.y)});
  for (size_t i = 0; i < spec.path.size(); ++i) {
    auto p = spec.path[i];
    // Trials demand accurate landings while retaining the room's silhouette.
    if(expert&&i>0&&i+1<spec.path.size()&&p.w>=7&&spec.rhythm!=Rhythm::Vertical){++p.x;p.w-=2;}
    if (p.kind == 'm'||p.kind=='h'||p.kind=='v') {
      // Conservative movement envelopes preserve both neighboring landings.
      g.platforms.push_back({{double(p.x), double(p.y), double(p.w), .5}, p.kind=='h'?2.8:p.kind=='v'?0.:.6, p.kind=='v'?1.8:p.kind=='h'?0.:.5, stable?4.5:3.5+variation.below(21)*.1, stable?0.:variation.below(100)*.01});
    } else g.add_solid_rect(p.x, p.y, p.w+(!stable&&i+1<spec.path.size()&&spec.rhythm!=Rhythm::Vertical?variation.below(2):0), 2);
    if (i) g.route_points.push_back({p.x + p.w * .5, double(p.y)});
    if (i && i % 4 == 0 && p.kind != 'm'&&p.kind!='h'&&p.kind!='v')
      g.checkpoint_points.push_back({p.x + 1.5, p.y - kPlayerHeight});
    if (i % 3 == 2) g.sweets.push_back({p.x + p.w * .5, p.y - 2.3});
    if (i > 0 && i % 4 == 2) {
      auto prev = spec.path[i - 1];
      g.refills.push_back({(p.x + prev.x + prev.w) * .5, std::min(p.y, prev.y) - 2.5});
    }
  }
  for (size_t i = 0; i < spec.optional.size(); ++i) {
    auto p = spec.optional[i];
    g.add_solid_rect(p.x, p.y, p.w, 1);
    g.sweets.push_back({p.x + p.w * .5, p.y - 1.5});
    if (i % 2 == 0) g.refills.push_back({p.x - 2., p.y + 1.});
  }
  if (id<72 && !spec.optional.empty() && (id % 12 == 0 || id % 12 == 4 || id % 12 == 8)) {
    auto p = spec.optional.back();
    int quest = district == 0 ? 0 : district == 1 ? 1 : district == 2 ? 2 : district == 3 ? 4 : 5;
    g.interactions.push_back({{p.x + 1.,p.y - 2.,2,2},InteractionKind::Task,quest,
      quest==1?Text{"Otwórz zawór","Open valve"}:quest==2?Text{"Napraw sygnał","Repair signal"}:quest==4?Text{"Zabierz teczkę","Recover file"}:
      quest==5?Text{"Zabierz nasiona","Take seeds"}:Text{"Zbadaj przesyłkę","Inspect parcel"},quest * 3 + (id % 12) / 4});
  }
  if(id<72 && !spec.optional.empty() && ((district==2&&id%4==2)||(district==1&&id%4==1))) {
    auto p=spec.optional.back();int quest=district==2?3:7;
    g.interactions.push_back({{p.x+1.,p.y-2.,2,2},InteractionKind::Task,quest,
      quest==3?Text{"Sprawdź tabliczkę","Read the plaque"}:Text{"Dostrój głośnik","Tune speaker"},quest*3+(id%12)/4});
  }
  // Catching a fall is a fast retry; no long free fall beneath the room.
  g.add_hazard_rect(0, 62, width, 2);
  if(id<24 && district==1 && !stable && (spec.rhythm==Rhythm::Timing||spec.rhythm==Rhythm::Precision)) {
    for(size_t i=3;i+1<spec.path.size();i+=4) {
      auto p=spec.path[i];if(p.kind=='m'||p.w<10)continue;
      g.pulses.push_back({{p.x+p.w*.5,p.y-2.5,1.2,2.5},4.2,variation.below(100)*.01});
    }
  }
  // Broad ceilings create low passages, followed by exposed leaps. They are
  // behind/below the optional upper route, never camouflage for a hazard.
  if(id==400||id==420) {
    for(size_t i=1;i<4;++i){auto p=spec.path[i];g.add_solid_rect(p.x-1,0,p.w+2,p.y-6);}
  }
  if(id==403) {
    for(int i:{1,4}){auto p=spec.path[size_t(i)];g.add_hazard_rect(p.x+p.w-1,p.y-1,1,1);}
  }
  if(id==423) {
    for(int i:{1,3,5}){auto p=spec.path[size_t(i)];g.pulses.push_back({{p.x+7.,p.y-2.5,1.5,2.5},4.8,double(i)*.13});}
  }
  if(district==5&&(id==60||id==63||id==66||id==70)) {
    g.air_fields.push_back({{69,15,44,44},id==63||id==70?-4.:4.,0,6.,0});
  }
  if(id==50) {
    for(size_t i=1;i+1<spec.path.size();i+=2) {
      const auto &bank=spec.path[i-1];
      const double left=bank.x+bank.w, right=spec.path[i+1].x;
      const double surface=spec.path[i].y+1.5;
      g.water_basins.push_back({left,surface,right-left,62-surface});
    }
  }
  if(id>=500&&id<=511)decorate_harbor_room(g,spec);
  return g;
}

#include "iceberg_sequences.hpp"
#include "iceberg_estate.hpp"
#include "iceberg_archive.hpp"
#include "iceberg_gardens.hpp"
#include "iceberg_crown.hpp"
#include "iceberg_missions.hpp"
#include "iceberg_harbor.hpp"
#include "iceberg_jubilee.hpp"
#include "iceberg_mastery.hpp"

inline EngineGrid build_authored(int district, bool finale) {
  if(district==1)return build_heatworks_sequence(finale);
  if(district==2)return build_estate_sequence(finale);
  if(district==3)return build_archive_sequence(finale);
  if(district==4)return build_garden_sequence(finale);
  if(district==5)return build_crown_sequence(finale);
  EngineGrid g(finale ? 144 : 128, 64);
  g.event_level = false; g.adventure_level = true; g.district = district;
  g.template_id = 72 + district * 2 + int(finale);
  g.room_role = finale ? RoomRole::Finale : RoomRole::Lesson;
  g.room_name = finale ? Text{"Serce przepompowni","Heart of the pumping station"}
                      : Text{"Szkoła bez dzwonka","School without a bell"};
  std::vector<Pad> pads = finale
    ? std::vector<Pad>{{2,55,14},{23,52,9},{39,49,9},{55,46,11},{73,43,9},{89,40,9},{105,37,9},{121,34,17}}
    : std::vector<Pad>{{2,55,22},{30,52,15},{51,49,14},{71,46,15},{92,43,13},{111,40,12}};
  for (size_t i=0;i<pads.size();++i) {
    auto p=pads[i]; g.add_solid_rect(p.x,p.y,p.w,2);
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    g.sweets.push_back({p.x+p.w*.6,p.y-2.});
    if (i%2==0) g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});
  }
  g.spawn_x=5;g.spawn_y=55-kPlayerHeight;
  auto p=pads.back();g.goal={p.x+p.w-3.,p.y-4.,2,4};
  g.add_hazard_rect(0,62,g.width(),2);
  if (finale) {
    g.goal_requires=1;
    g.interactions.push_back({{58,42,3,4},InteractionKind::Valve,0,{"Otwórz obieg","Open circuit"}});
    g.gates.push_back({{134,29,1,5},1});
    if(district==0||district==4)g.enemies.push_back({60,64,45,1.4});
    g.platforms.push_back({{80,31,7,.5},0,3,3.4,0});
    g.refills.push_back({84,26});
    if(district==0)g.interactions.push_back({{80,26,7,5},InteractionKind::Secret,2,{"",""}});
  } else {
    g.interactions.push_back({{7,51,4,4},InteractionKind::Note,0,{"Przeczytaj tabliczkę","Read sign"}});
    g.interactions.push_back({{36,48,4,4},InteractionKind::Note,1,{"Przeczytaj tabliczkę","Read sign"}});
    g.interactions.push_back({{74,42,4,4},InteractionKind::Note,2,{"Przeczytaj tabliczkę","Read sign"}});
    g.platforms.push_back({{54,39,7,.5},0,3,3.2,0});
    g.add_solid_rect(66,31,2,10);
    g.refills.push_back({64,36});
    g.sweets.push_back({71,29});
    g.add_solid_rect(70,32,8,1);
    if(district==0)g.interactions.push_back({{72,29,3,3},InteractionKind::Secret,1,{"",""}});
  }
  return g;
}

inline EngineGrid build_settlement(int district) {
  if(district==1)return build_heatworks_settlement();
  if(district==2)return build_estate_settlement();
  if(district==3)return build_archive_settlement();
  if(district==4)return build_garden_settlement();
  if(district==5)return build_crown_settlement();
  EngineGrid g(94, 58);
  g.event_level = false; g.adventure_level = true; g.district = district;
  g.template_id = 100 + district; g.room_role = RoomRole::Settlement;
  g.room_name = Districts[size_t(district)].name;
  g.add_solid_rect(0, 48, 94, 10);
  g.add_solid_rect(20, 43, 13, 2);
  g.add_solid_rect(38, 39, 14, 2);
  g.add_solid_rect(58, 43, 13, 2);
  g.spawn_x = 6; g.spawn_y = 48 - kPlayerHeight;
  g.goal = {86, 44, 3, 4};
  g.checkpoint_points = {{6, g.spawn_y}};
  g.springs.push_back({17, 47.7, 1.5, .3});
  const int npc_a[] = {0,1,2,4,5,6}, npc_b[] = {6,7,3,7,0,4};
  g.interactions.push_back({{11,44,4,4},InteractionKind::Npc,npc_a[district],{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{42,35,4,4},InteractionKind::Npc,npc_b[district],{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{77,44,4,4},InteractionKind::Board,district,{"Tablica wypraw","Expedition board"}});
  if(district==0)g.interactions.push_back({{27,40,3,3},InteractionKind::Secret,0,{"",""}});
  return g;
}
} // namespace iceberg

#pragma once
// Included in iceworld after the ordinary builders. New IDs are deliberately
// separate from the published routes, including their recovery/checkpoint data.
inline bool harbor_v4_room(int id) { return id>=500&&id<=511; }
inline bool harbor_v4_mission(int id) { return id>=310&&id<=312; }
inline int harbor_mail_item(int id) { return id==500?0:id==503?1:id==506?2:-1; }
inline std::vector<DialogueLine> harbor_mission_arrival(int mission) {
  if(mission==0)return {{0,{"Bogdan utknął w tramwaju. Przejdź po dwóch stykach zasilania na górnej drodze, zawołaj go, a potem poprowadź dolnym torem przez lampy A i B do przystanku.",
    "Bogdan is stranded in the tram. Cross both power contacts on the upper route, call him, then lead him along the lower track through lamps A and B to the stop."}}};
  if(mission==1)return {{0,{"Zabierz termometr. Ze skrzynką możesz skakać, ale nie robić zrywu. Porównaj odczyt przy ciepłej rurze pośrodku drogi i zanieś wynik Witkowi do śluzy.",
    "Take the thermometer. You can jump with the case, but cannot dash. Compare the reading at the warm pipe halfway along, then bring the result to Witek at the airlock."}}};
  return {{1,{"Ta pompa grzeje morze zamiast mieszkań. Manometry przy wejściu pokażą, jak ją bezpiecznie uruchomić. Potem wybierz właściwy odpływ. Znak domu prowadzi do ludzi.",
    "This pump heats the sea instead of the homes. Read the entry gauges to start it safely, then choose the right outlet. The house symbol leads to the residents."}}};
}
inline void place_dock_worker(EngineGrid &g,double x,double floor,int id) {
  g.interactions.push_back({{x,floor-2.4,2,2.4},InteractionKind::DockWorker,id,
    {"Porozmawiaj z dokerem","Talk to the dockworker"}});
}
inline void decorate_harbor_room(EngineGrid &g,const Pattern &spec) {
  if(!harbor_v4_room(g.template_id))return;
  // Roofs belong to solid buildings. Where a route turns back below a roof,
  // preserve a full jump-height service gallery instead of filling it in.
  for(const auto &p:spec.path)if(p.kind=='p') {
    int bottom=62;
    for(const auto &lower:spec.path)
      if(lower.y>p.y&&lower.x<p.x+p.w&&lower.x+lower.w>p.x)
        bottom=std::min(bottom,lower.y-7);
    if(bottom>p.y+2)g.add_solid_rect(p.x+2,p.y+2,std::max(1,p.w-4),bottom-p.y-2);
  }
  if(g.template_id==504) {
    for(size_t i=1;i+1<spec.path.size();i+=2) {
      const auto &left=spec.path[i-1], &ferry=spec.path[i], &right=spec.path[i+1];
      const double shore=left.x+left.w, surface=ferry.y+2.6;
      g.water_basins.push_back({shore,surface,right.x-shore,62-surface});
    }
  }
  if(g.template_id==503) {
    for(size_t i=1;i+1<spec.path.size();++i) {
      auto pad=spec.path[i];
      g.sorting_belts.push_back({pad.x+2.,double(pad.y),pad.w-4.,i%2?5.:-4.});
    }
  }
  int item=harbor_mail_item(g.template_id);
  if(item>=0) {
    auto p=spec.optional.back();
    g.interactions.push_back({{p.x+2.,p.y-3.,3,3},InteractionKind::Task,0,
      {"Zabierz listy do skrzynki przy wyjściu","Take the letters to the exit postbox"},item});
  }
  for(size_t i=3;i<spec.path.size();i+=3) {
    const auto &pad=spec.path[i];
    if(pad.kind=='p')place_dock_worker(g,pad.x+4.,pad.y,int(i));
  }
  // Small inhabited alcoves below the roofline are visual; all collision
  // remains the bright building silhouette. Broad roofs provide safe pauses.
  if(g.template_id==505) {
    for(int i:{1,2}){auto p=spec.path[size_t(i)];g.add_solid_rect(p.x,0,p.w,p.y-7);}
  }
}

inline EngineGrid build_harbor_settlement_v4() {
  EngineGrid g(126,64);g.adventure_level=true;g.event_level=false;g.district=0;
  g.template_id=600;g.room_role=RoomRole::Settlement;g.room_name=Districts[0].name;
  // A continuous inhabited quay with actual shop and post-office roofs.
  g.add_solid_rect(0,48,126,16);
  g.add_solid_rect(20,43,16,5);g.add_solid_rect(40,39,18,9);g.add_solid_rect(58,43,24,5);
  g.add_solid_rect(17,46,6,2);g.add_solid_rect(35,43,8,5);
  g.spawn_x=6;g.spawn_y=48-kPlayerHeight;g.goal={119,44,3,4};
  g.checkpoint_points={{6,g.spawn_y}};
  // Paired slings on one drum. Both can be boarded from the quay;
  // missing the aerial transfer lands the courier on the market floor.
  g.platforms.push_back({{83,40,8,.5},0,6.5,6,.25});
  g.platforms.push_back({{96,40,8,.5},0,-6.5,6,.25});
  g.sweets.push_back({93,30});
  g.springs.push_back({17,45.7,1.5,.3});
  g.interactions.push_back({{11,44,4,4},InteractionKind::Npc,0,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{44,35,4,4},InteractionKind::Npc,6,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{110,44,4,4},InteractionKind::Board,0,{"Tablica wypraw","Expedition board"}});
  g.interactions.push_back({{27,40,3,3},InteractionKind::Secret,0,{"",""}});
  place_dock_worker(g,94,48,0);
  return g;
}
inline EngineGrid build_harbor_lesson_v4() {
  EngineGrid g(198,64);g.adventure_level=true;g.event_level=false;g.district=0;
  g.template_id=601;g.room_role=RoomRole::Lesson;
  g.room_name={"Szkoła na nabrzeżu","Lessons on the quay"};
  const std::vector<Pad> path{{3,55,28},{36,52,24},{68,49,24},{100,46,24},{132,43,24},{164,40,28}};
  for(size_t i=0;i<path.size();++i) {
    auto p=path[i];
    if(i==3) {
      // This berth spans the channel between two fixed quay walls.
      // The short landing remains reachable if the player misses the ferry.
      g.add_solid_rect(p.x+p.w-4,p.y,4,62-p.y);
      g.platforms.push_back({{104,47,12,.6},8,0,6.8,.75});
    } else {
      g.add_solid_rect(p.x,p.y,p.w,2);
      g.add_solid_rect(p.x+2,p.y+2,p.w-4,62-p.y-2);
    }
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    if(i%2==0)g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});
    if(i)g.sweets.push_back({p.x+p.w*.5,p.y-2.});
  }
  g.water_basins.push_back({92,49.6,28,12.4});
  g.spawn_x=5;g.spawn_y=55-kPlayerHeight;g.goal={186,36,3,4};
  g.interactions.push_back({{8,51,4,4},InteractionKind::Note,0,{"Przeczytaj tabliczkę","Read sign"}});
  g.interactions.push_back({{43,48,4,4},InteractionKind::Note,1,{"Przeczytaj tabliczkę","Read sign"}});
  g.interactions.push_back({{76,45,4,4},InteractionKind::Note,2,{"Przeczytaj tabliczkę","Read sign"}});
  g.platforms.push_back({{75,40,9,.5},0,3,3.2,0});
  g.add_solid_rect(91,29,2,12);g.refills.push_back({88,35});
  g.add_solid_rect(95,30,12,2);g.sweets.push_back({103,28});
  g.interactions.push_back({{98,27,3,3},InteractionKind::Secret,1,{"",""}});
  g.add_hazard_rect(0,62,g.width(),2);
  return g;
}

inline EngineGrid build_harbor_mission_v4(int mission) {
  EngineGrid g(mission==0?166:mission==1?206:216,64);
  g.adventure_level=true;g.event_level=false;g.district=0;
  g.room_role=RoomRole::Interlude;g.template_id=310+mission;
  const Text titles[]={
    {"Bogdan: droga powrotna","Bogdan: the way home"},
    {"Pomiar dla mieszkańców","A reading for the residents"},
    {"Nie spuszczaj ciepła do morza","Don't drain the heat into the sea"}
  };
  g.room_name=titles[mission];
  std::vector<Pad> path;
  if(mission==0)path={{3,22,22},{32,26,18},{57,30,18},{82,34,18},{107,38,18},{134,42,23},
    {112,47,19},{87,51,19},{62,55,19},{37,52,19},{12,48,19}};
  else if(mission==1)path={{3,53,23},{31,50,20},{56,47,20},{81,44,23},{109,41,20},
    {134,38,21},{160,35,20},{185,32,18}};
  else path={{3,55,26},{37,51,22},{67,47,24},{99,43,23},{130,39,24},{162,35,23},{193,31,20}};
  for(size_t i=0;i<path.size();++i) {
    const auto p=path[i];g.add_solid_rect(p.x,p.y,p.w,2);
    int bottom=62;
    for(auto q:path)if(q.y>p.y&&q.x<p.x+p.w&&q.x+q.w>p.x)bottom=std::min(bottom,q.y-7);
    if(bottom>p.y+2)g.add_solid_rect(p.x+3,p.y+2,p.w-6,bottom-p.y-2);
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    if(mission!=0||i==0||i==5||i==8)g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});
    if(i%3==1)g.sweets.push_back({p.x+p.w*.5,p.y-2.2});
  }
  g.spawn_x=5;g.spawn_y=path.front().y-kPlayerHeight;
  const auto last=path.back();g.goal={last.x+last.w-4.,last.y-4.,3,4};
  g.add_hazard_rect(0,62,g.width(),2);
  if(mission==0) {
    g.interactions.push_back({{143,38,4,4},InteractionKind::Rescue,2,{"Zawołaj Bogdana","Call Bogdan"}});
    g.goal_requires=2;
  } else if(mission==1) {
    g.interactions.push_back({{8,49,3,4},InteractionKind::Carry,0,{"Weź termometr","Take the thermometer"}});
    g.interactions.push_back({{91,40,4,4},InteractionKind::Valve,2,{"Porównaj odczyt z ciepłą rurą","Compare the reading at the warm pipe"}});
    g.interactions.push_back({{194,28,3,4},InteractionKind::Deliver,1,{"Przekaż wynik i termometr","Deliver the reading and thermometer"}});
    g.goal_requires=2;g.dash_lock=1;g.dash_unlock=2;
    // The upper maintenance ledges are optional. They are reachable without
    // dashing, but the broad sheltered route remains the reliable delivery.
    g.add_solid_rect(64,39,11,2);g.add_solid_rect(82,33,12,2);
    g.sweets.push_back({87,31});
  } else {
    g.interactions.push_back({{12,51,4,4},InteractionKind::Note,40,{"Sprawdź manometry","Read the pressure gauges"}});
    g.interactions.push_back({{45,47,4,4},InteractionKind::Valve,1,{"Otwórz obejście","Open the bypass"}});
    g.interactions.push_back({{76,43,4,4},InteractionKind::Valve,0,{"Włącz pompę","Start the pump"}});
    g.interactions.push_back({{140,35,4,4},InteractionKind::Valve,2,{"Skieruj wodę do mieszkań","Route water to the homes"}});
    g.interactions.push_back({{173,31,4,4},InteractionKind::Valve,3,{"Zrzut do morza","Discharge to the sea"}});
    g.goal_requires=7;
    g.gates.push_back({{64,38,2,10},2});
    g.growth.push_back({{119,41,11,1},7,1.});
  }
  return g;
}

inline Text harbor_objective(int id,uint64_t flags) {
  if(id==310) {
    if(!(flags&1))return {"Stań na pierwszym styku na pomoście zasilania.","Stand on the first contact on the power gangway."};
    if(!(flags&4))return {"Dociśnij drugi styk przy torze tramwaju.","Step on the second contact beside the tram track."};
    if(!(flags&8))return {"Zawołaj Bogdana z kabiny tramwaju: E.","Call Bogdan from the tram cab: E."};
    if(!(flags&16))return {"Poprowadź tramwaj dolnym pomostem do lampy A.","Guide the tram along the lower boardwalk to lamp A."};
    if(!(flags&32))return {"Dalej dolną drogą do lampy B; Bogdan jedzie za tobą.","Follow the lower route to lamp B; Bogdan is following."};
    if(!(flags&2))return {"Doprowadź Bogdana do zadaszonego przystanku.","Lead Bogdan to the sheltered stop."};
    return {"Bogdan jest bezpieczny. Wyjście przy przystanku.","Bogdan is safe. The exit is beside the stop."};
  }
  if(id==311) {
    if(!(flags&1))return {"Zabierz termometr przy wejściu: E.","Collect the thermometer by the entrance: E."};
    if(!(flags&4))return {"Bez zrywu dotrzyj do ciepłej rury i porównaj odczyt: E.","Reach the warm pipe without dashing and compare its reading: E."};
    if(!(flags&2))return {"Dostarcz termometr i prawdziwy odczyt do śluzy pocztowej.","Deliver the thermometer and true reading to the postal airlock."};
    return {"Witek ma odczyt. Zryw znów działa; ruszaj do wyjścia.","Witek has the reading. Dash is restored; head for the exit."};
  }
  if(id==312) {
    if(!(flags&16))return {"Odczytaj manometry przy wejściu, zanim uruchomisz pompę.","Read the entry gauges before starting the pump."};
    if(!(flags&2))return {"Otwórz obejście, aby zdjąć ciśnienie z pompy.","Open the bypass to release pressure from the pump."};
    if(!(flags&1))return {"Przejdź przez otwartą zasuwę i uruchom pompę.","Cross the released shutter and start the pump."};
    if(!(flags&4))return {"Skieruj ciepłą wodę do mieszkań, nie do morskiego zrzutu.","Route the warm water to the homes, not the sea outlet."};
    return {"Mieszkania znów mają ciepło. Przejdź przez most serwisowy.","The homes have heat again. Cross the service bridge."};
  }
  if(harbor_mail_item(id)>=0&&(flags&(uint64_t(1)<<20)))
    return {"Listy są w torbie. Zanieś je do skrzynki przy wyjściu.","The letters are in your bag. Take them to the exit postbox."};
  return {"",""};
}

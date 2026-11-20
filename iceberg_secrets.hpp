#pragma once
// Deliberately absent from the public journal, route map, objectives, and counters.
// Entry fixtures are authored locations, independent of expedition randomness.
namespace iceworld {
inline EngineGrid build_hidden(int id) {
  if(id==iceberg::NightPlatform) {
    using iceberg::SideStory;
    EngineGrid g(94,61);
    g.adventure_level=true;g.event_level=false;g.room_role=iceberg::RoomRole::Secret;
    g.template_id=iceberg::NightPlatformTemplate;g.district=3;
    g.room_name={"Peron 11","Platform 11"};
    g.add_solid_rect(0,48,94,13);
    g.add_solid_rect(0,27,2,21);g.add_solid_rect(92,27,2,21);
    g.add_solid_rect(2,27,90,2);
    g.spawn_x=8;g.spawn_y=48-kPlayerHeight;g.checkpoint_points={{8,g.spawn_y}};
    g.goal={-20,-20,1,1};
    g.interactions={
      {{4,44,3,4},InteractionKind::Return,0,{"Wróć do Archiwum","Return to the Archive"}},
      {{12,44,3,4},InteractionKind::SideStory,int(SideStory::PlatformBoard),{"Przeczytaj tablicę odjazdów","Read the departures board"}},
      {{34,44,4,4},InteractionKind::SideStory,int(SideStory::PlatformBench),{"Usiądź na ławce","Sit on the bench"}},
      {{iceberg::PlatformSignalX,44,3,4},InteractionKind::SideStory,int(SideStory::PlatformSignal),
        {"Nadaj sygnał odjazdu","Give the departure signal"}}};
    return g;
  }
  if(id==iceberg::UnnumberedFloor) {
    using iceberg::SideStory;
    EngineGrid g(146,64);
    g.adventure_level=true;g.event_level=false;g.room_role=iceberg::RoomRole::Secret;
    g.template_id=200+id;g.district=2;
    g.room_name={"Poniżej parteru","Below the ground floor"};
    g.add_solid_rect(0,45,30,19);
    g.add_solid_rect(40,48,25,16);
    g.add_solid_rect(77,51,30,13);
    g.add_solid_rect(117,51,29,13);
    g.add_solid_rect(0,20,2,25);g.add_solid_rect(144,20,2,31);
    g.add_hazard_rect(30,60,10,4);g.add_hazard_rect(65,60,12,4);g.add_hazard_rect(107,60,10,4);
    g.water_basins={{30,58,10,6},{65,57,12,7},{107,56,10,8}};
    g.platforms.push_back({.base={69,50,5,.6},.dy=2,.period=4.8,.phase=.25,.service_lift=true});
    g.pulses.push_back({{87,47,6,4},4.4,.55,PulseKind::Arc,iceberg::BasementCircuitCut});
    g.spawn_x=7;g.spawn_y=45-kPlayerHeight;
    g.checkpoint_points={{7,g.spawn_y},{80,51-kPlayerHeight}};
    g.goal={-20,-20,1,1};
    g.interactions={
      {{4,41,4,4},InteractionKind::Return,0,{"Wróć na klatkę","Return to the stairwell"}},
      {{19,41,4,4},InteractionKind::SideStory,int(SideStory::Intercom),{"Podnieś słuchawkę","Lift the receiver"}},
      {{51,44,4,4},InteractionKind::SideStory,int(SideStory::Tape),{"Uruchom magnetofon","Play the tape"}},
      {{81,47,3,4},InteractionKind::SideStory,int(SideStory::BasementBreaker),{"Przełącz odłącznik","Toggle the breaker"}},
      {{99,47,4,4},InteractionKind::SideStory,int(SideStory::NightWeighbridge),{"Obejrzyj wagę peronową","Inspect the platform scale"}},
      {{119,47,2,4},InteractionKind::SideStory,int(SideStory::NightWindow),{"Spójrz w szybę","Look through the window"}},
      {{126,47,4,4},InteractionKind::SideStory,int(SideStory::Roster),{"Skopiuj listę","Make a copy"}},
      {{137,47,4,4},InteractionKind::Return,0,{"Wróć szybem serwisowym","Return by the service shaft"}}};
    return g;
  }
  EngineGrid g(78,58);
  g.adventure_level=true;g.event_level=false;g.room_role=iceberg::RoomRole::Secret;
  g.template_id=200+id;g.district=0;
  g.room_name=id==0?iceberg::Text{"Korytarz techniczny","Service corridor"}:
    id==1?iceberg::Text{"Linia wewnętrzna","Internal line"}:iceberg::Text{"Zbiornik wyrównawczy","Equalization reservoir"};
  if(id==2) {
    g.add_solid_rect(0,48,24,10);
    g.add_solid_rect(48,42,30,16);
    g.add_hazard_rect(24,56,24,2);
    g.water_basins.push_back({24,50,24,8});
  } else g.add_solid_rect(0,48,78,10);
  g.add_solid_rect(0,20,2,28);g.add_solid_rect(76,20,2,28);
  g.spawn_x=6;g.spawn_y=48-kPlayerHeight;g.checkpoint_points={{6,g.spawn_y}};
  g.goal={-20,-20,1,1};
  g.interactions.push_back({{4,44,4,4},InteractionKind::Return,0,{"Wróć","Return"}});
  if(id==0) {
    for(int x:{22,34,46,58})g.add_solid_rect(x,38,8,1);
    g.interactions.push_back({{64,44,4,4},InteractionKind::Note,10,{"Podnieś kartkę","Pick up paper"}});
  } else if(id==1) {
    g.interactions.push_back({{48,44,4,4},InteractionKind::Note,11,{"Naciśnij przycisk","Press button"}});
  } else {
    g.platforms.push_back({.base={31,45,9,.5},.dy=4,.period=5.2,.phase=.25,.service_lift=true});
    g.interactions.push_back({{56,38,5,4},InteractionKind::Note,12,{"Otwórz protokół","Open the log"}});
  }
  return g;
}
}

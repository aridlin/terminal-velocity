#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>

void check_harbor(bool good,const char *message) {
  if(!good)throw std::runtime_error(message);
  std::cout<<"PASS "<<message<<'\n';
}
void read_harbor(AdventureSession &s) {
  while(s.state.adventure.dialogue.active())s.dialogue_next();
}
void walk_harbor(AdventureSession &s,Rect target,bool grounded=false) {
  auto path=find_motion_path(s.grid,s.state,target,[&](const GameState &state){return !grounded||state.player.grounded;});
  if(!path.reached)throw std::runtime_error("No ordinary path in harbor "+std::to_string(s.grid.template_id)+" to "+std::to_string(target.x)+","+std::to_string(target.y));
  int deaths=s.state.deaths;
  for(auto c:path.actions)s.tick(c,.1);
  if(s.state.deaths!=deaths||(!s.state.won&&!s.state.player.rect().intersects(target)))throw std::runtime_error("Harbor input replay diverged");
}
void land_harbor(AdventureSession &s,size_t index) {
  auto [x,y]=s.grid.route_points.at(index);
  walk_harbor(s,{x-3,y-kPlayerHeight-.1,6,kPlayerHeight+.2},true);
}
void use_harbor(AdventureSession &s,InteractionKind kind,int id=-1) {
  for(auto node:s.grid.interactions)if(node.kind==kind&&(id<0||node.id==id)) {
    auto r=node.area;r.x-=1.5;r.y-=.9;r.w+=3;r.h+=1.8;
    walk_harbor(s,r);Controls c;c.interact=true;s.tick(c,1./120);read_harbor(s);return;
  }
  throw std::runtime_error("Harbor interaction missing");
}
void start_harbor_mission(AdventureSession &s,int mission) {
  s.begin_campaign(true);s.stage=2+mission*2;s.room=7;s.profile.interlude=true;s.load_room();s.state.menu=false;s.persist();
}
int main() {
  try {
    auto file=std::filesystem::temp_directory_path()/"terminal-velocity-harbor-test.dat";
    std::filesystem::remove(file);std::filesystem::remove(file.string()+".bak");
    iceberg::SessionOptions options;options.mute=true;options.seed=73;options.seed_override=true;
    AdventureSession s(options,file);
    auto sorting=iceworld::build_room(503,73,0);
    check_harbor(sorting.sorting_belts.size()==5,"the sorting office has five powered treads");
    for(size_t i=0;i<sorting.sorting_belts.size();++i) {
      const auto &belt=sorting.sorting_belts[i];
      check_harbor((belt.speed>0)==(i%2==0),"neighboring sorting treads run in opposite directions");
      auto rider=new_state(sorting);rider.player.x=belt.x+belt.width*.5;
      rider.player.y=belt.y-kPlayerHeight;rider.player.grounded=true;
      double x=rider.player.x;
      for(int tick=0;tick<48;++tick)world_step(rider,sorting,{},1./120);
      check_harbor(std::abs(rider.player.x-x-belt.speed*.4)<.02,"sorting tread carries a standing courier at the speed of its parcels");
      Controls leap;leap.jump_pressed=leap.jump_held=true;world_step(rider,sorting,leap,1./120);
      check_harbor(rider.player.vy<0&&rider.player.vx*belt.speed>0,"leaving the belt transfers its direction into the jump");
    }
    auto quay=iceworld::build_harbor_settlement_v4();
    auto riding=new_state(quay);auto sling=quay.platforms.front().at(0);
    riding.player.x=sling.x+3;riding.player.y=sling.y-kPlayerHeight;
    for(int tick=0;tick<360;++tick)world_step(riding,quay,{},1./120);
    check_harbor(riding.player.support==0&&sling.y-riding.player.rect().bottom()>12,"the quay sling lifts a rider to the high transfer");
    auto boiler=s.campaign_authored(1,false);auto plume=boiler.air_fields.front();
    auto rising=new_state(boiler);rising.elapsed=.6;rising.player.x=plume.area.x+1;
    rising.player.y=plume.area.bottom()-3;double y=rising.player.y;
    for(int tick=0;tick<96;++tick)world_step(rising,boiler,{},1./120);
    check_harbor(y-rising.player.y>7,"the pressure plume lifts an airborne courier without a jump or dash");
    for(int id:{18,22}) {
      auto chimney=verified_room(id,73,1);
      add_expedition_steam(chimney);
      check_harbor(chimney.air_fields.size()==1&&verify_route(chimney).complete,
        "the Heatworks expedition has an optional steam chute and a verified through-route");
      const auto &jet=chimney.air_fields[0];
      auto pressure=new_state(chimney),idle=pressure;
      pressure.elapsed=jet.phase==0?1.2:3.6;
      idle.elapsed=pressure.elapsed+2.4;
      pressure.player.x=idle.player.x=jet.area.x+.7;
      pressure.player.y=idle.player.y=jet.area.bottom()-4.;
      for(int tick=0;tick<48;++tick){
        world_step(pressure,chimney,{},1./120);
        world_step(idle,chimney,{},1./120);
      }
      check_harbor(pressure.player.y+4<idle.player.y,"full pressure lifts the courier faster than the quiet cycle");
    }
    start_harbor_mission(s,0);
    auto rescue=s.grid.interactions.front();s.harbor_interact(rescue);
    check_harbor(!(s.grid.switch_flags&8),"Bogdan cannot depart before the physical power contacts");
    for(size_t i=1;i<=5;++i)land_harbor(s,i);
    check_harbor((s.grid.switch_flags&5)==5,"grounded traversal closes both contacts without repeated E interactions");
    use_harbor(s,InteractionKind::Rescue);
    check_harbor((s.grid.switch_flags&8)&&!(s.grid.switch_flags&2),"calling Bogdan starts the escort without completing the rescue");
    for(size_t i=6;i<=8;++i)land_harbor(s,i);
    s.persist();
    AdventureSession rescued(options,file);rescued.begin_campaign(false);
    check_harbor(rescued.grid.template_id==310&&(rescued.grid.switch_flags&56)==56,
      "escort reload restores the actual route contacts and reached lamps");
    for(size_t i=9;i<rescued.grid.route_points.size();++i)land_harbor(rescued,i);
    walk_harbor(rescued,rescued.grid.goal);
    check_harbor(rescued.state.won&&(rescued.grid.switch_flags&2),"rescue completes only after guiding Bogdan to the shelter");

    start_harbor_mission(s,1);use_harbor(s,InteractionKind::Carry);
    Controls dash;dash.move=1;dash.dash_pressed=true;s.tick(dash,1./120);
    check_harbor(s.grid.dash_locked()&&s.state.player.dash_time_left==0,"carried thermometer enforces the shared no-dash constraint");
    for(size_t i=1;i<=3;++i)land_harbor(s,i);
    use_harbor(s,InteractionKind::Valve,2);s.persist();
    AdventureSession measured(options,file);measured.begin_campaign(false);
    check_harbor(measured.grid.template_id==311&&(measured.grid.switch_flags&5)==5&&measured.grid.dash_locked(),
      "measured parcel and movement restriction survive save reload");
    for(size_t i=4;i<measured.grid.route_points.size();++i)land_harbor(measured,i);
    use_harbor(measured,InteractionKind::Deliver);walk_harbor(measured,measured.grid.goal);
    check_harbor(measured.state.won&&!measured.grid.dash_locked(),"ordinary-input measured delivery restores dash and opens the exit");

    start_harbor_mission(s,2);
    auto pump=*std::find_if(s.grid.interactions.begin(),s.grid.interactions.end(),[](auto &n){return n.kind==InteractionKind::Valve&&n.id==0;});
    s.harbor_interact(pump);
    check_harbor(!(s.grid.switch_flags&1),"starting a pump without gauge evidence is rejected with feedback");
    use_harbor(s,InteractionKind::Note,40);
    s.harbor_interact(pump);check_harbor(!(s.grid.switch_flags&1),"known pressure still requires the bypass before the pump");
    land_harbor(s,1);use_harbor(s,InteractionKind::Valve,1);
    check_harbor(!s.grid.solid_at(64,43),"opening the bypass physically releases the service shutter");
    land_harbor(s,2);use_harbor(s,InteractionKind::Valve,0);
    land_harbor(s,3);land_harbor(s,4);land_harbor(s,5);use_harbor(s,InteractionKind::Valve,3);
    check_harbor((s.grid.switch_flags&8)&&!(s.grid.switch_flags&4)&&!s.state.won,"sea outlet has a distinct recoverable consequence and cannot satisfy the homes");
    land_harbor(s,4);use_harbor(s,InteractionKind::Valve,2);
    check_harbor((s.grid.switch_flags&7)==7&&!(s.grid.switch_flags&8),"returning to the house valve recovers the heat-routing choice");
    land_harbor(s,5);land_harbor(s,6);walk_harbor(s,s.grid.goal);
    check_harbor(s.state.won,"resolved hydraulic puzzle has an ordinary-input route to its exit");

    s.begin_campaign(true);s.stage=2;s.room=0;s.profile.quests[0]=1;
    s.profile.itinerary=iceworld::make_itinerary(73,0);s.profile.itinerary[0]=500;s.load_room();s.state.menu=false;
    use_harbor(s,InteractionKind::Task,0);
    check_harbor(s.profile.quest_counts[0]==0&&(s.grid.switch_flags&(uint64_t(1)<<20)),"recovering a mailbag does not count as delivering it");
    s.persist();AdventureSession mail(options,file);mail.begin_campaign(false);
    check_harbor(mail.grid.template_id==500&&(mail.grid.switch_flags&(uint64_t(1)<<20))&&mail.profile.quest_counts[0]==0,
      "undelivered letters remain in the bag after a real save reload");
    walk_harbor(mail,mail.grid.goal);
    check_harbor(mail.profile.quest_counts[0]==1&&(mail.profile.task_items[0]&1),"crossing the actual destination dispatch delivers one mailbag");
    mail.harbor_step(mail.state.player,mail.state.deaths);
    check_harbor(mail.profile.quest_counts[0]==1,"delivery cannot be counted twice");

    s.begin_campaign(true);s.state.menu=false;
    check_harbor(s.grid.template_id==600,"new campaigns begin on the inhabited architectural quay");
    auto worker=std::find_if(s.grid.interactions.begin(),s.grid.interactions.end(),
      [](const Interaction &node){return node.kind==InteractionKind::DockWorker;});
    check_harbor(worker!=s.grid.interactions.end(),"the quay dockworker has a real conversation target");
    auto talk_area=worker->area;
    talk_area.x-=1.4;talk_area.w+=2.8;
    walk_harbor(s,talk_area,true);
    s.profile.language=iceberg::Language::English;
    Controls chat;chat.interact=true;
    auto elapsed=s.state.elapsed;
    auto story=s.profile.story;
    s.tick(chat,.1);
    auto first=s.state.adventure.notice;
    check_harbor(first.starts_with("Dockworker: ")&&!s.state.adventure.dialogue.active()&&s.state.elapsed>elapsed&&s.profile.story==story,
      "dockworker conversation shows one line while play continues");
    s.tick(chat,.1);
    check_harbor(s.state.adventure.notice.starts_with("Dockworker: ")&&s.state.adventure.notice!=first,
      "a repeat conversation draws a different line");
    s.profile.language=iceberg::Language::Polish;
    s.tick(chat,.1);
    check_harbor(s.state.adventure.notice.starts_with("Doker: "),"dockworker banter follows the game language");
    s.begin_campaign(true);
    s.state.menu=false;
    use_harbor(s,InteractionKind::Npc,0);
    use_harbor(s,InteractionKind::Secret,0);
    check_harbor(s.profile.hidden_room==0,"the original first secret remains reachable through real hub movement");
    use_harbor(s,InteractionKind::Note,10);use_harbor(s,InteractionKind::Return);
    use_harbor(s,InteractionKind::Board,0);
    check_harbor(s.grid.template_id==601,"the quay board opens the authored rooftop lesson");
    use_harbor(s,InteractionKind::Secret,1);
    check_harbor(s.profile.hidden_room==1,"the earned second secret remains reachable on the new lesson route");
    std::filesystem::remove(file);std::filesystem::remove(file.string()+".bak");
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

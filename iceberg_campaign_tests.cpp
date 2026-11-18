#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
// This is the critical-path run: accept no resident tasks, find no secrets,
// and solve only the machinery/deliveries the main journey actually requires.
// Search edges are ordinary production controls, never position changes.
struct RequiredJourney {
  AdventureSession &game;
  void read() {
    while(game.state.adventure.dialogue.active()){Controls c;c.confirm=true;game.tick(c,1./120);}
  }
  void walk(Rect target,bool grounded=false) {
    auto path=find_motion_path(game.grid,game.state,target,[&](const GameState &s){return !grounded||s.player.grounded;});
    if(!path.reached)throw std::runtime_error("No required input path in template "+std::to_string(game.grid.template_id));
    int deaths=game.state.deaths;
    for(auto c:path.actions)game.tick(c,.1);
    if(game.state.deaths!=deaths||(!game.state.won&&!game.state.player.rect().intersects(target)))
      throw std::runtime_error("Required input path replay diverged");
  }
  void use(InteractionKind kind,int id=-1) {
    for(auto node:game.grid.interactions)if(node.kind==kind&&(id<0||node.id==id)) {
      auto target=node.area;target.x-=1.5;target.y-=.9;target.w+=3;target.h+=1.8;
      walk(target);Controls c;c.interact=true;game.tick(c,1./120);read();return;
    }
    throw std::runtime_error("Required interaction missing");
  }
  void landing(size_t index) {
    auto [x,y]=game.grid.route_points.at(index);
    walk({x-3,y-kPlayerHeight-.1,6,kPlayerHeight+.2},true);
  }
  void settlement() {
    if(!(game.profile.story&(uint64_t(1)<<game.district)))use(InteractionKind::Npc);
    for(auto node:game.grid.interactions)
      if(node.kind==InteractionKind::Epilogue&&!(game.grid.switch_flags&(uint64_t(1)<<node.id)))
        use(InteractionKind::Epilogue,node.id);
    use(InteractionKind::Board);
  }
  void harbor_mission() {
    if(game.grid.template_id==310) {
      for(size_t i=1;i<=5;++i)landing(i);
      use(InteractionKind::Rescue);
      for(size_t i=6;i<game.grid.route_points.size();++i)landing(i);
    } else if(game.grid.template_id==311) {
      use(InteractionKind::Carry);
      for(size_t i=1;i<=3;++i)landing(i);
      use(InteractionKind::Valve,2);
      for(size_t i=4;i<game.grid.route_points.size();++i)landing(i);
      use(InteractionKind::Deliver);
    } else {
      use(InteractionKind::Note,40);
      landing(1);use(InteractionKind::Valve,1);
      landing(2);use(InteractionKind::Valve,0);
      landing(3);landing(4);use(InteractionKind::Valve,2);
      for(size_t i=5;i<game.grid.route_points.size();++i)landing(i);
    }
    walk(game.grid.goal);
    if(!game.state.won)throw std::runtime_error("Required harbor exit remains locked");
  }
};
int main() {
  try {
  iceberg::SessionOptions options;options.no_save=true;options.mute=true;options.seed=73;
  AdventureSession game(options,std::filesystem::temp_directory_path()/"tv-campaign-no-save");
  game.begin_campaign(true);
  RoutePilot pilot;RequiredJourney required{game};std::string last;
  int rooms=0,previous_district=0;
  for(int tick=0;tick<120*60*60&&!game.profile.finished&&!game.profile.at_content_boundary;++tick) {
    std::string identity=std::to_string(game.district)+":"+std::to_string(game.stage)+":"+std::to_string(game.room)+":"+std::to_string(game.profile.interlude);
    if(identity!=last){pilot={};pilot.predictive=game.grid.verification_policy;last=identity;++rooms;}
    Controls c;
    if(game.state.adventure.dialogue.active())c.confirm=true;
    else if(game.state.won)c.confirm=true;
    else if(game.grid.room_role==iceberg::RoomRole::Settlement) {required.settlement();continue;}
    else if(iceworld::harbor_v4_mission(game.grid.template_id)) {required.harbor_mission();continue;}
    else if(game.grid.template_id==iceberg::CrownSentinel) {play_crown_sentinel(game);continue;}
    else {
      c=pilot.controls(game.grid,game.state);
      int at=game.near_interaction();
      if(at>=0&&(game.grid.interactions[size_t(at)].kind==InteractionKind::Valve||game.grid.interactions[size_t(at)].kind==InteractionKind::Rescue||game.grid.interactions[size_t(at)].kind==InteractionKind::Carry||game.grid.interactions[size_t(at)].kind==InteractionKind::Deliver||game.grid.interactions[size_t(at)].kind==InteractionKind::Evidence))c.interact=true;
    }
    game.tick(c,1./120);
    if(game.district!=previous_district||game.profile.finished||game.profile.at_content_boundary) {
      auto &r=game.profile.records[size_t(previous_district)];
      std::cout<<"DISTRICT "<<previous_district<<" seconds="<<r.best_seconds<<" deaths="<<r.best_deaths<<" treasures="<<r.best_treasures<<'\n';
      previous_district=game.district;
    }
    if(game.state.deaths>12) {
      std::cerr<<"BLOCKED "<<identity<<" template="<<game.grid.template_id<<" target="<<pilot.target
        <<" xy="<<game.state.player.x<<','<<game.state.player.y<<" switches="<<game.grid.switch_flags<<'\n';return 1;
    }
  }
  for(auto quest:game.profile.quests)if(quest!=0)throw std::runtime_error("Critical-path pilot accepted an optional resident task");
  if(game.profile.discoveries)throw std::runtime_error("Critical-path pilot entered an optional secret");
  std::cout<<"Campaign finished="<<game.profile.finished<<" content_boundary="<<game.profile.at_content_boundary<<" last="<<last<<" target="<<pilot.target<<" xy="<<game.state.player.x<<","<<game.state.player.y<<" rooms="<<rooms<<" active_seconds="<<game.profile.campaign_seconds<<'\n';
  std::cout<<"PASS ending requires no optional resident tasks or secret discoveries\n";
  return game.profile.finished||game.profile.at_content_boundary?0:1;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

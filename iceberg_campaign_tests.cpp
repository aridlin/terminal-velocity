#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
int main() {
  iceberg::SessionOptions options;options.no_save=true;options.mute=true;options.seed=73;
  AdventureSession game(options,std::filesystem::temp_directory_path()/"tv-campaign-no-save");
  game.begin_campaign(true);
  RoutePilot pilot;std::string last;
  double played=0;int rooms=0,previous_district=0;
  for(int tick=0;tick<120*60*60&&!game.profile.finished&&!game.profile.at_content_boundary;++tick) {
    std::string identity=std::to_string(game.district)+":"+std::to_string(game.stage)+":"+std::to_string(game.room)+":"+std::to_string(game.profile.interlude);
    if(identity!=last){pilot={};pilot.predictive=game.grid.verification_policy;last=identity;++rooms;}
    Controls c;
    if(game.state.adventure.dialogue.active())c.confirm=true;
    else if(game.state.won)c.confirm=true;
    else if(game.grid.room_role==iceberg::RoomRole::Settlement) {
      c=game.grid.route_points.empty()?Controls{}:pilot.controls(game.grid,game.state);
      if(game.grid.route_points.empty())c.move=1;
      int at=game.near_interaction();
      if(at>=0) {
        const auto &node=game.grid.interactions[size_t(at)];
        if(node.kind==InteractionKind::Board||(!(game.profile.story&(uint64_t(1)<<game.district))&&node.kind==InteractionKind::Npc))c.interact=true;
      }
    } else {
      c=pilot.controls(game.grid,game.state);
      int at=game.near_interaction();
      if(at>=0&&(game.grid.interactions[size_t(at)].kind==InteractionKind::Valve||game.grid.interactions[size_t(at)].kind==InteractionKind::Rescue||game.grid.interactions[size_t(at)].kind==InteractionKind::Carry||game.grid.interactions[size_t(at)].kind==InteractionKind::Deliver||game.grid.interactions[size_t(at)].kind==InteractionKind::Evidence))c.interact=true;
    }
    game.tick(c,1./120);
    played+=1./120;
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
  std::cout<<"Campaign finished="<<game.profile.finished<<" content_boundary="<<game.profile.at_content_boundary<<" last="<<last<<" target="<<pilot.target<<" xy="<<game.state.player.x<<","<<game.state.player.y<<" rooms="<<rooms<<" simulated_seconds="<<played<<'\n';
  return game.profile.finished||game.profile.at_content_boundary?0:1;
}

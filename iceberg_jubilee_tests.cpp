#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
void check_jubilee(bool yes,const char *what){if(!yes)throw std::runtime_error(what);std::cout<<"PASS "<<what<<'\n';}
Rect near_jubilee(Rect a){a.x-=1.5;a.y-=.9;a.w+=3;a.h+=1.8;return a;}
void walk_jubilee(AdventureSession &game,Rect target) {
  auto path=find_motion_path(game.grid,game.state,target);
  if(!path.reached)throw std::runtime_error("No input path in scene "+std::to_string(game.grid.template_id)+" to "+std::to_string(target.x)+","+std::to_string(target.y));
  for(auto c:path.actions)game.tick(c,.1);
  check_jubilee(game.state.player.rect().intersects(target),"ordinary inputs reach the requested apparatus");
}
void use_jubilee(AdventureSession &game){Controls c;c.interact=true;game.tick(c,1./120);}
void read_jubilee(AdventureSession &game){while(game.state.adventure.dialogue.active()){Controls c;c.confirm=true;game.tick(c,1./120);}}
int main(){try {
  auto dir=std::filesystem::temp_directory_path()/("tv-jubilee-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir);auto file=dir/"profile";
  iceberg::SessionOptions options;options.mute=true;options.seed=73;
  auto game=std::make_unique<AdventureSession>(options,file);game->begin_campaign(true);
  game->district=1;game->stage=3;game->profile.story=3;game->profile.unlocked=1;game->clear_room_record();game->load_room();game->persist();
  auto original_quests=game->profile.quests;
  Interaction door{};for(auto n:game->grid.interactions)if(n.kind==InteractionKind::Replay&&n.id==2)door=n;
  check_jubilee(door.id==2,"optional fountain route is available from the heating works");
  walk_jubilee(*game,near_jubilee(door.area));use_jubilee(*game);read_jubilee(*game);
  check_jubilee(game->grid.template_id==303&&game->profile.revisit.active,"fountain enters its own authored scene");
  // Walking out before solving it must not grant the reward or move the story.
  for(auto n:game->grid.interactions)if(n.kind==InteractionKind::Return)door=n;
  walk_jubilee(*game,near_jubilee(door.area));use_jubilee(*game);
  check_jubilee(!(game->profile.story&(uint64_t(1)<<40))&&game->stage==3,"early exit leaves the optional repair unfinished and keeps the journey");
  for(auto n:game->grid.interactions)if(n.kind==InteractionKind::Replay&&n.id==2)door=n;
  walk_jubilee(*game,near_jubilee(door.area));use_jubilee(*game);read_jubilee(*game);
  for(int valve=0;valve<3;++valve) {
    Interaction target{};for(auto n:game->grid.interactions)if(n.kind==InteractionKind::Valve&&n.id==valve)target=n;
    walk_jubilee(*game,near_jubilee(target.area));use_jubilee(*game);
    check_jubilee(game->grid.switch_flags&(uint64_t(1)<<valve),"repair valve opens through production interaction");
    if(valve==0) {
      game->persist();auto saved=game->state.checkpoint;game.reset();game=std::make_unique<AdventureSession>(options,file);game->begin_campaign(false);
      check_jubilee(game->grid.template_id==303&&(game->grid.switch_flags&1)&&game->state.checkpoint==saved,"save/resume restores the fountain repair and checkpoint");
    }
  }
  walk_jubilee(*game,{137,17.5,7,2.5});
  walk_jubilee(*game,game->grid.goal);
  check_jubilee(game->state.won&&game->grid.switch_flags==7,"all three circuits and the return route are playable");
  Controls c;c.confirm=true;game->tick(c,1./120);read_jubilee(*game);
  check_jubilee((game->profile.story&(uint64_t(1)<<40))&&(game->profile.cosmetics&(1u<<3)),"completion remembers restored heating and unlocks the jacket");
  check_jubilee(!game->profile.revisit.active&&game->stage==3&&game->grid.template_id==101&&game->profile.quests==original_quests,"optional scene returns to the same journey without altering character tasks");
  game.reset();game=std::make_unique<AdventureSession>(options,file);game->begin_campaign(false);
  check_jubilee(game->profile.story&(uint64_t(1)<<40),"completed fountain survives another application start");
  game.reset();std::filesystem::remove_all(dir);return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

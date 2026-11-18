#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
void require_end(bool ok,const char *message){if(!ok)throw std::runtime_error(message);std::cout<<"PASS "<<message<<'\n';}
void read_end(AdventureSession &game){while(game.state.adventure.dialogue.active())game.dialogue_next();}
int main(){try{
 auto folder=std::filesystem::temp_directory_path()/("tv-ending-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(folder);auto file=folder/"campaign";
 iceberg::SessionOptions options;options.mute=true;
 AdventureSession game(options,file);game.begin_campaign(true);
 game.district=4;game.profile.district=4;game.profile.at_content_boundary=true;game.persist();
 game.begin_campaign(false);
 require_end(game.district==5&&!game.profile.at_content_boundary&&game.grid.template_id==105,"Underfrost boundary resumes into the authored Crown without resetting history");
 game.stage=7;game.load_room();game.advance_room();
 require_end(game.grid.template_id==iceberg::CrownSentinel&&game.music_track()==3&&
             (game.profile.story&iceberg::CrownArenaReached),"the Crown machinery leads to the boss arena and remix");
 {
  AdventureSession resume(options,file);resume.begin_campaign(false);
  require_end(resume.grid.template_id==iceberg::CrownSentinel&&resume.state.crown.hits==0,
              "reloading an unfinished Crown fight restores the entrance");
 }
 {
  auto shield=game.state;
  shield.player.x=game.grid.goal.x;shield.player.y=game.grid.goal.bottom()-kPlayerHeight;
  world_step(shield,game.grid,{},1./120);
  require_end(!shield.won,"the arena exit stays sealed until the boss is beaten");
  shield=game.state;shield.player.x=23;shield.crown.time=crown_strike_time;
  world_step(shield,game.grid,{},1./120);
  require_end(shield.deaths==1&&shield.crown.hits==0,"the marked sweep hits a courier who stays on the floor");
 }
 play_crown_sentinel(game);
 require_end(game.state.adventure.dialogue.lines.size()==14&&(game.profile.story&iceberg::CrownSentinelDefeated),
             "three dodges and dash hits open the full Crown cutscene");
 {
  AdventureSession resume(options,file);resume.begin_campaign(false);
  require_end(resume.state.crown.hits==3&&resume.state.adventure.dialogue.active(),
              "reloading after victory reopens the cutscene without refighting");
 }
 read_end(game);
 require_end(game.grid.template_id==330&&!game.profile.finished&&game.profile.records[5].complete,"beacon completion records Crown and starts a playable epilogue before marking the journey finished");
 game.advance_room();require_end(!game.profile.finished&&!game.state.adventure.dialogue.active(),"epilogue cannot finish before planting seeds and delivering the public register");
 auto use=[&](int id){for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Epilogue&&node.id==id){game.state.player.x=node.area.x;game.state.player.y=node.area.y;game.interact();read_end(game);return;}};
 use(0);game.persist();
 require_end(game.grid.switch_flags==1,"seed planting persists independently of optional seed quest progress");
 {AdventureSession resume(options,file);resume.begin_campaign(false);require_end(resume.grid.template_id==330&&resume.grid.switch_flags==1&&!resume.profile.finished,"restart resumes the morning round with planted seeds and pending delivery");}
 use(0);require_end(game.grid.switch_flags==1&&!game.state.adventure.dialogue.active(),"repeated planting cannot duplicate the epilogue action");
 use(1);game.advance_room();read_end(game);
 require_end(game.profile.finished&&game.profile.discoveries==0&&game.profile.quests[5]==0,"main ending needs no hidden discoveries or optional character tasks");
 require_end(game.profile.cosmetics&(1u<<7),"completed journey unlocks the courier reward");
 {AdventureSession resume(options,file);resume.begin_campaign(false);require_end(resume.profile.finished&&resume.grid.template_id==330&&resume.grid.switch_flags==3,"completed epilogue and its world changes survive restart");
  resume.profile.visit_district=2;resume.load_room();require_end(resume.grid.template_id==102,"completed journey can visit restored settlements");
  resume.advance_room();require_end(resume.grid.template_id==330&&resume.grid.switch_flags==3,"settlement return preserves the completed morning round");
  auto &saved=resume.profile.expeditions[0];
  saved.active=true;saved.seed=saved.origin_seed=73;saved.district=3;saved.room=saved.depth=4;
  saved.itinerary=iceworld::make_itinerary(73,3,iceberg::RoomsPerLeg);
  resume.run_district=5;resume.advance_room();
  require_end(resume.mode==iceberg::Mode::Endless&&resume.district==3&&resume.depth==4,
    "the completed journey board resumes the saved expedition in its own district");}
 {
  AdventureSession tour(options,file);
  const auto before=tour.profile.story;
  tour.start_tour(int(iceberg::DebugTour.size())-1);
  require_end(tour.grid.template_id==iceberg::CrownSentinel&&tour.music_track()==3,
              "the Debug Tour opens the Crown Sentinel with the same score");
  play_crown_sentinel(tour);read_end(tour);tour.end_tour();
  require_end(tour.profile.story==before,"debug victory does not change the campaign save");
 }
 std::filesystem::remove_all(folder);return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

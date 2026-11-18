#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
void require_end(bool ok,const char *message){if(!ok)throw std::runtime_error(message);std::cout<<"PASS "<<message<<'\n';}
void read_end(AdventureSession &game){while(game.state.adventure.dialogue.active())game.dialogue_next();}
void read_to_reply(AdventureSession &game) {
  auto &dialogue=game.state.adventure.dialogue;
  while(dialogue.active()&&!dialogue.choosing())game.dialogue_next();
  if(!dialogue.choosing())throw std::runtime_error("expected a reply prompt");
}
int main(){try{
 auto folder=std::filesystem::temp_directory_path()/("tv-ending-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(folder);auto file=folder/"campaign";
 iceberg::SessionOptions options;options.mute=true;
 for(int option=0;option<2;++option) {
  auto witness_file=folder/("witness-"+std::to_string(option));
  AdventureSession witness(options,witness_file);witness.begin_campaign(true);
  witness.stage=3;witness.load_room();witness.persist();
  {
   AdventureSession old(options,witness_file);old.begin_campaign(false);
   require_end(!old.state.adventure.dialogue.active(),"older saves parked in town keep their free movement");
  }
  witness.talk_homecoming(0);witness.persist();
  AdventureSession resumed(options,witness_file);resumed.begin_campaign(false);
  read_to_reply(resumed);
  Controls next;next.navigate=-option;next.jump_pressed=option!=0;resumed.tick(next,1./60);
  require_end(resumed.state.adventure.dialogue.choosing()&&resumed.state.adventure.dialogue.reply_option==option,
              "the up input moves the reply selection without confirming it");
  Controls accept;accept.confirm=true;resumed.tick(accept,1./60);
  const auto &lines=resumed.state.adventure.dialogue.lines;
  const auto &reply=iceberg::civic_reply(0,option);
  require_end(lines.size()==2&&lines.front().speaker==iceberg::CourierSpeaker&&
              std::string_view(lines.front().text.en)==reply.said.en&&
              std::string_view(lines.back().text.pl)==reply.answer.text.pl,
              "the selected response is spoken by the courier and answered by Helena");
  AdventureSession later(options,witness_file);later.begin_campaign(false);later.talk_homecoming(1);
  const auto followup=iceberg::civic_followup(0,option!=0);
  const auto &continued=later.state.adventure.dialogue.lines;
  require_end((later.profile.story&iceberg::reply_recorded(0))&&
              ((later.profile.story&iceberg::reply_value(0))!=0)==(option!=0)&&
              std::any_of(continued.begin(),continued.end(),[&](const auto &line){return std::string_view(line.text.en)==followup.text.en;}),
              "the settlement remembers either reply after restarting");
 }
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
 require_end(game.state.adventure.dialogue.lines.size()>=15&&(game.profile.story&iceberg::CrownSentinelDefeated),
             "three dodges and dash hits open the full Crown cutscene");
 {
  AdventureSession resume(options,file);resume.begin_campaign(false);
  require_end(resume.state.crown.hits==3&&resume.state.adventure.dialogue.active(),
              "reloading after victory reopens the cutscene without refighting");
 }
 read_to_reply(game);
 require_end(game.state.crown.airlock_open,"the upper airlock opens at the cutscene cue");
 Controls next;next.navigate=1;game.tick(next,1./60);
 game.dialogue_next();
 {
  AdventureSession resume(options,file);resume.begin_campaign(false);
  require_end((resume.profile.story&iceberg::reply_recorded(6))&&(resume.profile.story&iceberg::reply_value(6))&&
              resume.state.crown.airlock_open&&resume.state.adventure.dialogue.lines.front().speaker==iceberg::CourierSpeaker,
              "reloading after choosing keeps the chosen Crown reply");
  read_end(resume);
  require_end(resume.grid.template_id==330,"the resumed reply still advances to the epilogue");
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

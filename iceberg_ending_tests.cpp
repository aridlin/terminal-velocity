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
  auto briefing_file=folder/("briefing-"+std::to_string(option));
  AdventureSession briefing(options,briefing_file);briefing.begin_campaign(true);
  briefing.advance_room();
  AdventureSession reading(options,briefing_file);reading.begin_campaign(false);
  require_end(reading.stage==0&&reading.state.adventure.dialogue.active()&&!reading.state.adventure.dialogue.choosing(),
              "reloading an open briefing restores its introduction");
  read_to_reply(reading);
  AdventureSession prompted(options,briefing_file);prompted.begin_campaign(false);
  require_end(prompted.stage==0&&prompted.state.adventure.dialogue.choosing(),
              "reloading at the briefing question restores the reply prompt");
  prompted.state.adventure.dialogue.reply_option=option;
  prompted.dialogue_next();
  AdventureSession resumed(options,briefing_file);resumed.begin_campaign(false);
  const auto &exchange=resumed.state.adventure.dialogue;
  const auto &reply=iceberg::briefing_reply(0,option);
  require_end(resumed.stage==0&&!(resumed.profile.story&1)&&exchange.active()&&!exchange.choosing()&&
              exchange.lines.size()==2&&exchange.lines.front().speaker==iceberg::CourierSpeaker&&
              std::string_view(exchange.lines.front().text.en)==reply.said.en&&
              std::string_view(exchange.lines.back().text.pl)==reply.answer.text.pl,
              "a reload after choosing resumes the briefing reply without advancing the district");
  auto elsewhere_file=folder/("briefing-elsewhere-"+std::to_string(option));
  auto elsewhere_profile=resumed.profile;
  elsewhere_profile.district=elsewhere_profile.unlocked=1;
  require_end(iceberg::SaveStore(elsewhere_file).save(elsewhere_profile),"fixture saves a district reached by an older build");
  AdventureSession elsewhere(options,elsewhere_file);elsewhere.begin_campaign(false);
  require_end(elsewhere.district==1&&!elsewhere.state.adventure.dialogue.active(),
              "an earlier district's unfinished reply does not reopen in a later district");
  read_end(resumed);
  AdventureSession completed(options,briefing_file);completed.begin_campaign(false);
  require_end(completed.stage==0&&(completed.profile.story&1)&&
              !(completed.profile.story&iceberg::BriefingReplyMask)&&!completed.state.adventure.dialogue.active(),
              "finishing the reply records the briefing and clears the pending exchange");
  completed.advance_room();
  require_end(completed.stage==1,"the resumed briefing opens the regular district route");
 }
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
  AdventureSession unfinished(options,witness_file);unfinished.begin_campaign(false);
  const auto &exchange=unfinished.state.adventure.dialogue;
  require_end(exchange.active()&&!exchange.choosing()&&exchange.lines.size()==2&&
              exchange.lines.front().speaker==iceberg::CourierSpeaker&&
              std::string_view(exchange.lines.front().text.en)==reply.said.en&&
              std::string_view(exchange.lines.back().text.en)==reply.answer.text.en,
              "reloading before Helena answers restores the chosen exchange");
  read_end(unfinished);
  AdventureSession later(options,witness_file);later.begin_campaign(false);
  require_end(!later.state.adventure.dialogue.active()&&!(later.profile.story&iceberg::HomecomingReplyPending),
              "acknowledging the reply clears the conversation from the next reload");
  later.talk_homecoming(1);
  const auto followup=iceberg::civic_followup(0,option!=0);
  const auto &continued=later.state.adventure.dialogue.lines;
  require_end((later.profile.story&iceberg::reply_recorded(0))&&
              ((later.profile.story&iceberg::reply_value(0))!=0)==(option!=0)&&
              std::any_of(continued.begin(),continued.end(),[&](const auto &line){return std::string_view(line.text.en)==followup.text.en;}),
              "the settlement remembers either reply after restarting");
 }
 AdventureSession game(options,file);game.begin_campaign(true);
 {
  auto return_file=folder/"second-homecoming";
  AdventureSession returning(options,return_file);returning.begin_campaign(true);
  returning.district=1;returning.stage=4;returning.room=iceberg::RoomsPerLeg-1;
  returning.load_room();returning.advance_room();
  require_end(returning.stage==5&&returning.state.adventure.dialogue.active(),
              "finishing the second route opens the settlement conversation");
  AdventureSession resumed(options,return_file);resumed.begin_campaign(false);
  const auto &dialogue=resumed.state.adventure.dialogue;
  const auto expected=iceberg::homecoming(1,1,resumed.profile.story);
  require_end(resumed.stage==5&&dialogue.active()&&
              std::string_view(dialogue.lines.front().text.en)==expected.front().text.en,
              "reloading restores the second homecoming conversation");
  auto later_file=folder/"later-homecoming";
  auto later_profile=resumed.profile;later_profile.district=later_profile.unlocked=2;
  require_end(iceberg::SaveStore(later_file).save(later_profile),"fixture saves the next district on an older client");
  AdventureSession later(options,later_file);later.begin_campaign(false);
  require_end(later.district==2&&!later.state.adventure.dialogue.active(),
              "a previous district's conversation does not reopen at another settlement");
  read_end(resumed);
  AdventureSession completed(options,return_file);completed.begin_campaign(false);
  require_end(completed.stage==5&&!completed.state.adventure.dialogue.active(),
              "the finished second homecoming returns to ordinary settlement play");
 }
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
 {
  auto follow=game.state;
  follow.crown.hits=2;follow.crown.cycle=2;
  follow.player.x=31.;
  follow.crown.time=crown_follow_warning-1./240;
  step_crown_sentinel(follow,1./120,false);
  require_end(follow.crown.target>31.&&crown_beat(follow.crown)==CrownBeat::Warning&&
              crown_attack_areas(follow.crown)[1].x==27.,
              "the final follow-up retargets and reverses the floor sweep");
  follow.crown.time=crown_follow_strike;
  step_crown_sentinel(follow,1./120,false);
  require_end(follow.deaths==1&&follow.crown.hits==2,"camping through the second strike is fatal");
  follow.player.x=crown_core.x;follow.player.y=crown_core.y;
  follow.crown.time=crown_close_time(2);
  step_crown_sentinel(follow,1./120,true);
  require_end(follow.crown.hits==2,"a late dash cannot hit the closed hatch");
  auto beam=game.state;
  beam.crown.hits=1;beam.crown.target=27;
  beam.adventure.door_left=0.;
  const auto lane=crown_attack_areas(beam.crown).front();
  CellFrame frame;cell_frame_sink=[&](const CellFrame &cells){frame=cells;};
  for(double aspect:{1.5,2.,2.5}) {
   update_camera(beam,game.grid,110,27,aspect,10);
   const int left=int(std::floor((lane.left()-beam.camera_x)/.4));
   const int right=int(std::floor((std::nextafter(lane.right(),lane.left())-beam.camera_x)/.4));
   const int row=4+int(std::floor((lane.top()+3-beam.camera_y)/(.4*aspect)));
   beam.crown.time=crown_warning_time;
   RenderGrid(game.grid,beam,34,110,aspect).draw();
   require_end(frame.at(row).at(left).rgb==0xe4b872&&frame.at(row).at(right).rgb==0xe4b872,
               "warning marks both edges of the damaging beam lane");
   beam.crown.time=crown_strike_time;
   RenderGrid(game.grid,beam,34,110,aspect).draw();
   require_end(std::all_of(frame.at(row).begin()+left,frame.at(row).begin()+right+1,
                          [](const Cell &cell){return cell.rgb==0xf78677;}),
               "the live beam fills the collision lane at the current terminal aspect");
  }
  cell_frame_sink={};
 }
 {
  auto core=game.state;
  core.adventure.door_left=0.;
  core.camera_x=20.2;core.camera_y=41.2;
  CellFrame frame;cell_frame_sink=[&](const CellFrame &cells){frame=cells;};
  for(const auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
   for(double aspect:{1.5,2.,2.5}) {
    const int left=int(std::floor((crown_core.left()-core.camera_x)/.4));
    const int right=int(std::floor((std::nextafter(crown_core.right(),crown_core.left())-core.camera_x)/.4));
    const int top=4+int(std::floor((crown_core.top()-core.camera_y)/(.4*aspect)));
    const int bottom=4+int(std::floor((std::nextafter(crown_core.bottom(),crown_core.top())-core.camera_y)/(.4*aspect)));
    core.crown.time=crown_open_time(0)-.1;
    RenderGrid(game.grid,core,rows,cols,aspect).draw();
    require_end(frame.at(top).at(left).rgb!=0x88e6dc,"the core hatch stays closed during a strike");
    core.crown.time=crown_open_time(0);
    RenderGrid(game.grid,core,rows,cols,aspect).draw();
    require_end(frame.at(top).at(left).glyph=="╭"&&frame.at(bottom).at(right).glyph=="╯"&&
                frame.at(top).at(left).rgb==0x88e6dc&&
                frame.at((top+bottom)/2).at((left+right)/2).glyph=="█",
                "the exposed hatch outlines the actual dash target at each viewport and aspect");
   }
  }
  core.crown.hits=3;
  RenderGrid(game.grid,core,34,110,2.).draw();
  require_end(std::none_of(frame.begin(),frame.end(),[](const auto &row){
   return std::any_of(row.begin(),row.end(),[](const Cell &cell){return cell.rgb==0xd4faf3&&cell.glyph=="█";});
  }),"the core hatch is dark after the last shield breaks");
  cell_frame_sink={};
 }
 play_crown_sentinel(game,1);
 {
  auto phase_file=folder/"crown-phase";
  std::filesystem::copy_file(file,phase_file);
  AdventureSession resume(options,phase_file);resume.begin_campaign(false);
  require_end(resume.state.crown.hits==1&&resume.state.crown.time==0.,
              "a shield break saves immediately and reload starts the new phase at its wind-up");
  const int old_deaths=resume.state.deaths;
  for(int i=0;i<245;++i)resume.tick({},1./120);
  require_end(resume.state.deaths==old_deaths+1&&resume.state.crown.hits==1&&resume.state.crown.time<crown_warning_time,
              "taking a beam hit keeps the shield broken and restarts the wind-up");
  Controls retry;retry.restart=true;resume.tick(retry,1./120);
  require_end(resume.state.deaths==old_deaths+2&&resume.state.crown.hits==1&&resume.state.crown.time==0.,
              "manual Retry keeps the current phase and starts at a clean wind-up");
  play_crown_sentinel(resume,2);
  AdventureSession last_phase(options,phase_file);last_phase.begin_campaign(false);
  require_end(last_phase.state.crown.hits==2&&last_phase.state.crown.time==0.,
              "the last attack pattern resumes after both shield breaks have saved");
  const int last_deaths=last_phase.state.deaths;
  play_crown_sentinel(last_phase);
  require_end(last_phase.state.deaths==last_deaths&&last_phase.state.crown.hits==3&&last_phase.state.adventure.dialogue.active(),
              "ordinary movement and a dash clear the resumed final phase from the entrance");
 }
 play_crown_sentinel(game);
 require_end(game.state.adventure.dialogue.lines.size()>=15&&(game.profile.story&iceberg::CrownSentinelDefeated),
             "the two follow-up strikes and three dash counters lead into the Crown film");
 {
  AdventureSession resume(options,file);resume.begin_campaign(false);
  require_end(resume.state.crown.hits==3&&resume.state.adventure.dialogue.active(),
              "reloading after victory reopens the cutscene without refighting");
 }
 {
  auto &film=game.state.adventure.dialogue;
  require_end(film.cinematic,"the final blow cuts from gameplay to the film");
  std::array<bool,5> shots{};
  for(int frame=0;frame<5000&&!film.choosing();++frame) {
    if(!film.active())throw std::runtime_error("the film must stop at the interactive reply");
    shots[size_t(film.lines[film.page].shot)]=true;
    game.tick({},.1,true,34,24);
  }
  require_end(film.choosing()&&std::all_of(shots.begin(),shots.end(),[](bool seen){return seen;}),
              "five shots advance on their own at the smallest viewport and wait for the courier reply");
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
 while(game.grid.template_id==iceberg::CrownSentinel&&game.state.adventure.dialogue.active())game.dialogue_next();
 require_end(game.grid.template_id==330&&!game.profile.finished&&game.profile.records[5].complete,"beacon completion records Crown and starts a playable epilogue before marking the journey finished");
 {
  AdventureSession resume(options,file);resume.begin_campaign(false);
  const auto &dialogue=resume.state.adventure.dialogue;
  require_end(resume.grid.template_id==330&&dialogue.active()&&
              std::string_view(dialogue.lines.front().text.en)==iceberg::epilogue_arrival().front().text.en,
              "reloading after the Crown restores Helena's instructions for the epilogue");
 }
 for(int option:{0,1}) {
  auto morning_file=folder/("morning-"+std::to_string(option));
  require_end(iceberg::SaveStore(morning_file).save(game.profile),"save the epilogue arrival before its reply");
  AdventureSession morning(options,morning_file);morning.begin_campaign(false);
  read_to_reply(morning);
  AdventureSession prompted(options,morning_file);prompted.begin_campaign(false);
  require_end(prompted.state.adventure.dialogue.choosing()&&
              prompted.state.adventure.dialogue.reply_scene==iceberg::EpilogueReplyScene,
              "the epilogue reply prompt reopens after a reload");
  if(option==1) {
    Controls down;down.navigate=1;prompted.tick(down,1./60);
  }
  Controls accept;accept.confirm=true;prompted.tick(accept,1./60);
  AdventureSession chosen(options,morning_file);chosen.begin_campaign(false);
  const auto &lines=chosen.state.adventure.dialogue.lines;
  const auto &reply=iceberg::epilogue_reply(option);
  require_end(lines.size()==2&&lines.front().speaker==iceberg::CourierSpeaker&&
              std::string_view(lines.front().text.en)==reply.said.en&&
              lines.back().speaker==0&&std::string_view(lines.back().text.pl)==reply.answer.text.pl,
              "the chosen courier question and Helena's answer survive a reload");
  read_end(chosen);
  AdventureSession done(options,morning_file);done.begin_campaign(false);
  require_end(done.grid.template_id==330&&!done.profile.finished&&
              !done.state.adventure.dialogue.active()&&
              !(done.profile.story&(iceberg::ArrivalMask|iceberg::ArrivalExchangeMask)),
              "acknowledgement clears the exchange and leaves both epilogue tasks available");
 }
 read_end(game);
 {
  AdventureSession resume(options,file);resume.begin_campaign(false);
  require_end(resume.grid.template_id==330&&!resume.state.adventure.dialogue.active(),
              "finishing the epilogue arrival keeps the morning round playable after reload");
 }
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
  tour.state.menu=true;
  tour.state.adventure.tour_picker=true;
  Controls finale;finale.browse_leg=1;
  tour.tick(finale,0);
  require_end(tour.state.menu&&iceberg::DebugTour[size_t(tour.state.adventure.picker_choice)].room==iceberg::CrownSentinel,
              "Right selects the Crown finale without starting the tour");
  finale.browse_leg=-1;tour.tick(finale,0);
  require_end(tour.state.adventure.picker_choice==0,"Left returns the picker to the first sample");
  finale.browse_leg=1;finale.confirm=true;tour.tick(finale,0);
  require_end(tour.grid.template_id==iceberg::CrownSentinel&&tour.music_track()==3,
              "the Debug Tour opens the Crown Sentinel with the same score");
  play_crown_sentinel(tour,1);
  Controls retry_tour;retry_tour.restart=true;
  tour.tick(retry_tour,1./120);
  require_end(tour.state.crown.hits==0&&tour.state.crown.time==0&&tour.state.deaths==1,
              "Retry restarts the Crown sample from its first phase");
  play_crown_sentinel(tour);read_end(tour);tour.end_tour();
  require_end(tour.profile.story==before,"debug victory does not change the campaign save");
 }
 std::filesystem::remove_all(folder);return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}

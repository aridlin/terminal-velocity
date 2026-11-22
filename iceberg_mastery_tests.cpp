#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
int main() {
  try {
    {
      const auto file=std::filesystem::temp_directory_path()/("tv-transit-fare-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
      iceberg::SessionOptions options;options.mute=true;
      AdventureSession paid(options,file);
      paid.begin_campaign(true);
      paid.profile.district_treasures=iceberg::MasteryTokenFare-1;
      paid.profile.records[0].best_treasures=iceberg::MasteryTokenFare-2;
      if(paid.trial_unlocked())throw std::runtime_error("a partial replay must not count its prior toll twice");
      paid.grid.sweets={{paid.state.player.x+.3,paid.state.player.y+.5}};
      paid.state.collected.assign(1,false);
      paid.tick({},1./120);
      if(!paid.state.collected.front()||!paid.trial_unlocked()||
        !(paid.profile.cosmetics&iceberg::MasteryTransitPass))
        throw std::runtime_error("collecting the final transit token must grant the pass immediately");
      AdventureSession reopened(options,file);
      reopened.begin_campaign(true);
      reopened.begin_run(iceberg::Mode::Trial);
      if(reopened.mode!=iceberg::Mode::Trial||reopened.state.menu)
        throw std::runtime_error("the transit pass must open the trial after save reload and New Journey");
      std::error_code ignored;
      std::filesystem::remove(file,ignored);
      std::filesystem::remove(file.string()+".bak",ignored);
      std::cout<<"PASS transit token pickup permanently unlocks trial entry without the mastery quest\n";
    }
    {
      const auto folder=std::filesystem::temp_directory_path()/("tv-outfitter-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
      std::filesystem::create_directories(folder);
      const auto file=folder/"campaign";
      iceberg::SessionOptions options;options.mute=true;
      AdventureSession shop(options,file);
      shop.begin_campaign(true);
      const auto stall=std::find_if(shop.grid.interactions.begin(),shop.grid.interactions.end(),[](const auto &node){
        return node.kind==InteractionKind::Outfitter;
      });
      if(stall==shop.grid.interactions.end())throw std::runtime_error("outfitter missing from Lower Harbor");
      shop.state.player.x=stall->area.x+1.;
      shop.state.player.y=stall->area.bottom()-kPlayerHeight;
      shop.state.player.grounded=true;
      const Controls use{.interact=true},confirm{.confirm=true},previous{.navigate=-1};
      shop.profile.district_treasures=2;
      shop.tick(use,1./120);shop.tick(previous,1./120);shop.tick(confirm,1./120);
      if(shop.profile.cosmetics&iceberg::HarborScarf||iceberg::outfitter_balance(shop.profile)!=2)
        throw std::runtime_error("a purchase cannot exceed the token balance");
      shop.tick(Controls{.pause=true},1./120);
      shop.profile.district_treasures=3*iceberg::RoomsPerLeg;
      shop.tick(use,1./120);shop.tick(previous,1./120);shop.tick(confirm,1./120);
      shop.tick(confirm,1./120);shop.tick(previous,1./120);shop.tick(confirm,1./120);
      if(!(shop.profile.cosmetics&iceberg::HarborScarf)||(shop.profile.cosmetics&iceberg::DockBoots)||
         iceberg::outfitter_balance(shop.profile)!=6)
        throw std::runtime_error("three tokens per room afford only the starter gear after a leg");
      shop.tick(Controls{.pause=true},1./120);
      int budget=5;
      for(int i=0;i<iceberg::market_stock_size(0);++i)budget+=iceberg::market_item(0,i).price;
      shop.profile.district_treasures=budget;
      shop.tick(use,1./120);shop.tick(confirm,1./120);
      for(int i=1;i<iceberg::market_stock_size(0);++i) {
        const auto item=iceberg::market_item(0,i);
        const int before=iceberg::outfitter_balance(shop.profile);
        shop.tick(previous,1./120);shop.tick(confirm,1./120);
        if(!iceberg::owns_market_item(shop.profile.cosmetics,shop.profile.task_items[1],0,i)||
           iceberg::outfitter_balance(shop.profile)!=before-item.price)
          throw std::runtime_error("buying gear must debit its price exactly once");
        shop.tick(previous,1./120);shop.tick(confirm,1./120);
        if(iceberg::outfitter_balance(shop.profile)!=before-item.price)
          throw std::runtime_error("an owned item cannot be charged again");
        shop.tick(previous,1./120);shop.tick(confirm,1./120);
      }
      if(shop.state.adventure.dialogue.active()||!shop.trial_unlocked()||!iceberg::survey_pass(shop.profile))
        throw std::runtime_error("shopping must preserve both transit unlocks and provide a way to close the shop");
      AdventureSession reopened(options,file);
      reopened.begin_campaign(true);
      const auto all=iceberg::HarborScarf|iceberg::DockBoots|iceberg::SignalSatchel;
      if((reopened.profile.cosmetics&all)!=all||
         !iceberg::owns_market_item(reopened.profile.cosmetics,reopened.profile.task_items[1],0,4)||
         iceberg::outfitter_balance(reopened.profile)!=5||
         !reopened.trial_unlocked()||!iceberg::survey_pass(reopened.profile))
        throw std::runtime_error("gear and the remaining balance must survive a save reload and a fresh Journey");
      for(int district=1;district<iceberg::DistrictCount;++district) {
        reopened.profile.visit_district=district;
        reopened.load_room(false);
        const auto stall=std::find_if(reopened.grid.interactions.begin(),reopened.grid.interactions.end(),
          [](const auto &node){return node.kind==InteractionKind::Outfitter;});
        if(stall==reopened.grid.interactions.end()||stall->id!=district)
          throw std::runtime_error("district market is missing");
        reopened.state.player.x=stall->area.x+1.;
        reopened.state.player.y=stall->area.bottom()-kPlayerHeight;
        reopened.interact();
        if(reopened.state.adventure.dialogue.outfitter_district!=district||
           reopened.state.adventure.dialogue.outfitter_item!=0)
          throw std::runtime_error("district market cannot be reached or opened");
        reopened.state.adventure.dialogue={};
        for(int item=0;item<2;++item)
          if(iceberg::market_price(district,item)<=iceberg::market_price(district-1,item))
            throw std::runtime_error("market prices must rise with each district");
      }
      std::filesystem::remove_all(folder);
      std::cout<<"PASS outfitter purchases preserve both passes and survive reload without double charges\n";
    }
    {
      const auto folder=std::filesystem::temp_directory_path()/("tv-survey-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
      std::filesystem::create_directories(folder);
      const auto file=folder/"campaign";
      iceberg::SessionOptions options;options.mute=true;
      AdventureSession game(options,file);
      game.begin_campaign(true);
      const auto door=std::find_if(game.grid.interactions.begin(),game.grid.interactions.end(),[](const auto &node) {
        return node.kind==InteractionKind::Replay&&node.id==iceberg::SurveyReplay;
      });
      if(door==game.grid.interactions.end())throw std::runtime_error("survey entrance missing from Harbor");
      const auto entry=door->area;
      game.state.player.x=entry.x;
      game.state.player.y=entry.bottom()-kPlayerHeight;
      game.profile.district_treasures=iceberg::SurveyTokenFare-1;
      game.interact();
      if(game.profile.revisit.active)throw std::runtime_error("survey opens before the token fare is met");
      game.grid.sweets={{game.state.player.x+.3,game.state.player.y+.5}};
      game.state.collected.assign(1,false);
      game.tick({},1./120);
      const auto harbor_time=game.state.elapsed;
      Controls doorway;
      doorway.interact=doorway.jump_held=doorway.jump_pressed=doorway.dash_pressed=true;
      doorway.move=1;
      game.tick(doorway,.1);
      if(game.grid.template_id!=iceberg::BreakwaterSurvey||!(game.profile.cosmetics&iceberg::SurveyTransitPass))
        throw std::runtime_error("the final token should open the survey entrance");
      if(game.state.elapsed!=0.||game.state.player.x!=game.grid.spawn_x||
          game.state.player.y!=game.grid.spawn_y||!game.state.player.can_dash())
        throw std::runtime_error("entering the survey must consume same-frame movement, jump and dash");
      game.tick(doorway,.1);
      if(game.profile.revisit.active||game.state.elapsed!=harbor_time||
          game.state.player.x!=entry.x||game.state.player.y!=entry.bottom()-kPlayerHeight||
          !game.state.player.can_dash())
        throw std::runtime_error("returning from the survey must restore the exact entrance");
      game.tick(doorway,.1);
      AdventureSession resumed(options,file);
      resumed.begin_campaign(false);
      if(resumed.grid.template_id!=iceberg::BreakwaterSurvey||!resumed.profile.revisit.active)
        throw std::runtime_error("a save inside the survey must reopen at its checkpoint");
      int initial_deaths=resumed.state.deaths;
      for(const auto target:{Rect{42,44,8,3},Rect{79,34,6,3},Rect{118,31,3,3}}) {
        const auto path=find_motion_path(resumed.grid,resumed.state,target,
          [](const GameState &s){return s.player.grounded;});
        if(!path.reached)throw std::runtime_error("cannot cross the survey to "+std::to_string(target.x));
        for(const auto &input:path.actions)resumed.tick(input,.1);
      }
      resumed.profile.language=iceberg::Language::English;
      const auto surveyor=resumed.state.player;
      resumed.state.player.x=resumed.grid.goal.x;
      resumed.state.player.y=resumed.grid.goal.bottom()-kPlayerHeight;
      resumed.tick({},0.);
      if(resumed.state.won||resumed.state.adventure.prompt!="Mark buoy 3 at the chart.")
        throw std::runtime_error("the locked return door should point the player to the buoy chart");
      resumed.inspect_nearby();
      if(!resumed.state.adventure.inspecting||resumed.state.adventure.inspect_title!="Return to harbor"||
          resumed.state.adventure.inspect_body.find("chart to your left")==std::string::npos)
        throw std::runtime_error("inspecting the locked return door should explain how to open it");
      resumed.state.adventure.inspecting=false;
      resumed.state.player=surveyor;
      resumed.inspect_nearby();
      if(resumed.state.adventure.inspect_title!="Harbor chart"||
          resumed.state.adventure.inspect_art!=iceberg::InspectionArt::Chart||
          resumed.state.adventure.inspect_body.find("press E at the chart")==std::string::npos)
        throw std::runtime_error("the chart should explain how to record the dark buoy");
      resumed.state.adventure.inspecting=false;
      resumed.interact();
      if(resumed.grid.switch_flags!=1)throw std::runtime_error("the dark buoy must be marked before leaving");
      resumed.inspect_nearby();
      if(resumed.state.adventure.inspect_body.find("already marked")==std::string::npos||
          resumed.state.adventure.inspect_detail!=1)
        throw std::runtime_error("the chart should show that the buoy has been recorded");
      resumed.state.adventure.inspecting=false;
      const auto exit=find_motion_path(resumed.grid,resumed.state,resumed.grid.goal);
      for(const auto &input:exit.actions)resumed.tick(input,.1);
      if(!exit.reached||!resumed.state.won||resumed.state.deaths!=initial_deaths)
        throw std::runtime_error("the survey should be traversable without retries");
      resumed.advance_room();
      if(resumed.profile.revisit.active||std::abs(resumed.state.player.x-entry.x)>.01||
        !(resumed.profile.cosmetics&iceberg::SurveyCourseComplete))
        throw std::runtime_error("the survey should record completion and return to its entrance");
      std::filesystem::remove_all(folder);
      std::cout<<"PASS survey fare, save/resume, platform route, buoy signal and Harbor return\n";
    }
    for(int district=0;district<iceberg::AuthoredDistrictCount;++district) {
      iceberg::SessionOptions options;options.no_save=options.mute=options.seed_override=true;options.seed=73;
      AdventureSession game(options,std::filesystem::temp_directory_path()/"tv-mastery-no-save");
      game.profile.quests[6]=3;game.profile.unlocked=district;game.run_district=district;
      game.begin_run(iceberg::Mode::Trial,true);
      for(int room=0;room<iceberg::RoomsPerLeg-1;++room) {
        RoutePilot pilot;pilot.predictive=game.grid.verification_policy;
        for(int tick=0;tick<120*120&&!game.state.won;++tick)game.tick(pilot.controls(game.grid,game.state),1./120);
        if(!game.state.won)throw std::runtime_error("trial approach failed, district="+std::to_string(district)+" room="+std::to_string(room));
        Controls next;next.confirm=true;game.tick(next,1./120);
      }
      if(game.grid.template_id!=320)throw std::runtime_error("missing mastery finale");
      int dashjumps=0,bounces=0,lifts=0,initial_deaths=game.state.deaths;
      std::vector<Rect> targets{{42,52,9,3},{61,47,2,5},{54,40,6,3},{46,30,7,3},{80,24,3,10},{98,17,8,3},game.grid.goal};
      auto input=[&](Controls c,double dt) {
        auto before=game.state.player;game.tick(c,dt);
        dashjumps+=game.state.player.dash_jump_left>before.dash_jump_left;
        bounces+=game.state.player.wall_bounce_left>before.wall_bounce_left;
        lifts+=before.support==0&&c.jump_pressed&&game.state.player.vy<-23;
      };
      // Exercise the timing deliberately on the first runway. The motion
      // search may now choose a ledge climb while crossing the wider gap.
      for(int n=0;n<30&&(!game.state.player.grounded||!game.state.player.can_dash());++n)
        input({},1./120);
      Controls launch;launch.move=1;launch.dash_pressed=true;
      input(launch,1./120);
      launch.dash_pressed=false;launch.jump_pressed=launch.jump_held=true;
      input(launch,1./120);
      for(int n=0;n<90&&!game.state.player.grounded;++n)input({},1./120);
      for(size_t target=0;target<targets.size();++target) {
        auto path=find_motion_path(game.grid,game.state,targets[target],[&](const GameState &s){
          return target==4?s.player.grounded&&s.player.support==0:target==2?s.player.grounded:true;
        });
        if(!path.reached)throw std::runtime_error("mastery path failed, target="+std::to_string(target));
        for(auto c:path.actions)input(c,.1);
        if(target==2) {
          Controls c;c.move=1;
          for(int n=0;n<120&&wall_dir(game.grid,game.state.player)!=1;++n)input(c,1./120);
          c={};c.aim_y=-1;c.dash_pressed=true;input(c,.1);
          c={};c.jump_pressed=c.jump_held=true;input(c,.05);
        }
        if(target==4) {
          for(int n=0;n<600;++n) {
            const auto &platform=game.grid.platforms[0];
            if(game.state.player.grounded&&game.state.player.support==0&&
              platform.at(game.state.elapsed+.01).y<platform.at(game.state.elapsed).y-.04)break;
            input({},1./120);
          }
          Controls c;c.move=1;c.jump_pressed=c.jump_held=true;input(c,1./120);
        }
      }
      if(!game.state.won||game.state.deaths!=initial_deaths||!dashjumps||!bounces||!lifts)
        throw std::runtime_error("mastery demonstration must combine all three techniques without deaths");
      Controls finish;finish.confirm=true;game.tick(finish,1./120);
      if(!game.state.menu||game.profile.expeditions[1].active||game.profile.challenges.empty()||
        !game.profile.challenges.back().record.complete||!(game.profile.cosmetics&(1u<<(16+district))))
        throw std::runtime_error("mastery trial did not store its independent record and reward");
      std::cout<<"PASS district "<<district<<" full eight-room trial; finale dash_jumps="<<dashjumps
        <<" wall_bounces="<<bounces<<" lift_launches="<<lifts<<"; record and mastery badge saved\n";
    }
    iceberg::SessionOptions options;options.no_save=options.mute=true;
    AdventureSession imported(options,std::filesystem::path{});
    auto &saved=imported.profile.expeditions[1];
    saved.active=true;saved.seed=saved.origin_seed=73;saved.layout_version=2;
    saved.room=saved.depth=7;saved.seconds=84.;saved.deaths=2;saved.treasures=5;
    saved.itinerary=iceworld::make_itinerary(saved.seed,0,iceberg::RoomsPerLeg,2,iceberg::Mode::Trial);
    const auto itinerary=saved.itinerary;
    imported.profile.trials[0]={10.,0,40,true};
    imported.profile.challenges.push_back({73,0,2,{120.,3,4,true}});
    imported.sync_view();
    if(!imported.state.adventure.trial_resume||imported.state.adventure.trials[0].best_seconds!=120.)
      throw std::runtime_error("the imported trial must show its own records before resuming");
    imported.options.seed_override=true;imported.options.seed=74;imported.sync_view();
    if(imported.state.adventure.trial_resume||imported.state.adventure.trials[0].complete)
      throw std::runtime_error("a conflicting explicit seed must not advertise a resume or another route's record");
    imported.options.seed_override=false;
    Controls select;select.select_level=4;select.confirm=true;
    imported.tick(select,1./120);
    if(imported.state.menu||imported.run_seed!=73||imported.room!=7||imported.run_seconds!=84.||
       imported.route_version()!=2||saved.itinerary!=itinerary)
      throw std::runtime_error("the menu must resume the imported trial even without its command-line seed or unlock quest");
    imported.advance_room();imported.sync_view();
    if(imported.profile.trials[0].best_seconds!=10.||imported.state.adventure.trials[0].best_seconds!=84.||
       imported.profile.challenges.front().record.best_treasures!=5)
      throw std::runtime_error("the imported trial must finish into the challenge record, visible in the completion menu");
    const auto completion=std::filesystem::temp_directory_path()/("tv-completed-trial-"+
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!iceberg::SaveStore(completion).save(imported.profile))
      throw std::runtime_error("the completed seeded trial did not save");
    AdventureSession reopened(options,completion);
    std::filesystem::remove(completion);
    Controls show_trial;show_trial.select_level=4;reopened.tick(show_trial,1./120);
    if(!reopened.state.adventure.trial_replay||reopened.state.adventure.trial_resume||
       reopened.state.adventure.trials[0].best_seconds!=84.)
      throw std::runtime_error("after reopening, the menu must offer replay with the original seeded record");
    reopened.options.seed_override=true;reopened.options.seed=74;reopened.sync_view();
    if(reopened.state.adventure.trial_replay)
      throw std::runtime_error("an explicit different seed must not offer to replay the completed route");
    reopened.options.seed_override=false;
    reopened.tick(select,1./120);
    if(reopened.state.menu||reopened.room!=0||reopened.run_seed!=73||reopened.route_version()!=2||
       reopened.run_seconds!=0||reopened.profile.expeditions[1].itinerary!=itinerary)
      throw std::runtime_error("replaying after reopening must preserve the completed route's seed, layout and rooms");
    std::cout<<"PASS the saved completion replays its seeded trial from room one after process restart\n";
    auto gardens_completion=imported.profile;
    auto &gardens_trial=gardens_completion.expeditions[1];
    gardens_trial.district=4;
    gardens_trial.itinerary=iceworld::make_itinerary(73,4,iceberg::RoomsPerLeg,2,iceberg::Mode::Trial);
    gardens_completion.challenges.front().district=4;
    if(!iceberg::SaveStore(completion).save(gardens_completion))
      throw std::runtime_error("the completed Gardens trial did not save");
    AdventureSession gardens_replay(options,completion);
    std::filesystem::remove(completion);
    gardens_replay.tick(select,1./120);
    if(gardens_replay.state.menu||gardens_replay.district!=4||gardens_replay.run_seed!=73||
       gardens_replay.profile.expeditions[1].itinerary!=gardens_trial.itinerary)
      throw std::runtime_error("a completed trial keeps its district even with a fresh campaign");
    Controls retry;retry.confirm=true;imported.tick(retry,1./120);
    if(imported.state.menu||imported.room!=0||imported.run_seed!=73||imported.route_version()!=2||
       imported.run_seconds!=0||saved.itinerary!=itinerary)
      throw std::runtime_error("confirm must retry the same completed challenge from room one");
    std::cout<<"PASS imported trial retains seed, layout, route and separate records through resume, finish and retry\n";
    AdventureSession replacement(options,std::filesystem::path{});
    replacement.profile.unlocked=2;replacement.profile.quests[6]=3;
    auto &endless_save=replacement.profile.expeditions[0];
    auto &trial_save=replacement.profile.expeditions[1];
    endless_save.active=trial_save.active=true;
    endless_save.seed=endless_save.origin_seed=91;endless_save.room=endless_save.depth=4;
    trial_save.seed=trial_save.origin_seed=55;trial_save.room=trial_save.depth=6;
    endless_save.itinerary=iceworld::make_itinerary(91,0,iceberg::RoomsPerLeg);
    trial_save.itinerary=iceworld::make_itinerary(55,0,iceberg::RoomsPerLeg,iceberg::ContentVersion,iceberg::Mode::Trial);
    auto saved_route=endless_save.itinerary;
    const auto revision=replacement.save_revision;
    Controls retry_run;retry_run.select_level=3;retry_run.restart=true;
    replacement.tick(retry_run,1./120);
    replacement.tick(retry,1./120);
    if(!replacement.state.menu||!replacement.state.adventure.confirm_new||endless_save.depth!=4||
       endless_save.itinerary!=saved_route||replacement.save_revision!=revision)
      throw std::runtime_error("Retry and a repeated Enter must each preserve the saved expedition");
    Controls cancel;cancel.pause=cancel.interact=true;
    replacement.tick(cancel,1./120);
    if(replacement.state.adventure.confirm_new||!replacement.state.menu||endless_save.depth!=4)
      throw std::runtime_error("Cancel wins over a simultaneous Replace");
    replacement.tick(retry,1./120);
    if(replacement.state.menu||replacement.depth!=4||replacement.run_seed!=91)
      throw std::runtime_error("the saved expedition must resume after cancellation");
    Controls menu_request;menu_request.menu_request=true;
    replacement.tick(menu_request,1./120);
    Controls district_change;district_change.map=true;
    replacement.tick(district_change,1./120);
    replacement.tick(retry,1./120);
    if(!replacement.state.adventure.confirm_new||endless_save.district!=0||endless_save.depth!=4)
      throw std::runtime_error("choosing a new district must also protect the saved expedition slot");
    Controls replace_run;replace_run.interact=true;
    replacement.tick(replace_run,1./120);
    if(replacement.state.menu||endless_save.district!=1||endless_save.depth!=0||trial_save.depth!=6)
      throw std::runtime_error("accepting replacement must start only the selected expedition slot");
    replacement.tick(menu_request,1./120);
    retry_run={};retry_run.select_level=4;retry_run.new_run=true;
    replacement.tick(retry_run,1./120);
    if(!replacement.state.adventure.confirm_new||trial_save.depth!=6)
      throw std::runtime_error("New run must protect a saved Mastery Trial");
    replacement.tick(replace_run,1./120);
    if(replacement.state.menu||replacement.mode!=iceberg::Mode::Trial||trial_save.depth!=0||endless_save.district!=1)
      throw std::runtime_error("accepting replacement must start only the selected trial slot");
    std::cout<<"PASS saved run replacement needs a separate action and preserves the other slot\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

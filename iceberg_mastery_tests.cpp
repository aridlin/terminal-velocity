#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
int main() {
  try {
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
    Controls retry;retry.confirm=true;imported.tick(retry,1./120);
    if(imported.state.menu||imported.room!=0||imported.run_seed!=73||imported.route_version()!=2||
       imported.run_seconds!=0||saved.itinerary!=itinerary)
      throw std::runtime_error("confirm must retry the same completed challenge from room one");
    std::cout<<"PASS imported trial retains seed, layout, route and separate records through resume, finish and retry\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

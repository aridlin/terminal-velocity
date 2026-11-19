#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void require(bool okay,const char *why){if(!okay)throw std::runtime_error(why);std::cout<<"PASS "<<why<<'\n';}
int main() {
  try {
    for(unsigned seed=0;seed<10000;++seed) {
      auto itinerary=iceworld::make_itinerary(seed,0,24,4);
      for(int leg=0;leg<3;++leg) {
        int main=itinerary[size_t(leg*8+2)],alt=iceworld::harbor_alternate(itinerary,leg,seed);
        if(!(main!=500&&main!=503&&main!=506&&alt!=500&&alt!=503&&alt!=506&&main!=alt&&iceworld::pattern(alt).id==alt))
          throw std::runtime_error("fork replaced a parcel landmark or repeated its room: seed="+std::to_string(seed)+" leg="+std::to_string(leg)+" main="+std::to_string(main)+" alternate="+std::to_string(alt));
      }
    }
    require(true,"10,000 seeded fork graphs preserve landmarks and distinct branches");
    auto path=std::filesystem::temp_directory_path()/"tv-route-state-test.dat";
    std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");
    iceberg::SessionOptions options;options.mute=true;
    AdventureSession s(options,path);s.begin_campaign(true);s.stage=2;s.room=1;s.load_room();
    const auto original=s.profile.itinerary;const auto fork=s.grid;
    auto at=std::find_if(s.grid.interactions.begin(),s.grid.interactions.end(),[](auto &n){return n.kind==InteractionKind::RouteChoice;});
    require(at!=s.grid.interactions.end(),"actual fork doorway exists at the second room");
    auto enter_fork=[](AdventureSession &game) {
      game.state.player.x=game.grid.goal.x-.25;
      game.state.player.y=game.grid.goal.bottom()-kPlayerHeight;
      game.state.won=true;
      game.sync_view();
    };
    const auto choice=*at;
    enter_fork(s);
    require(s.state.adventure.branch_at_door,"the view offers both choices while overlapping the fork and the main exit");
    s.persist();
    AdventureSession waiting(options,path);waiting.begin_campaign(false);
    require(waiting.room==1&&waiting.state.adventure.branch_at_door&&waiting.profile.route_decisions[0]==0,
            "reloading at the split restores both exits without choosing one");
    require(waiting.state.spawn_x==s.state.spawn_x&&waiting.state.spawn_y==s.state.spawn_y&&
            waiting.state.checkpoint==s.state.checkpoint,"the fork resume point preserves the Retry lamp");
    Controls retry;retry.restart=true;
    waiting.tick(retry,1./120);
    AdventureSession after_retry(options,path);after_retry.begin_campaign(false);
    require(after_retry.room==1&&!after_retry.state.adventure.branch_at_door&&
            after_retry.state.player.x==after_retry.state.spawn_x&&after_retry.state.deaths==1,
            "Retry after restoring a fork returns to the lamp and saves that restart");
    Controls use;use.interact=true;
    s.tick(retry,0);
    s.tick(use,1./120);
    require(s.room==1&&!s.state.won&&s.state.deaths==1&&s.profile.route_decisions[0]==0,
      "a buffered retry beats a later Passage B interaction at the same door");
    enter_fork(s);
    auto retry_at_fork=retry;retry_at_fork.interact=true;
    s.tick(retry_at_fork,1./120);
    require(s.room==1&&!s.state.won&&s.state.deaths==2&&s.profile.route_decisions[0]==0,
      "Retry and Use together return to the checkpoint without taking the fork");
    iceberg::Profile checkpoint;
    require(iceberg::SaveStore::validate_file(path,checkpoint)&&checkpoint.room==1&&checkpoint.room_deaths==2&&
      checkpoint.route_decisions[0]==0,"manual retries save the updated count before the next autosave");
    enter_fork(s);s.tick(use,1./120);
    require(s.room==2&&s.grid.template_id==choice.id,"alternate doorway enters its named room");
    require(s.profile.itinerary==original,"branch choice never rewrites the reserved itinerary");
    require(s.profile.route_choices[0]==1&&s.profile.route_decisions[0]==1,"branch decision recorded once");
    auto id=s.grid.template_id;auto seed=s.grid.seed;s.profile.quest_focus=2;s.persist();
    iceberg::Profile persisted;require(iceberg::SaveStore::validate_file(path,persisted),"TVICE6 validates with graph state");
    require(persisted.quest_focus==2,"chosen quest focus survives save/reload");
    require(persisted.route_choices[0]==1&&persisted.layout_version==4&&!(persisted.story&iceberg::ForkResumeMask),
            "choosing a branch saves the room and clears the waiting fork");
    AdventureSession resumed(options,path);resumed.begin_campaign(false);
    require(resumed.grid.template_id==id&&resumed.grid.seed==seed,"resume rebuilds the same alternate room");
    resumed.state.deaths++;resumed.load_room(true);
    require(resumed.grid.template_id==id,"death cannot reroll a chosen branch");
    resumed.advance_room();
    require(resumed.room==3&&resumed.grid.template_id==original[3],"alternate rejoins the actual fourth room");
    require((resumed.profile.route_visited[0]&15)==14,"visited map does not invent visits to skipped cells");
    resumed.profile.hidden_room=0;resumed.load_room();resumed.sync_view();
    require(!resumed.state.adventure.route.available,"secret rooms are absent from the route map");
    AdventureSession direct(options,path);direct.profile.hidden_room=-1;direct.profile.route_choices={};direct.profile.route_decisions={};direct.room=1;direct.stage=2;direct.load_room();
    enter_fork(direct);
    Controls confirm;confirm.confirm=true;direct.tick(confirm,1./120);
    require(direct.grid.template_id==original[2]&&direct.profile.route_decisions[0]==1&&direct.profile.route_choices[0]==0,
      "normal doorway keeps the main branch");
    for(unsigned version:{1u,2u,3u}) {
      auto legacy=iceworld::make_itinerary(73,0,24,version);
      require(std::none_of(legacy.begin(),legacy.end(),[](int v){return v>=500;}),"published generator versions exclude new rooms");
    }
    {
      const std::filesystem::path events_path=path.string()+".events";
      std::filesystem::remove(events_path);std::filesystem::remove(events_path.string()+".bak");
      AdventureSession events(options,events_path);
      events.profile.unlocked=events.run_district=4;
      auto &run=events.profile.expeditions[0];
      run.active=true;run.district=4;run.seed=run.origin_seed=73;
      run.itinerary=iceworld::make_itinerary(run.seed,4,iceberg::RoomsPerLeg);
      run.itinerary.front()=57;
      events.begin_run(iceberg::Mode::Endless);
      const auto [token_x,token_y]=events.grid.sweets.front();
      events.state.player.x=token_x-kPlayerWidth*.5;
      events.state.player.y=token_y-kPlayerHeight*.5;
      events.tick({},1./120);
      iceberg::Profile saved_events;
      require(events.state.sweets_count()==1&&events.state.checkpoint==0&&!events.state.won&&
        iceberg::SaveStore::validate_file(events_path,saved_events)&&
        saved_events.expeditions[0].collected==1&&saved_events.expeditions[0].treasures==1,
        "an Expedition token is saved before checkpoint contact or leaving the room");
      events.state.player.y=events.grid.height()+6;
      events.tick({},1./120);
      require(events.state.deaths==1&&iceberg::SaveStore::validate_file(events_path,saved_events)&&
        saved_events.expeditions[0].deaths==1&&saved_events.expeditions[0].room_deaths==1&&
        saved_events.expeditions[0].collected==1&&saved_events.expeditions[0].treasures==1,
        "an ordinary fall saves both retry counts and keeps the collected token");
      events.profile.quests[6]=1;events.profile.quest_counts[6]=2;
      events.persist();
      for(int i=0;i<24;++i)events.tick({},1./120);
      Controls launch;launch.move=1;launch.dash_pressed=true;
      events.tick(launch,1./120);
      launch.dash_pressed=false;launch.jump_pressed=launch.jump_held=true;
      events.tick(launch,1./120);
      require(events.state.player.dash_jump_left>0&&events.state.checkpoint==0&&!events.state.won&&
        events.state.sweets_count()==1&&events.state.deaths==1&&
        iceberg::SaveStore::validate_file(events_path,saved_events)&&
        saved_events.quest_counts[6]==3&&saved_events.quests[6]==2,
        "Kuba's third dash jump saves both the count and the ready-to-return quest state");
      std::filesystem::remove(events_path);std::filesystem::remove(events_path.string()+".bak");
    }
    options.no_save=true;
    AdventureSession chimney(options,{});
    chimney.profile.unlocked=chimney.run_district=1;
    auto &heat=chimney.profile.expeditions[0];
    heat.active=true;heat.district=1;heat.seed=heat.origin_seed=73;
    heat.itinerary=iceworld::make_itinerary(heat.seed,1,iceberg::RoomsPerLeg);
    heat.itinerary.front()=18;
    chimney.begin_run(iceberg::Mode::Endless);
    const auto chimney_door=chimney.grid.goal;
    require(chimney.grid.template_id==18&&chimney.grid.width()-chimney_door.right()>16,
      "the chimney exit sits on an interior landing");
    chimney.state.player.x=chimney_door.x-1;
    chimney.state.player.y=chimney_door.bottom()-kPlayerHeight;
    chimney.state.player.grounded=true;
    Controls dash;dash.move=1;dash.dash_pressed=true;
    chimney.tick(dash,.1);dash.dash_pressed=false;
    for(int i=0;i<5;++i)chimney.tick(dash,.1);
    require(chimney.state.won&&chimney.state.player.grounded&&
      chimney.state.player.rect().right()<=chimney_door.right()+1.001&&chimney.state.deaths==0,
      "an eastward dash stops at the chimney exit without falling off its landing");
    chimney.tick(confirm,1./120);
    require(chimney.room==1,"the chimney door still advances with a separate confirmation");
    auto westbound=iceworld::build_room(25,73,2);
    const int entry_edge=int(std::ceil(westbound.goal.right()))+1;
    const int landing_y=int(westbound.goal.bottom());
    westbound.protect_exit();
    require(!westbound.solid_at(entry_edge,landing_y-1)&&verify_route(westbound).complete,
      "the westbound stairwell keeps its right-hand approach open");
    AdventureSession archive(options,{});
    archive.begin_campaign(true);archive.profile.unlocked=3;archive.run_district=3;
    auto &copies=archive.profile.expeditions[0];
    copies.active=true;copies.district=3;copies.seed=copies.origin_seed=73;
    copies.itinerary=iceworld::make_itinerary(copies.seed,3,iceberg::RoomsPerLeg);
    copies.itinerary[0]=44;const auto reserved=copies.itinerary;
    copies.layout_version=3;archive.begin_run(iceberg::Mode::Endless);
    require(archive.grid.template_id==44&&archive.grid.sorting_belts.empty(),"older Archive itineraries retain their original footing");
    copies.layout_version=4;archive.load_room(true);
    require(archive.grid.sorting_belts.size()==3&&copies.itinerary==reserved&&verify_route(archive.grid).complete,
      "the Archives add three rideable belts to the saved Copies Conveyor room without changing its itinerary");
    for(const auto &belt:archive.grid.sorting_belts) {
      auto rider=new_state(archive.grid);
      rider.player.x=belt.x+belt.width*.5;rider.player.y=belt.y-kPlayerHeight;rider.player.grounded=true;
      const double x=rider.player.x;
      for(int tick=0;tick<48;++tick)world_step(rider,archive.grid,{},1./120);
      require(std::abs(rider.player.x-x-belt.speed*.4)<.02,"a Copies Conveyor belt carries a standing courier in its indicated direction");
    }
    copies.itinerary[0]=38;copies.layout_version=3;archive.load_room();
    require(archive.grid.template_id==38&&archive.grid.springs.empty(),
      "legacy Pneumatic Post saves keep their original route");
    copies.layout_version=4;archive.load_room();
    require(archive.grid.springs.size()==2&&verify_route(archive.grid).complete,
      "current Pneumatic Post adds two launchers and keeps a verified through-route");
    auto courier=new_state(archive.grid);
    courier.player.x=53.5;courier.player.y=38-kPlayerHeight;courier.player.grounded=true;
    Controls run_right;run_right.move=1;
    for(auto [shelf_x,shelf_y]: {std::pair{66.,32.}, std::pair{82.,28.}}) {
      int tick=0;
      while(tick++<90&&courier.player.vy>=-27)
        world_step(courier,archive.grid,run_right,1./120);
      require(courier.player.vx==28.&&courier.player.vy<-27&&courier.player.dash_available,
        "walking over a pressure nozzle launches diagonally and restores dash");
      for(tick=0;tick<135&&!courier.player.grounded;++tick)
        world_step(courier,archive.grid,{},1./120);
      require(courier.player.grounded&&courier.player.x>=shelf_x-.9&&courier.player.x<shelf_x+8&&
              std::abs(courier.player.rect().bottom()-shelf_y)<.01&&courier.deaths==0,
        "a pressure launch delivers the courier to the next shelf with neutral input");
    }
    AdventureSession estate(options,{});estate.begin_campaign(true);
    estate.profile.unlocked=2;estate.run_district=2;
    auto &stairs=estate.profile.expeditions[0];
    stairs.active=true;stairs.district=2;stairs.seed=stairs.origin_seed=73;
    stairs.itinerary={25,24,26,27,28,29,30,31};
    stairs.layout_version=3;estate.begin_run(iceberg::Mode::Endless);
    require(estate.grid.template_id==25&&!estate.grid.solid_at(59,37)&&!estate.grid.solid_at(72,27),
      "an older stairwell expedition has its original geometry");
    stairs.layout_version=4;estate.load_room();
    require(estate.grid.solid_at(59,37)&&estate.grid.solid_at(72,27)&&verify_route(estate.grid).complete,
      "both stairwell masonry faces are solid and retain the main route");
    require(std::none_of(estate.grid.sweets.begin(),estate.grid.sweets.end(),[&](auto token){
      return estate.grid.solid_at(int(token.first),int(token.second));
    }),"the stairwell walls leave the original tokens clear");
    for(auto [x,y,w,h]:iceworld::EstateStairwellWalls) {
      auto climber=new_state(estate.grid);
      climber.player.x=x-kPlayerWidth;climber.player.y=y+h-5;
      Controls kick;kick.move=1;kick.jump_pressed=kick.jump_held=true;
      world_step(climber,estate.grid,kick,1./120);
      require(climber.player.vx<-15&&climber.player.vy<-20&&climber.player.wall_lock_left>0,
        "the new stairwell face supports a wall jump while holding toward it");
    }
    AdventureSession crown(options,{});
    crown.begin_campaign(true);
    crown.profile.unlocked=crown.run_district=5;
    auto &ridge=crown.profile.expeditions[0];
    ridge.active=true;ridge.district=5;ridge.seed=ridge.origin_seed=73;
    ridge.itinerary={61,60,62,63,64,65,66,67};
    ridge.layout_version=3;
    crown.begin_run(iceberg::Mode::Endless);
    const auto old_shelves=crown.grid.checkpoint_points;
    const auto old_tokens=crown.grid.sweets;
    require(crown.grid.template_id==61&&!crown.grid.crown_seam,
      "a legacy Seam of Light keeps its original geometry");
    ridge.layout_version=4;
    crown.load_room();
    require(crown.grid.crown_seam.has_value()&&crown.grid.checkpoint_points==old_shelves&&
      crown.grid.sweets==old_tokens&&verify_route(crown.grid).complete,
      "the Seam of Light shortcut leaves every old checkpoint, token and the ridge route usable");
    const auto seam=*crown.grid.crown_seam;
    for(int toward:{-1,1}) {
      auto climber=new_state(crown.grid);
      climber.player.x=toward<0?seam.left():seam.right()-kPlayerWidth;
      climber.player.y=seam.y+seam.h*.5;
      Controls kick;kick.move=toward;kick.jump_pressed=kick.jump_held=true;
      world_step(climber,crown.grid,kick,1./120);
      require(climber.player.vx*toward<-15&&climber.player.vy<-20&&climber.player.wall_lock_left>0,
        "both faces of the Crown shaft support a wall kick");
    }
    for(int q:{500,503,506})require(std::find(original.begin(),original.begin()+16,q)!=original.begin()+16,"parcel landmark remains before the last turn-in");
    std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

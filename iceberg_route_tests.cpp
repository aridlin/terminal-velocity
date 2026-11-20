#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void require(bool okay,const char *why){if(!okay)throw std::runtime_error(why);std::cout<<"PASS "<<why<<'\n';}
void test_garden_vine_and_flytrap() {
  constexpr double dt=1./120.;
  auto room=verified_room(48,73,4);
  add_garden_vines_and_traps(room);
  require(room.vines.size()==1&&room.pulses.size()==1,"Garden terrace has a swing vine and a flytrap");
  auto run=new_state(room);
  run.spawn_x=55.4;run.spawn_y=39.-kPlayerHeight;
  reset_player(run);
  for(int i=0;i<4;++i)world_step(run,room,{},dt);
  for(int i=0;i<2;++i)world_step(run,room,{.move=1},dt);
  Controls jump{.move=1,.jump_held=true,.jump_pressed=true};
  for(int i=0;i<23;++i){world_step(run,room,jump,dt);jump.jump_pressed=false;}
  require(run.player.vine==0,"a held jump catches the vine just past the edge of the terrace");
  auto charged=run;
  world_step(charged,room,{.move=1,.dash_pressed=true},dt);
  require(charged.player.vine<0&&!charged.player.dash_available&&charged.player.dash_time_left>0.,
    "a charged dash releases the vine and fires on the same tick");
  auto spent=new_state(room);
  spent.spawn_x=run.spawn_x;spent.spawn_y=run.spawn_y;
  reset_player(spent);
  for(int i=0;i<64;++i)world_step(spent,room,{},dt);
  for(int i=0;i<=25;++i)
    world_step(spent,room,{.move=i==4?0:1,.jump_held=true,.jump_pressed=i==0,.dash_pressed=i==4},dt);
  require(spent.player.vine==0&&!spent.player.dash_available,"an airborne dash can reach the vine with an empty charge");
  world_step(spent,room,{.move=1,.dash_pressed=true},dt);
  require(spent.player.vine==0&&!spent.player.dash_available&&spent.player.dash_time_left==0.,
    "pressing an empty dash preserves the vine grip");
  world_step(spent,room,{.move=1,.jump_held=true,.jump_pressed=true},dt);
  require(spent.player.vine<0&&!spent.player.dash_available&&spent.player.vy<-20.,
    "jump still releases a rider who has no dash charge");
  for(int i=0;i<60;++i)world_step(run,room,{},dt);
  const double momentum=run.player.vx;
  world_step(run,room,{.move=1,.jump_held=true,.jump_pressed=true},dt);
  require(momentum>7.&&run.player.vine<0&&run.player.vx>=momentum-.5&&run.player.vy<-20.,
    "the second jump releases the vine and preserves forward momentum");
  for(int i=0;i<90&&!run.player.grounded;++i)
    world_step(run,room,{.move=run.player.x<67.5?1:-1,.jump_held=true},dt);
  require(run.deaths==0&&run.player.grounded&&run.player.x>=66.&&
    std::abs(run.player.y+kPlayerHeight-34.)<.05,"a swing reaches the high terrace without a dash");
  const auto &trap=room.pulses.front();
  run.player.x=trap.area.x+.25;run.player.y=trap.area.bottom()-kPlayerHeight;
  run.elapsed=trap.period*(1.9-trap.phase);
  world_step(run,room,{},dt);
  require(trap.warning(run.elapsed)&&run.deaths==0,"the flytrap warns before it can hit");
  run.elapsed=trap.period*(1.1-trap.phase);
  world_step(run,room,{},dt);
  require(trap.active(run.elapsed)&&run.deaths==1&&run.player.vine==-1,"the flytrap snaps and retries at the checkpoint");
}

void test_heatworks_route_machinery() {
  constexpr double dt=1./120.;
  iceberg::SessionOptions options;options.no_save=options.mute=true;
  AdventureSession heat(options,{});
  heat.mode=iceberg::Mode::Campaign;heat.district=1;heat.stage=4;heat.room=6;
  heat.profile.layout_version=iceberg::ContentVersion;
  heat.profile.itinerary.assign(iceberg::RoomsPerLeg*3,422);
  heat.load_room();
  require(heat.grid.air_fields.size()==1&&heat.grid.air_fields.front().turbine&&heat.grid.air_fields.front().ax==0.,
    "the upward fan appears in a late Journey room");
  heat.mode=iceberg::Mode::Endless;
  heat.room=7;
  auto &expedition=heat.profile.expeditions[0];
  expedition.itinerary.assign(iceberg::RoomsPerLeg,424);
  expedition.layout_version=iceberg::ContentVersion;
  heat.load_room();
  require(heat.grid.pulses.size()==2,"two timed vents appear late in the Expedition leg");
  const auto &vent=heat.grid.pulses.front();
  auto run=new_state(heat.grid);
  run.player.x=vent.area.x+.2;run.player.y=vent.area.bottom()-kPlayerHeight;
  run.elapsed=vent.period*(1.9-vent.phase);
  world_step(run,heat.grid,{},dt);
  require(vent.warning(run.elapsed)&&run.deaths==0,"the service vent warns before doing damage");
  run.elapsed=vent.period*(1.1-vent.phase);
  world_step(run,heat.grid,{},dt);
  require(vent.active(run.elapsed)&&run.deaths==1,"the active service vent sends the courier to the lamp");
  auto fan=verified_room(422,73,1);
  add_heatworks_return_fan(fan);
  run=new_state(fan);
  run.spawn_x=111.;run.spawn_y=39.-kPlayerHeight;
  reset_player(run);
  for(int i=0;i<51;++i)world_step(run,fan,{.move=1},dt);
  for(int i=0;i<98;++i)world_step(run,fan,{},dt);
  require(run.player.y<27.&&run.player.x>=116.75,"the fan carries a walking player above the maintenance shelf");
  for(int i=0;i<150&&!run.player.grounded&&!run.deaths;++i)
    world_step(run,fan,{.move=run.player.x<121.3?1:run.player.x>121.7?-1:0},dt);
  require(run.deaths==0&&run.player.grounded&&run.player.x>=119.&&
    std::abs(run.player.y+kPlayerHeight-23.)<.05,"ordinary steering reaches the upper fan catwalk without a dash");
}

void test_estate_balcony_and_masonry() {
  constexpr double dt=1./120.;
  auto room=verified_room(27,73,2);
  AdventureSession::apply_estate_mechanics(room);
  require(room.platforms.size()==1&&room.platforms.front().estate_balcony,
    "the Estate street leads to an upper balcony on a cable");
  require(room.pulses.size()==1&&room.pulses.front().kind==PulseKind::Masonry,
    "loose masonry threatens the raised route");
  auto run=new_state(room);
  run.spawn_x=34.;run.spawn_y=38.-kPlayerHeight;
  reset_player(run);
  for(int i=0;i<2;++i)world_step(run,room,{},dt);
  for(int i=0;i<110&&run.player.support!=0;++i) {
    const int steer=run.player.x<37.15?1:run.player.x>37.65?-1:0;
    world_step(run,room,{.move=steer,.jump_held=true,.jump_pressed=i==0},dt);
  }
  require(run.deaths==0&&run.player.support==0,"a normal jump boards the moving balcony");
  for(int i=0;i<230&&run.player.y>30.5;++i)world_step(run,room,{},dt);
  require(run.player.y<30.5&&run.player.support==0&&run.deaths==0,
    "the balcony climbs to the upper floor with a neutral rider");
  const auto &drop=room.pulses.front();
  run=new_state(room);
  run.player.x=drop.area.x+.25;run.player.y=drop.area.bottom()-kPlayerHeight;
  run.elapsed=drop.period*(1.89-drop.phase);
  world_step(run,room,{},dt);
  require(drop.warning(run.elapsed)&&run.deaths==0,"the cornice sheds dust before the masonry falls");
  run.elapsed=drop.period*(1.01-drop.phase);
  world_step(run,room,{},dt);
  require(drop.active(run.elapsed)&&run.deaths==0,"the space below a newly falling block is still clear");
  run.elapsed=drop.period*(1.22-drop.phase);
  world_step(run,room,{},dt);
  require(drop.active(run.elapsed)&&run.deaths==1,"the falling block hits when it reaches the courier");
}

void test_archive_rack_and_scanner() {
  constexpr double dt=1./120.;
  auto room=verified_room(37,73,3);
  AdventureSession::apply_archive_mechanics(room);
  require(room.platforms.size()==1&&room.platforms.front().archive_rack,"the catalogue shaft has working rack transport");
  auto run=new_state(room);
  run.spawn_x=62.;run.spawn_y=48.-kPlayerHeight;
  reset_player(run);
  for(int i=0;i<2;++i)world_step(run,room,{},dt);
  for(int i=0;i<110&&run.player.support!=0;++i) {
    const auto steer=run.player.x<65.9?1:run.player.x>66.5?-1:0;
    world_step(run,room,{.move=steer,.jump_held=true,.jump_pressed=i==0},dt);
  }
  require(run.deaths==0&&run.player.support==0,"a normal jump boards the archive rack");
  for(int i=0;i<260&&run.player.y>41.5;++i)world_step(run,room,{},dt);
  require(run.deaths==0&&run.player.y<41.5&&run.player.support==0,"the archive rack carries a neutral rider to the upper level");

  auto desk=verified_room(40,73,3);
  AdventureSession::apply_archive_mechanics(desk);
  require(desk.pulses.size()==1&&desk.pulses.front().kind==PulseKind::Scanner,"the archive desk is guarded by a scanner");
  const auto &scanner=desk.pulses.front();
  const double left=scanner.period*(1.012-scanner.phase),right=scanner.period*(1.20-scanner.phase);
  require(scanner.damage_area(right).x>scanner.damage_area(left).x+4.,"the damaging strip sweeps from left to right");
  run=new_state(desk);
  run.player.x=scanner.area.x+.16;run.player.y=scanner.area.bottom()-kPlayerHeight;
  run.elapsed=scanner.period*(1.90-scanner.phase);
  world_step(run,desk,{},dt);
  require(scanner.warning(run.elapsed)&&run.deaths==0,"the scanner warning leaves the desk safe");
  run.elapsed=right;
  world_step(run,desk,{},dt);
  require(scanner.active(run.elapsed)&&run.deaths==0,"a beam at the far side does not hit the cleared desk");
  run.elapsed=left;
  world_step(run,desk,{},dt);
  require(scanner.active(run.elapsed)&&run.deaths==1,"only contact with the moving beam triggers a retry");
}

void test_crown_wind_and_rime() {
  constexpr double dt=1./120.;
  auto room=verified_room(64,73,5);
  AdventureSession::apply_crown_mechanics(room);
  require(room.air_fields.size()==1&&room.air_fields.front().reverses&&room.ice_strips.size()==1,
    "the Roofs have a reversing gust and an icy ledge");
  const auto &wind=room.air_fields.front();
  const auto velocity=[&](double phase) {
    auto run=new_state(room);
    run.elapsed=(1.+phase-wind.phase)*wind.period;
    run.player.x=wind.area.x+wind.area.w*.5;
    run.player.y=wind.area.y+1.;
    for(int i=0;i<12;++i)world_step(run,room,{},dt);
    return run.player.vx;
  };
  require(velocity(.25)>0.&&velocity(.75)<0.,"the gust pushes an airborne courier in both directions");
  require(room.platforms.size()==1&&room.platforms.front().rime_shelf,"the icy side passage has a brittle shelf");
  auto run=new_state(room);
  run.spawn_x=58.;run.spawn_y=26.-kPlayerHeight;
  reset_player(run);
  for(int i=0;i<2;++i)world_step(run,room,{},dt);
  for(int i=0;i<120&&run.player.support!=0;++i) {
    const int steer=run.player.x<62.73?1:run.player.x>63.27?-1:0;
    world_step(run,room,{.move=steer,.jump_held=true,.jump_pressed=i==0},dt);
  }
  require(run.deaths==0&&run.player.support==0,"a normal jump reaches the rime shelf in the gust");
  const auto &shelf=room.platforms.front();
  require(shelf.available(run,0,run.elapsed),"the shelf gives the courier time to jump away");
  for(int i=0;i<80&&shelf.available(run,0,run.elapsed);++i) {
    const int steer=run.player.x<62.73?1:run.player.x>63.27?-1:0;
    world_step(run,room,{.move=steer},dt);
  }
  require(!shelf.available(run,0,run.elapsed)&&run.player.support!=0&&run.deaths==0,
    "a held landing breaks the rime and drops the courier");
}

void test_harbor_sling_and_surge() {
  constexpr double dt=1./120.;
  auto dock=verified_room(501,73,0);
  AdventureSession::apply_harbor_mechanics(dock);
  require(dock.platforms.size()==1&&dock.platforms.front().harbor_sling,"a cargo sling connects the dairy's loading ramps");
  auto run=new_state(dock);
  run.spawn_x=45.;run.spawn_y=39.-kPlayerHeight;
  reset_player(run);
  for(int i=0;i<2;++i)world_step(run,dock,{},dt);
  for(int i=0;i<115&&run.player.support!=0;++i) {
    const int steer=run.player.x<48.53?1:run.player.x>49.07?-1:0;
    world_step(run,dock,{.move=steer,.jump_held=true,.jump_pressed=i==0},dt);
  }
  require(run.deaths==0&&run.player.support==0,"a normal jump can reach the cargo sling");
  for(int i=0;i<450&&run.player.y>32.5;++i)world_step(run,dock,{},dt);
  require(run.deaths==0&&run.player.support==0&&run.player.y<=32.5,
    "the cargo sling travels from the lower berth to the upper ramp");

  auto crossing=verified_room(504,73,0);
  AdventureSession::apply_harbor_mechanics(crossing);
  require(crossing.pulses.size()==1&&crossing.pulses.front().kind==PulseKind::Surge&&crossing.water_basins.size()==3,
    "the ferry crossing gains a surge in its existing water channel");
  const auto &wave=crossing.pulses.front();
  const double active=wave.period*(1.12-wave.phase);
  const double warning=wave.period*(1.9-wave.phase);
  require(wave.damage_area(active).top()<wave.area.bottom()-2.,"the surge rises from the water toward the deck");
  run=new_state(crossing);
  run.player.x=wave.area.x+.25;run.player.y=wave.area.y+.25;
  run.elapsed=warning;
  world_step(run,crossing,{},dt);
  require(wave.warning(run.elapsed)&&run.deaths==0,"the narrow channel stays clear during the foam warning");
  run.elapsed=active;
  world_step(run,crossing,{},dt);
  require(wave.active(run.elapsed)&&run.deaths==1,"an active crest can catch a low jump near the quay");
}

void test_expedition_door_resume() {
  using iceberg::Mode;
  const auto path=std::filesystem::temp_directory_path()/"tv-expedition-door-test";
  std::filesystem::create_directories(path);
  for(auto mode:{Mode::Endless,Mode::Trial}) {
    const auto file=path/(mode==Mode::Endless?"endless":"trial");
    iceberg::SessionOptions options;options.mute=true;
    AdventureSession run(options,file);
    run.begin_campaign(true);
    run.profile.cosmetics|=iceberg::MasteryTransitPass;
    run.begin_run(mode,true);
    run.room=run.depth=iceberg::RoomsPerLeg-1;
    run.load_room();
    const auto original_itinerary=run.profile.expeditions[mode==Mode::Endless?0:1].itinerary;
    const auto lamp=std::pair{run.state.spawn_x,run.state.spawn_y};
    run.state.player.x=run.grid.goal.x+(run.grid.goal.w-kPlayerWidth)*.5;
    run.state.player.y=run.grid.goal.bottom()-kPlayerHeight;
    run.tick({},1./120.);
    require(run.state.won,"the last door is playable before the restart");
    AdventureSession loaded(options,file);
    loaded.begin_run(mode);
    require(loaded.room==iceberg::RoomsPerLeg-1&&loaded.state.won&&
      loaded.grid.touches_exit(loaded.state.player.rect()),
      "a saved door is immediately ready when the Expedition resumes");
    require(std::pair{loaded.state.spawn_x,loaded.state.spawn_y}==lamp,
      "resuming at the exit keeps the original retry lamp");
    const auto portable=path/"portable";
    iceberg::Profile imported;
    require(iceberg::SaveStore::write_backup(portable,loaded.profile)&&
      iceberg::SaveStore::read_backup(portable,imported)&&
      imported.expeditions[mode==Mode::Endless?0:1].at_exit,
      "a portable save preserves the ready doorway");

    loaded.tick({.restart=true},1./120.);
    AdventureSession retry(options,file);
    retry.begin_run(mode);
    require(!retry.state.won&&std::pair{retry.state.player.x,retry.state.player.y}==lamp,
      "retry clears the saved doorway and returns to the same lamp");
    retry.state.player.x=retry.grid.goal.x+(retry.grid.goal.w-kPlayerWidth)*.5;
    retry.state.player.y=retry.grid.goal.bottom()-kPlayerHeight;
    retry.tick({},1./120.);
    AdventureSession depart(options,file);
    depart.begin_run(mode);
    depart.tick({.confirm=true},1./120.);
    if(mode==Mode::Endless) {
      const auto &saved=depart.profile.expeditions[0];
      require(saved.room==0&&saved.depth==iceberg::RoomsPerLeg&&saved.completed_legs.size()==1&&
        std::equal(saved.completed_legs.front().begin(),saved.completed_legs.front().end(),original_itinerary.begin()),
        "confirming a restored door enters the next leg and records the old leg once");
    } else require(!depart.profile.expeditions[1].active&&depart.profile.trials[0].complete,
      "confirming the restored Trial door records the result once");
  }
  std::filesystem::remove_all(path);
}

int main() {
  try {
    test_expedition_door_resume();
    test_heatworks_route_machinery();
    test_garden_vine_and_flytrap();
    test_estate_balcony_and_masonry();
    test_archive_rack_and_scanner();
    test_crown_wind_and_rime();
    test_harbor_sling_and_surge();
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
      const auto clock=events.state.elapsed;
      const auto switches=events.grid.switch_flags;
      Controls retry_frame;
      retry_frame.restart=retry_frame.jump_pressed=retry_frame.jump_held=retry_frame.dash_pressed=true;
      retry_frame.move=1;
      events.tick(retry_frame,.1);
      require(events.state.player.x==events.state.spawn_x&&events.state.player.y==events.state.spawn_y&&
        events.state.player.vx==0.&&events.state.player.dash_available&&
        std::abs(events.state.elapsed-clock-1./120.)<1e-8,
        "a slow retry frame stops at the lamp before consuming stale movement");
      require(events.state.deaths==2&&iceberg::SaveStore::validate_file(events_path,saved_events)&&
        saved_events.expeditions[0].deaths==2&&saved_events.expeditions[0].room_deaths==2&&
        saved_events.expeditions[0].collected==1&&saved_events.expeditions[0].treasures==1&&
        saved_events.expeditions[0].switches==switches,
        "a slow retry records one attempt while preserving its token and room switches");
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
    AdventureSession west_exit(options,{});
    west_exit.profile.unlocked=west_exit.run_district=2;
    auto &saved_stairs=west_exit.profile.expeditions[0];
    saved_stairs.active=true;saved_stairs.district=2;saved_stairs.seed=saved_stairs.origin_seed=73;
    saved_stairs.itinerary=iceworld::make_itinerary(saved_stairs.seed,2,iceberg::RoomsPerLeg);
    saved_stairs.itinerary.front()=25;
    west_exit.begin_run(iceberg::Mode::Endless);
    const auto stair_door=west_exit.grid.goal;
    require(west_exit.grid.template_id==25,"the saved Estate run opens the west-facing stairwell door");
    west_exit.state.player.x=stair_door.right()+.4;
    west_exit.state.player.y=stair_door.bottom()-kPlayerHeight;
    west_exit.state.player.grounded=true;
    Controls west_dash;west_dash.move=-1;west_dash.dash_pressed=true;
    west_exit.tick(west_dash,.1);west_dash.dash_pressed=false;
    for(int i=0;i<8;++i)west_exit.tick(west_dash,.1);
    require(west_exit.state.won&&west_exit.state.player.grounded&&west_exit.state.deaths==0&&
      west_exit.state.player.x>=std::floor(west_exit.grid.route_points.back().first)-1.001,
      "a westward dash stays at the door and never falls off the far side");
    west_exit.tick(confirm,1./120);
    require(west_exit.room==1,"the west-facing door still advances on a separate confirmation");
    AdventureSession turbine(options,{});
    const auto fan_stop=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==23;});
    require(fan_stop!=iceberg::DebugTour.end(),"Debug Tour includes the turbine launch shelf");
    turbine.load_tour_scene(int(fan_stop-iceberg::DebugTour.begin()));
    const auto original_turbine=verified_room(23,73,1);
    require(turbine.grid.air_fields.size()==1&&turbine.grid.air_fields.front().turbine&&
      iceworld::has_turbine_access(turbine.grid)&&!iceworld::has_turbine_access(original_turbine)&&
      turbine.grid.sweets==original_turbine.sweets&&turbine.grid.goal.x==original_turbine.goal.x&&
      verify_route(turbine.grid).complete,
      "the Heatworks turbine adds maintenance steps and keeps the ordinary catwalk and tokens");
    for(const double phase:{1.,3.7}) {
      const auto courier_jump=[&](const EngineGrid &room) {
        auto rider=new_state(room);
        rider.player.x=63.7;rider.player.y=32.-kPlayerHeight;
        rider.player.grounded=true;rider.player.vx=kMaxRunSpeed;rider.elapsed=phase;
        Controls launch;launch.move=1;launch.jump_held=launch.jump_pressed=true;
        for(int tick=0;tick<180;++tick) {
          world_step(rider,room,launch,1./120);
          launch.jump_pressed=false;
          if(rider.player.grounded&&tick>16)break;
        }
        return rider.deaths==0&&rider.player.grounded&&
          std::abs(rider.player.rect().bottom()-27.)<.01&&rider.player.x>=74.1&&rider.player.x<83.;
      };
      require(courier_jump(turbine.grid)==(phase==1.)&&!courier_jump(original_turbine),
        "only a timed turbine draft carries an ordinary jump to the upper shelf without a dash");
    }
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
    const auto relay=std::find_if(archive.grid.interactions.begin(),archive.grid.interactions.end(),[](const auto &node) {
      return node.kind==InteractionKind::Valve;
    });
    require(relay!=archive.grid.interactions.end(),"the Copies Conveyor has a relay on fixed footing");
    const auto relay_area=relay->area;
    auto use_relay=[&] {
      archive.state.player.x=relay_area.x;
      archive.state.player.y=relay_area.bottom()-kPlayerHeight;
      Controls use;use.interact=true;archive.tick(use,1./120);
    };
    use_relay();
    require(archive.grid.switch_flags==1&&copies.switches==1&&verify_route(archive.grid).complete,
      "the belt relay reverses the middle belt and persists the setting without blocking the route");
    archive.load_room(true);
    require(archive.grid.switch_flags==1&&archive.grid.sorting_belts[1].speed_at(archive.grid.switch_flags)>0,
      "the Copies Conveyor restores the reversed belt after loading the room");
    auto rider=new_state(archive.grid);
    rider.player.x=51.;rider.player.y=47.-kPlayerHeight;rider.player.grounded=true;
    for(int tick=0;tick<48;++tick)world_step(rider,archive.grid,{},1./120);
    require(std::abs(rider.player.x-52.6)<.02,"the reversed belt physically carries the courier to the right");
    Controls leap;leap.jump_pressed=leap.jump_held=true;
    world_step(rider,archive.grid,leap,1./120);
    require(rider.player.vy<0&&rider.player.vx>0,"the reversed belt carries momentum into the next jump");
    use_relay();
    require(!archive.grid.switch_flags&&!copies.switches&&copies.itinerary==reserved,
      "a second pull restores the original direction without changing the itinerary");
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
    stairs.itinerary[0]=33;
    stairs.layout_version=3;
    estate.load_room();
    const auto original_gutter=estate.grid;
    require(!iceworld::has_estate_gutter(estate.grid),"a legacy Estate gutter keeps its original geometry");
    stairs.layout_version=4;
    estate.load_room();
    require(iceworld::has_estate_gutter(estate.grid)&&estate.grid.sweets==original_gutter.sweets&&
      estate.grid.checkpoint_points==original_gutter.checkpoint_points&&verify_route(estate.grid).complete,
      "the downspouts keep the lower route and existing tokens and checkpoints");
    for(size_t i=0;i<iceworld::EstateGutterWalls.size();++i) {
      const auto [x,y,w,h]=iceworld::EstateGutterWalls[i];
      auto courier=new_state(estate.grid);
      courier.player.x=i?x-kPlayerWidth:x+w;
      courier.player.y=y+h-4;
      Controls kick;kick.move=i?1:-1;kick.jump_pressed=kick.jump_held=true;
      world_step(courier,estate.grid,kick,1./120);
      require(courier.player.wall_lock_left>0&&courier.player.vx*kick.move<-15&&courier.player.vy<-20,
        "the inner face of each downspout supports a wall kick");
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

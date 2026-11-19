#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_validation.hpp"
#include <stdexcept>
#include <set>
void check(bool value,const char *message) {
  if(!value)throw std::runtime_error(message);
  std::cout<<"PASS "<<message<<'\n';
}
int main(int argc,char **argv) {
  try {
    int seeds=argc>1?std::max(1,std::atoi(argv[1])):100;
    for(int seed=0;seed<seeds;++seed) {
      int biome=seed%iceberg::AuthoredDistrictCount;
      auto route=iceworld::make_itinerary(unsigned(seed),biome);
      if(route!=iceworld::make_itinerary(unsigned(seed),biome))throw std::runtime_error("nondeterministic itinerary");
      for(int required:biome==0?std::vector<int>{500,503,506}:biome==1?std::vector<int>{12,13,16,17,20,21}:biome==2?std::vector<int>{24,26,28,30,32,34}:biome==3?std::vector<int>{36,40,44}:biome==4?std::vector<int>{48,52,56}:std::vector<int>{})
        if(std::find(route.begin(),route.begin()+16,required)==route.begin()+16)throw std::runtime_error("task reserved after final settlement visit");
      for(size_t i=1;i<route.size();++i)if(route[i]==route[i-1]||!iceworld::connects(iceworld::pattern(route[i-1]),iceworld::pattern(route[i])))throw std::runtime_error("repeated adjacent room");
    }
    check(true,"structural seed determinism and adjacency checks");
    for(int d=1;d<iceberg::AuthoredDistrictCount;++d)for(int seed=0;seed<128;++seed) {
      auto route=iceworld::make_itinerary(seed,d,iceberg::RoomsPerLeg,iceberg::ContentVersion,iceberg::Mode::Endless);
      auto early=route.begin()+4;
      if(std::none_of(route.begin(),early,[&](int id){return iceworld::has_expedition_mechanic(d,id);})||
        std::set<int>(route.begin(),route.end()).size()!=route.size())
        throw std::runtime_error("early Endless mechanic or unique room missing");
      const auto legacy=iceworld::make_itinerary(seed,d,iceberg::RoomsPerLeg,2);
      const auto trial=iceworld::make_itinerary(seed,d,iceberg::RoomsPerLeg);
      if(legacy!=iceworld::make_itinerary(seed,d,iceberg::RoomsPerLeg,2,iceberg::Mode::Endless)||
        trial!=iceworld::make_itinerary(seed,d,iceberg::RoomsPerLeg,iceberg::ContentVersion,iceberg::Mode::Trial))
        throw std::runtime_error("another mode or layout inherited the Endless room reserve");
    }
    check(true,"new Endless legs include a local mechanic by room four without changing legacy routes or Trials");
    std::set<int> estate_openings,archive_openings;
    for(uint32_t seed=0;seed<128;++seed) {
      const auto estate=iceworld::make_itinerary(seed,2,iceberg::RoomsPerLeg,iceberg::ContentVersion,iceberg::Mode::Endless);
      const auto archive=iceworld::make_itinerary(seed,3,iceberg::RoomsPerLeg,iceberg::ContentVersion,iceberg::Mode::Endless);
      estate_openings.insert(estate.begin(),estate.begin()+4);
      archive_openings.insert(archive.begin(),archive.begin()+4);
    }
    check(estate_openings.count(24)&&estate_openings.count(25)&&
      archive_openings.count(38)&&archive_openings.count(39)&&archive_openings.count(44)&&archive_openings.count(47),
      "district openings can showcase different local mechanics, including newer rooms");
    std::set<int> archive_closers;
    for(uint32_t seed=0;seed<128;++seed) {
      const auto order=iceworld::make_itinerary(seed,3,iceberg::RoomsPerLeg,iceberg::ContentVersion,iceberg::Mode::Endless);
      archive_closers.insert(order.back());
    }
    check(archive_closers.count(43)&&archive_closers.count(40)&&archive_closers.count(44),
      "Archive Endless legs can close with exploration or recovery");
    for(int d=0;d<2;++d) {
      auto old_route=iceworld::make_itinerary(73,d,24,2);
      check(std::all_of(old_route.begin(),old_route.end(),[](int id){return id<24;}),"legacy route version retains the released room library");
      auto new_route=iceworld::make_itinerary(73,d);
      check(std::count_if(new_route.begin(),new_route.end(),[](int id){return id>=400;})>=6,"revised route contains the new traversal structures");
    }

    int failures=0;
    for(const auto &pattern:iceworld::patterns()) {
      auto g=iceworld::build_room(pattern.id,73,iceworld::district_of(pattern));
      for(const auto &pad:pattern.optional)for(int x=pad.x+1;x<pad.x+pad.w-1;++x)
        if(g.solid_at(x,pad.y-1)||g.hazard_at(x,pad.y-1))throw std::runtime_error("blocked optional landing id="+std::to_string(pattern.id));
      auto result=verify_route(g);
      std::cout<<"ROUTE "<<pattern.id<<" complete="<<result.complete<<" deaths="<<result.deaths
        <<" seconds="<<result.ticks/120.<<" target="<<result.target<<" pos="<<result.x<<','<<result.y<<'\n';
      if(!result.complete)++failures;
    }
    check(failures==0,"all room templates traversed with ordinary inputs");
    int traversals=argc>2?std::max(0,std::atoi(argv[2])):0;
    bool expert=argc>3&&std::string(argv[3])=="expert";
    int stable_fallbacks=0,recovery_fallbacks=0;
    for(int n=0;n<traversals;++n) {
      const auto &p=iceworld::patterns()[size_t(n)%iceworld::patterns().size()];
      auto g=verified_room(p.id,iceberg::mix(unsigned(n)),iceworld::district_of(p),expert);
      stable_fallbacks+=g.generation_fallback==1;recovery_fallbacks+=g.generation_fallback==2;
      if(g.generation_fallback)std::cout<<"FALLBACK seed="<<g.seed<<" requested="<<p.id<<" actual="<<g.template_id<<" kind="<<g.generation_fallback<<'\n';
      auto result=verify_route(g);
      if(!result.complete) {
        std::cerr<<"FAILED seed="<<iceberg::mix(unsigned(n))<<" template="<<p.id<<" target="<<result.target<<'\n';++failures;
      }
    }
    std::cout<<"GENERATION routes="<<traversals<<" expert="<<expert<<" stable_fallbacks="<<stable_fallbacks<<" recovery_fallbacks="<<recovery_fallbacks<<'\n';
    check(failures==0,"distributed generated routes traversed with ordinary inputs");
    EngineGrid flat(80,60);flat.adventure_level=true;flat.event_level=false;
    flat.add_solid_rect(0,50,80,10);flat.spawn_x=4;flat.spawn_y=50-kPlayerHeight;flat.goal={75,45,2,5};
    auto objective=flat;objective.goal={4,46,3,4};objective.goal_requires=1;
    auto locked_goal=new_state(objective);FixedStepper objective_step;objective_step.advance(locked_goal,objective,{},1./120);
    check(!locked_goal.won,"climbing over a locked gate cannot bypass its story objective");
    objective.switch_flags=1;objective_step.advance(locked_goal,objective,{},1./120);
    check(locked_goal.won,"completing the objective enables its exit");
    auto parcel=flat;parcel.dash_lock=1;parcel.dash_unlock=2;parcel.switch_flags=1;
    auto courier=new_state(parcel);FixedStepper parcel_step;parcel_step.advance(courier,parcel,{},1./120);
    Controls dash;dash.move=1;dash.dash_pressed=true;parcel_step.advance(courier,parcel,dash,1./120);
    check(courier.player.dash_time_left==0&&courier.player.dash_available,"upright delivery restriction is enforced by shared physics");
    parcel.switch_flags=3;parcel_step.advance(courier,parcel,dash,1./120);
    check(courier.player.dash_time_left>0,"delivery restores the ordinary dash immediately");
    auto jumping=new_state(flat);FixedStepper physics;Controls input;
    physics.advance(jumping,flat,input,1./120);input.move=1;input.dash_pressed=true;
    physics.advance(jumping,flat,input,1./120);input.dash_pressed=false;input.jump_pressed=true;input.jump_held=true;
    physics.advance(jumping,flat,input,1./120);
    check(jumping.player.vx>24&&jumping.player.vy<-19&&jumping.player.dash_jump_left>0,"dash jump preserves horizontal momentum");
    auto wall=flat;wall.add_solid_rect(12,20,2,30);
    auto bouncing=new_state(wall);bouncing.player.x=12-kPlayerWidth-.02;bouncing.player.y=44;
    input={};input.aim_y=-1;input.dash_pressed=true;physics={};
    physics.advance(bouncing,wall,input,1./120);input.dash_pressed=false;input.jump_pressed=true;input.jump_held=true;
    physics.advance(bouncing,wall,input,1./120);
    check(bouncing.player.vx<-18&&bouncing.player.vy<-27&&bouncing.player.wall_bounce_left>0,"upward dash wall bounce reverses horizontal direction");
    EngineGrid lift(80,60);lift.adventure_level=true;lift.event_level=false;lift.spawn_x=6;lift.spawn_y=45-kPlayerHeight;lift.goal={75,40,2,5};
    lift.platforms.push_back({{4,45,10,.5},0,3,3,.5});
    auto rising=new_state(lift);rising.player.support=0;rising.player.grounded=true;
    input={};input.jump_pressed=input.jump_held=true;physics={};physics.advance(rising,lift,input,1./120);
    check(rising.player.vy<-26&&rising.player.momentum_left>0,"rising platform contributes real launch momentum");
    iceberg::SaveStore v4_save("tests/fixtures/iceberg-v4.dat");iceberg::Profile v4;
    check(v4_save.load(v4)&&v4.layout_version==2&&v4.expeditions[0].layout_version==2,
      "released TVICE4 saves retain their campaign and expedition layout versions");
    {
      iceberg::SessionOptions old_options;old_options.mute=old_options.no_save=true;
      AdventureSession old_run(old_options,"tests/fixtures/iceberg-v4.dat");
      old_run.begin_campaign(false);old_run.stage=2;old_run.load_room();
      check(std::all_of(old_run.profile.itinerary.begin(),old_run.profile.itinerary.end(),[](int id){return id<24;}),
        "continuing a released campaign does not adopt revised room geometry");
      old_run.mode=iceberg::Mode::Endless;old_run.run_active=true;old_run.room=7;old_run.district=0;
      old_run.profile.expeditions[0].layout_version=2;old_run.advance_room();
      const auto &bag=old_run.profile.expeditions[0].itinerary;
      check(!bag.empty()&&std::all_of(bag.begin(),bag.end(),[](int id){return id<24;}),
        "legacy endless bag rotation preserves its original generator version");
      old_run.mode=iceberg::Mode::Trial;old_run.run_active=true;old_run.room=7;
      old_run.profile.expeditions[1].layout_version=2;old_run.run_seconds=42;old_run.advance_room();
      check(!old_run.profile.trials[0].complete&&!old_run.profile.challenges.empty()&&old_run.profile.challenges.back().version==2,
        "finishing a resumed legacy trial cannot overwrite revised-course records");
    }
    iceberg::SaveStore old_save("tests/fixtures/iceberg-v2.dat");iceberg::Profile migrated;
    check(old_save.load(migrated)&&migrated.story==291&&migrated.room==4&&migrated.expeditions[0].seed==821,
      "published TVICE2 campaign migrates without losing story or expedition state");
    iceberg::SaveStore v3_save("tests/fixtures/iceberg-v3.dat");iceberg::Profile v3;
    check(v3_save.load(v3)&&v3.challenges.size()==2&&v3.expeditions[0].origin_seed==821&&!v3.revisit.active,
      "TVICE3 preserves challenge records and migrates expedition origin identity");
    iceberg::Record independent;
    AdventureSession::merge_record(independent,50,3,7);
    AdventureSession::merge_record(independent,70,0,12);
    check(independent.best_seconds==50&&independent.best_deaths==0&&independent.best_treasures==12,
      "time, clean completion and collection keep independent personal records");
    auto path=std::filesystem::temp_directory_path()/"terminal-velocity-iceberg-test.dat";
    iceberg::SaveStore save(path);iceberg::Profile profile;
    profile.started=true;profile.seed=73;profile.stage=2;profile.room=4;
    profile.itinerary=iceworld::make_itinerary(73,0);
    profile.story=0x123;profile.quests[0]=2;profile.quest_counts[0]=3;profile.task_items[0]=7;
    profile.checkpoint=2;profile.collected=17;
    profile.revisit={true,0,1,2,3,5,1,42.5};profile.interlude=true;
    profile.challenges.push_back({73,0,iceberg::ContentVersion,{99,0,12,true}});
    profile.challenges.push_back({74,0,iceberg::ContentVersion,{88,1,14,true}});
    auto &exp=profile.expeditions[0];exp.active=true;exp.seed=821;exp.depth=13;exp.room=5;exp.seconds=100;
    exp.itinerary=iceworld::make_itinerary(821,0,iceberg::RoomsPerLeg);exp.checkpoint=1;exp.switches=2;
    profile.smooth_camera=true;
    profile.pause_on_focus_loss=false;
    profile.cosmetics=1u<<3;
    check(save.save(profile),"campaign save written and verified");
    iceberg::Profile loaded;check(save.load(loaded),"campaign save loaded");
    check(loaded.itinerary==profile.itinerary&&loaded.story==profile.story&&loaded.quest_counts[0]==3&&loaded.checkpoint==2,
      "route, story, task and checkpoint survive reload");
    check(loaded.expeditions[0].seed==821&&loaded.expeditions[0].depth==13&&loaded.expeditions[0].itinerary==exp.itinerary,
      "expedition identity and checkpoint stored independently");
    check(loaded.revisit.active&&loaded.revisit.scene==1&&loaded.revisit.seconds==42.5&&loaded.revisit.switches==1&&loaded.interlude,"revisited story route persists its separate checkpoint and machinery");
    check(loaded.challenges.size()==2&&loaded.challenges[0].record.best_seconds==99&&loaded.challenges[1].seed==74,"seeded challenge records stay isolated by route identity");
    check(loaded.smooth_camera&&!loaded.pause_on_focus_loss&&loaded.cosmetics==(1u<<3),
      "camera and focus preferences round-trip independently of outfit unlocks");
    check(!v4.smooth_camera,"older profiles use the standard camera by default");
    auto camera=new_state(flat);
    camera.player.x=32;camera.player.y=35;
    update_camera(camera,flat,34,17,2,10);
    const auto centered=camera;
    auto standard=camera;
    camera.player.x+=.4;standard.player.x+=.4;
    update_camera(camera,flat,34,17,2,1./60,true);
    update_camera(standard,flat,34,17,2,1./60);
    check(camera.camera_x==centered.camera_x&&standard.camera_x>camera.camera_x,
      "smooth camera holds framing for small movements while standard camera follows");
    auto coarse=centered,fine=centered;
    coarse.player.x+=2;fine.player.x+=2;
    update_camera(coarse,flat,34,17,2,.1,true);
    for(int frame=0;frame<12;++frame)update_camera(fine,flat,34,17,2,1./120,true);
    check(std::abs(coarse.camera_x-fine.camera_x)<1e-6&&coarse.camera_x>centered.camera_x,
      "smooth follow uses elapsed time and eases toward the courier");
    camera.player.x+=12;camera.player.y-=9;
    update_camera(camera,flat,34,17,2,1./120,true);
    check(camera.player.x-camera.camera_x<34*.4*.65&&camera.player.y-camera.camera_y>0.,
      "smooth follow bounds the lag during quick moves at compact sizes");
    camera.player.x=4;camera.player.y=50-kPlayerHeight;
    update_camera(camera,flat,34,17,2,10,true);
    check(camera.camera_x<.001,"camera snaps back to the new starting position");
    // A correctly checksummed save from a later release must not be replaced,
    // including when an older healthy backup happens to be available.
    {std::ifstream in(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});
      auto future=bytes;future.replace(0,6,"TVICE9");
      {std::ofstream out(path,std::ios::binary);out<<future;}
      iceberg::SaveStore newer(path);iceberg::Profile ignored;
      newer.load(ignored);
      check(newer.protected_future&&!newer.save(profile),"unknown future format is protected against overwrite");
      {std::ofstream out(path,std::ios::binary);out<<bytes;}
    }
    profile.room=5;check(save.save(profile),"replacement save retains recovery copy");
    auto backup=path;backup+=".bak";
    const auto older=std::filesystem::file_time_type::clock::now()-std::chrono::hours(1);
    std::filesystem::last_write_time(path,older);
    std::filesystem::last_write_time(backup,older);
    const auto primary_time=std::filesystem::last_write_time(path);
    const auto backup_time=std::filesystem::last_write_time(backup);
    check(save.save(profile)&&save.save(profile)&&std::filesystem::last_write_time(path)==primary_time&&
      std::filesystem::last_write_time(backup)==backup_time,
      "identical autosaves leave both the current file and distinct recovery file in place");
    {std::ofstream out(path);out<<"damaged";}
    check(save.load(loaded)&&save.recovered&&loaded.room==4,"damaged primary recovers prior good save");
    const auto archived=std::filesystem::path(path.string()+".unreadable");
    const auto second_archive=std::filesystem::path(archived.string()+".1");
    const auto archived_backup=std::filesystem::path(backup.string()+".unreadable");
    for(const auto &copy:{archived,second_archive,archived_backup})std::filesystem::remove(copy);
    auto file_bytes=[](const auto &file) {
      std::ifstream in(file,std::ios::binary);
      return std::string(std::istreambuf_iterator<char>(in),{});
    };
    check(save.save(loaded)&&file_bytes(archived)=="damaged"&&save.load(loaded)&&!save.recovered&&loaded.room==4,
      "a repaired primary preserves the exact unreadable file");
    {std::ofstream out(path,std::ios::binary);out<<"second damage";}
    check(save.save(loaded)&&file_bytes(archived)=="damaged"&&file_bytes(second_archive)=="second damage",
      "later damage gets a separate preservation copy");
    {std::ofstream out(backup,std::ios::binary);out<<"damaged backup";}
    check(save.save(profile)&&file_bytes(archived_backup)=="damaged backup"&&
        iceberg::SaveStore::validate_file(backup,loaded)&&loaded.room==4,
      "rotating a good primary preserves the unreadable backup before replacing it");
    for(const auto &copy:{path,backup,archived,second_archive,archived_backup})std::filesystem::remove(copy);
    iceberg::SessionOptions options;options.mute=true;options.seed=123;
    {
      AdventureSession session(options,path);session.begin_campaign(true);
      session.profile.story=1;session.stage=2;session.room=3;session.clear_room_record();session.load_room();session.persist();
      auto campaign_template=session.grid.template_id;
      session.begin_run(iceberg::Mode::Endless,true);
      session.room=4;session.depth=4;session.load_room();session.persist();
      auto expedition_template=session.grid.template_id;auto expedition_seed=session.run_seed;
      session.begin_campaign(false);
      check(session.room==3&&session.grid.template_id==campaign_template,"endless trip preserves campaign location");
      session.begin_run(iceberg::Mode::Endless);
      check(session.room==4&&session.depth==4&&session.run_seed==expedition_seed&&session.grid.template_id==expedition_template,
        "endless resume restores the same assembled room");
      session.begin_campaign(false);session.persist();session.profile.hidden_room=0;session.load_room();session.persist();
      check(session.grid.room_role==iceberg::RoomRole::Secret&&session.profile.room==3,"hidden room keeps ordinary route checkpoint intact");
      session.advance_room();
      check(session.profile.hidden_room==-1&&session.grid.template_id==campaign_template&&session.room==3,"hidden exit returns to the original route");
      session.profile.revisit={true,0,0,2,0,0,1,12.};session.load_room(true);session.persist();
      auto replay_template=session.grid.template_id;int replay_checkpoint=session.state.checkpoint;
      session.profile.hidden_room=1;session.load_room();session.persist();session.advance_room();
      check(session.profile.revisit.active&&session.grid.template_id==replay_template&&session.state.checkpoint==replay_checkpoint&&session.grid.switch_flags==1,
        "hidden exit restores the revisited scene checkpoint without overwriting the journey");
      session.advance_room();
      check(!session.profile.revisit.active&&session.grid.template_id==campaign_template&&session.room==3,
        "revisited scene returns to original journey checkpoint");
      session.options.seed_override=true;session.options.seed=73;session.begin_run(iceberg::Mode::Endless,true);
      for(int i=0;i<9;++i)session.advance_room();
      auto continued_seed=session.run_seed;int continued_depth=session.depth;
      session.begin_campaign(false);session.begin_run(iceberg::Mode::Endless);
      check(session.depth==continued_depth&&session.run_seed==continued_seed&&session.profile.expeditions[0].origin_seed==73,
        "seeded endless resume survives rotation into the next room library bag");
      session.profile.unlocked=1;session.run_district=1;session.begin_run(iceberg::Mode::Endless,true);
      session.depth=17;session.room=3;session.load_room();session.persist();session.begin_campaign(false);
      session.state.menu=true;session.state.adventure.selected=0;session.run_district=0;
      Controls choose_endless;choose_endless.select_level=3;choose_endless.confirm=true;session.tick(choose_endless,1./120);
      check(session.district==1&&session.depth==17&&session.room==3,"menu resume selects the saved expedition district rather than replacing the run");
      session.profile.trials[0]={20,0,5,true};session.options.seed=9999;session.sync_view();
      check(!session.state.adventure.endless_resume,"a different explicit seed does not advertise resuming a saved expedition");
      check(!session.state.adventure.trials[0].complete,"new custom seed never displays another route's default record");
      session.begin_campaign(true);
      check(session.stage==0&&session.room==0&&session.profile.story==0,"new story resets progression without copying old room state");
      check(session.profile.seed==9999,"an explicit new campaign seed survives switching from an expedition");
      session.options.seed_override=false;session.begin_campaign(true);
      check(session.profile.seed!=9999,"a fresh unseeded journey assembles a new campaign itinerary");
    }
    {
      auto secondary=iceberg::SaveStore::slot_file(path,2);
      auto tertiary=iceberg::SaveStore::slot_file(path,3);
      AdventureSession session(options,path);
      const unsigned original_seed=session.profile.seed;
      auto press=[&](Controls c){session.tick(c,1./120);};
      auto open_slots=[&](){
        press({.menu_request=true});
        press({.confirm=true,.select_level=5});
        press({.select_level=6});
      };
      open_slots();
      check(session.state.adventure.save_picker&&session.state.adventure.slot_status==std::array{1,0,0},
        "save picker recognizes the existing journey as Slot 1 and fresh slots as empty");
      press({.confirm=true,.select_level=2});
      check(session.save_slot==2&&!session.profile.started,"selecting Slot 2 starts with an independent profile");
      session.begin_campaign(true);
      session.profile.language=iceberg::Language::English;session.persist();
      open_slots();
      press({.confirm=true,.select_level=3});
      check(session.save_slot==3&&!session.profile.started,"Slot 3 is independent of the journey in Slot 2");
      session.persist();
      open_slots();
      press({.confirm=true,.select_level=4});
      check(session.state.adventure.tour_picker,"save picker opens the debug tour");
      const auto bytes=[](const auto &file){
        std::ifstream in(file,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});
      };
      const auto first_save=bytes(path),second_save=bytes(secondary),third_save=bytes(tertiary);
      const auto write_count=session.save_revision;
      session.profile_action_busy=true;
      press({.confirm=true});
      check(session.tour_scene<0,"debug tour waits for an in-flight account action");
      session.profile_action_busy=false;
      press({.confirm=true});
      press({.pause=true});
      press({.navigate=-1});
      check(session.state.paused&&session.tour_scene==int(iceberg::DebugTour.size())-1,
        "paused debug tour previews the previous scene and wraps around");
      press({.navigate=1});
      check(session.state.paused&&session.tour_scene==0,"paused debug tour can preview the next scene");
      press({.confirm=true});
      check(!session.state.paused&&session.tour_scene==0,"resuming plays the previewed scene");
      press({.map=true});
      check(session.state.adventure.map&&session.state.adventure.map_room&&!session.state.adventure.map_route,
        "Debug Tour opens its room map directly");
      press({.inspect=true});
      check(session.state.adventure.map_room,"Inspect keeps the Debug Tour room map in place");
      press({.pause=true});
      check(!session.state.adventure.map&&!session.state.paused,"closing the tour map returns to the same play state");
      press({.pause=true});
      press({.map=true});
      press({.navigate=1});
      check(session.state.paused&&session.state.adventure.map_room&&session.tour_scene==0,
        "navigation in the paused tour map does not select another sample");
      press({.map=true});
      check(session.state.paused&&!session.state.adventure.map,"closing the paused tour map preserves the manual pause");
      press({.confirm=true});
      for(size_t i=0;i<iceberg::DebugTour.size();++i) {
        const auto &stop=iceberg::DebugTour[i];
        check(session.tour_scene==int(i)&&session.grid.template_id==stop.room&&session.grid.district==stop.district,
          "debug tour loads the indicated room and district");
        check(session.state.player.x==stop.entry_x&&
          std::abs(session.state.player.rect().bottom()-stop.entry_floor)<.01,
          "debug tour starts beside the featured room mechanic");
        if(stop.room==33) {
          check(i+2==iceberg::DebugTour.size()&&iceworld::has_estate_gutter(session.grid),
            "the Estate gutter sample starts at the live wall-kick route before the Crown");
        }
        session.state.player.x+=2;
        press({.restart=true});
        check(session.state.player.x==stop.entry_x&&
          std::abs(session.state.player.rect().bottom()-stop.entry_floor)<.01,
          "debug tour retry returns to the featured landing");
        session.profile.story=3;
        session.persist();
        session.state.won=true;
        press({.confirm=true});
      }
      check(session.tour_scene<0&&session.state.adventure.tour_picker&&session.profile.story==0&&
          session.save_revision==write_count&&bytes(path)==first_save&&bytes(secondary)==second_save&&bytes(tertiary)==third_save,
        "finishing the tour restores the profile and leaves all three save files identical");
      press({.pause=true});press({.confirm=true,.select_level=1});
      check(session.save_slot==1&&session.profile.seed==original_seed&&session.profile.started,
        "switching back to Slot 1 restores its existing journey");
      options.save_slot=2;
      AdventureSession reopened(options,path);
      check(reopened.save_slot==2&&reopened.profile.started&&reopened.profile.language==iceberg::Language::English,
        "explicit Slot 2 startup reloads its own journey and settings");
      options.save_slot=1;
      for(auto file:{secondary,tertiary}){std::filesystem::remove(file);file+=".bak";std::filesystem::remove(file);}
    }
    options.no_save=true;
    AdventureSession estate(options,path);
    estate.begin_campaign(true);
    estate.profile.unlocked=2;estate.run_district=2;
    auto &run=estate.profile.expeditions[0];
    run={};
    run.active=true;run.district=2;run.seed=run.origin_seed=73;
    run.itinerary={24,25,26,27,28,29,30,31};
    estate.begin_run(iceberg::Mode::Endless);
    check(estate.grid.template_id==24&&estate.grid.platforms.size()==3&&
      std::all_of(estate.grid.platforms.begin(),estate.grid.platforms.end(),[](const auto &platform){return platform.tram;}),
      "Windowsill Estate's tram room runs scheduled shuttles in current Endless layouts");
    const auto &tram=estate.grid.platforms.front();
    check(tram.at(0).x==tram.at(.5).x&&tram.at(3.).x==tram.at(3.5).x&&tram.at(3.).x-tram.at(0).x==15.,
      "the first tram stops at both landings and spans the street");
    auto rider=new_state(estate.grid);
    rider.player.x=tram.at(0).x+3.;rider.player.y=tram.base.y-kPlayerHeight;
    for(int tick=0;tick<360;++tick)world_step(rider,estate.grid,{},1./120);
    check(rider.player.support==0&&std::abs(rider.player.x-(tram.at(3.).x+3.))<.02&&rider.deaths==0,
      "the tram carries a standing courier between stops");
    run.layout_version=2;estate.load_room();
    check(estate.grid.template_id==24&&!estate.grid.platforms.front().tram,
      "older Endless saves keep the original tram room movement");
    run.itinerary[0]=26;run.layout_version=4;estate.load_room();
    check(estate.grid.template_id==26&&estate.grid.platforms.size()==2&&
      estate.grid.platforms[0].clothesline&&estate.grid.platforms[1].clothesline,
      "current Estate expeditions have two rideable clotheslines in Laundry over the street");
    const auto &laundry=estate.grid.platforms[1];
    estate.profile.language=iceberg::Language::English;
    estate.state.player.x=66.5;
    estate.state.player.y=laundry.at(0).y-kPlayerHeight;
    estate.state.player.grounded=true;estate.state.player.support=1;
    estate.tick({},1./120);
    check(estate.state.player.support==1&&estate.state.adventure.prompt.find("drop through")!=std::string::npos,
      "a courier on a clothesline sees how to drop through it");
    Controls cross;cross.move=1;
    for(int tick=0;tick<180&&estate.state.player.x<79.5;++tick)estate.tick(cross,1./120);
    check(estate.state.player.x>=79.5&&estate.state.player.support==1&&estate.state.deaths==0,
      "the full upper clothesline clears the balcony with the courier aboard");
    auto awning_rider=new_state(estate.grid);
    awning_rider.elapsed=laundry.period*.75;
    awning_rider.player.x=laundry.base.right()-.65;
    awning_rider.player.y=laundry.at(awning_rider.elapsed).top()-kPlayerHeight;
    awning_rider.player.grounded=true;awning_rider.player.support=1;
    for(int tick=0;tick<540;++tick)world_step(awning_rider,estate.grid,{},1./120);
    check(awning_rider.player.support==1&&awning_rider.deaths==0,
      "the upper clothesline carries an idle rider clear of the awning throughout its sway");
    // Stop before dropping onto the street landing under the line.
    estate.state.player.vx=0;
    Controls drop;drop.fast_fall=true;
    for(int tick=0;tick<100;++tick)estate.tick(drop,1./120);
    check(estate.state.player.grounded&&estate.state.player.support==-1&&estate.state.deaths==0&&
          std::abs(estate.state.player.rect().bottom()-43.)<.03,
      "a courier can drop through the second clothesline onto a lower street balcony");
    run.layout_version=2;estate.load_room();
    check(estate.grid.template_id==26&&estate.grid.platforms.empty(),
      "legacy Estate expeditions keep their original Laundry over the street layout");

    AdventureSession pressure(options,{});
    const auto steam_sample=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==20;});
    pressure.start_tour(int(steam_sample-iceberg::DebugTour.begin()));
    check(pressure.grid.air_fields.size()==1&&!pressure.grid.air_fields.front().active(pressure.grid.switch_flags),
      "the Heatworks pressure bypass starts closed");
    const auto jump_to_steam_shelf=[](AdventureSession &session) {
      for(int tick=0;tick<250;++tick) {
        Controls jump;
        jump.move=tick<29?1:tick<86?0:tick<131?-1:0;
        jump.jump_pressed=tick==0;
        jump.jump_held=tick<80;
        session.tick(jump,1./120.);
      }
      return session.state.player.grounded&&std::abs(session.state.player.rect().bottom()-35.)<.05;
    };
    check(!jump_to_steam_shelf(pressure),"the same jump misses the upper steam shelf with the valve closed");
    pressure.end_tour();
    pressure.start_tour(int(steam_sample-iceberg::DebugTour.begin()));
    pressure.tick({.interact=true},1./120.);
    check(pressure.grid.air_fields.front().active(pressure.grid.switch_flags)&&
          jump_to_steam_shelf(pressure)&&pressure.state.deaths==0&&
          std::count(pressure.state.collected.begin(),pressure.state.collected.end(),true)==1,
      "opening the supply lifts an ordinary jump to the upper token without a dash or retry");
    AdventureSession steam_run(options,path);
    steam_run.begin_campaign(true);
    steam_run.profile.unlocked=1;steam_run.run_district=1;
    auto &steam_save=steam_run.profile.expeditions[0];
    steam_save={};steam_save.active=true;steam_save.district=1;steam_save.seed=steam_save.origin_seed=73;
    steam_save.itinerary={20,12,13,16,17,18,21,22};
    steam_run.begin_run(iceberg::Mode::Endless);
    check(steam_run.grid.air_fields.size()==1&&!steam_run.grid.air_fields.front().active(steam_run.grid.switch_flags),
      "the pressure bypass is available in a regular current Heatworks expedition");
    steam_run.state.player.x=54.;steam_run.state.player.y=43.-kPlayerHeight;
    steam_run.tick({.interact=true},1./120.);
    steam_run.begin_campaign(false);steam_run.begin_run(iceberg::Mode::Endless);
    check(steam_run.grid.air_fields.front().active(steam_run.grid.switch_flags),
      "the open steam supply survives a saved expedition resume");
    steam_save.layout_version=2;steam_run.load_room();
    check(steam_run.grid.air_fields.empty(),"legacy Heatworks expeditions do not gain the pressure bypass");

    AdventureSession archive(options,path);
    archive.begin_campaign(true);
    archive.profile.unlocked=3;archive.run_district=3;
    auto &archive_run=archive.profile.expeditions[0];
    archive_run={};archive_run.active=true;archive_run.district=3;archive_run.seed=archive_run.origin_seed=73;
    archive_run.itinerary={39,36,37,38,40,41,42,43};
    archive.begin_run(iceberg::Mode::Endless);
    check(archive.grid.growth.size()==2&&!archive.grid.solid_at(65,40)&&verify_route(archive.grid).complete,
      "the Archive's main route still works while the catalogue drawers are shut");
    archive.state.player.x=57.;archive.state.player.y=44.-kPlayerHeight;
    Controls pull;pull.interact=true;archive.tick(pull,1./120);
    check((archive.grid.switch_flags&1)&&!archive.grid.solid_at(65,40),
      "the catalogue handle starts extending drawers through ordinary interaction");
    for(int tick=0;tick<150;++tick)archive.tick({},1./120);
    check(archive.grid.solid_at(65,40)&&archive.grid.solid_at(73,35),
      "both open drawers become solid landing steps");
    archive.begin_campaign(false);archive.begin_run(iceberg::Mode::Endless);
    check(archive.grid.solid_at(65,40)&&archive.grid.solid_at(73,35),
      "the same expedition room resumes with the drawers extended");
    archive.advance_room();
    check(archive.grid.growth.empty()&&archive.grid.switch_flags==0,
      "drawer state does not carry into the next expedition room");
    archive_run.itinerary[1]=41;
    archive_run.layout_version=2;archive.load_room();
    check(archive.grid.template_id==41&&archive.grid.platforms.size()==3&&
          std::none_of(archive.grid.platforms.begin(),archive.grid.platforms.end(),[](const auto &car){return car.tram;}),
      "older Archive expeditions keep their original rail platforms");
    archive_run.layout_version=4;archive.load_room();
    check(archive.grid.platforms.size()==3&&
          std::all_of(archive.grid.platforms.begin(),archive.grid.platforms.end(),[](const auto &car){return car.tram;})&&
          verify_route(archive.grid).complete,"the scheduled Archive shuttles preserve the through-route");
    AdventureSession rail(options,{});
    const auto rail_sample=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==41;});
    rail.start_tour(int(rail_sample-iceberg::DebugTour.begin()));
    const auto &car=rail.grid.platforms.front();
    check(car.at(0.).x==car.at(.5).x&&car.at(2.8).x==car.at(3.3).x&&
          std::abs(car.at(2.8).x-car.at(0.).x-10.8)<.01,
      "the Archive shuttle waits at both landings and crosses between them");
    for(int tick=0;tick<93;++tick) {
      Controls step;step.move=tick<24?1:0;step.jump_pressed=tick==0;step.jump_held=tick<85;
      rail.tick(step,1./120);
    }
    check(rail.state.player.support==0,"a normal jump from the Archive landing boards the rail car");
    const auto boarded_x=rail.state.player.x;
    for(int tick=93;tick<332;++tick)rail.tick({},1./120);
    check(rail.state.player.support==0&&rail.state.player.x-boarded_x>9.&&rail.state.deaths==0,
      "waiting aboard carries the courier to the far landing without steering");
    archive_run.itinerary={47,36,37,38,39,40,41,42};
    archive_run.room=archive_run.depth=archive.room=archive.depth=0;
    archive.load_room();
    const auto &racks=archive.grid.platforms;
    check(racks.size()==5&&racks[3].archive_rack&&racks[4].archive_rack&&
      std::abs(racks[3].at(1.6).y-44.)<.01&&std::abs(racks[4].at(1.6).y-30.)<.01&&
      std::abs(racks[3].at(4.8).y-30.)<.01&&std::abs(racks[4].at(4.8).y-44.)<.01,
      "Archive racks move in opposite directions over the same height range");
    if(racks.size()==5) {
      auto rider=new_state(archive.grid);
      rider.player.x=racks[4].base.x+2.;
      rider.player.y=racks[4].at(0).y-kPlayerHeight;
      rider.player.grounded=true;rider.player.support=4;
      for(int tick=0;tick<120;++tick)world_step(rider,archive.grid,{},1./120);
      check(rider.player.support==4&&rider.player.rect().bottom()<32.&&rider.player.lift_y<-3.,
        "the rising rack carries a rider and supplies upward momentum");
    }
    archive_run.layout_version=2;archive.load_room();
    check(archive.grid.platforms.size()==3&&
      archive.grid.sweets.size()==verified_room(47,archive.grid.seed,3).sweets.size(),
      "older Endless saves retain the original room decks and token count");
    AdventureSession gardens(options,path);
    gardens.begin_campaign(true);
    gardens.profile.unlocked=4;gardens.run_district=4;
    auto &garden_run=gardens.profile.expeditions[0];
    garden_run={};garden_run.active=true;garden_run.district=4;garden_run.seed=garden_run.origin_seed=73;
    garden_run.itinerary={48,49,50,51,52,53,54,58};
    gardens.begin_run(iceberg::Mode::Endless);
    const auto &frozen=gardens.grid;
    check(frozen.template_id==48&&frozen.ice_strips.size()==4&&verify_route(frozen).complete,
      "current Underfrost expeditions have verified patches of ice");
    auto dry=frozen;dry.ice_strips.clear();
    auto skating=new_state(frozen);
    skating.player.x=11.;skating.player.grounded=true;skating.player.vx=10.;
    auto walking=skating;
    for(int tick=0;tick<30;++tick) {
      world_step(skating,frozen,{},1./120);
      world_step(walking,dry,{},1./120);
    }
    check(skating.player.x-walking.player.x>1.2&&skating.player.vx>4.&&walking.player.vx==0.,
      "the same move release coasts along ice and brakes on dry ground");
    auto above=skating.player;above.grounded=false;
    check(!frozen.on_ice(above),"airborne movement retains its normal traction rules");
    garden_run.layout_version=2;gardens.load_room();
    check(gardens.grid.template_id==48&&gardens.grid.ice_strips.empty(),
      "a legacy Underfrost save keeps its prior dry surfaces");
    gardens.advance_room();
    check(gardens.grid.template_id==49&&gardens.grid.springs.empty(),
      "a legacy Root Well retains its original landing");
    garden_run.layout_version=iceberg::ContentVersion;gardens.load_room();
    const auto &well=gardens.grid;
    check(well.springs.size()==1&&verify_route(well).complete,
      "current Root Well expeditions include a seedpod and retain a through-route");
    auto bounce=new_state(well);
    bounce.player.x=65;bounce.player.y=48-kPlayerHeight;bounce.player.grounded=true;
    Controls approach;approach.move=1;approach.jump_held=true;
    bool launched=false,reached_shelf=false;
    for(int tick=0;tick<300&&!reached_shelf;++tick) {
      approach.move=int(bounce.player.x<80.);
      world_step(bounce,well,approach,1./120);
      launched|=bounce.player.vy<-29;
      reached_shelf=bounce.player.grounded&&bounce.player.x>=78&&bounce.player.x<86&&
                   std::abs(bounce.player.y+kPlayerHeight-43)<.01;
    }
    check(launched&&reached_shelf&&bounce.deaths==0,
      "running onto the seedpod reaches the optional shelf without a jump or dash press");
    gardens.room=3;gardens.load_room();
    const auto &pot_room=gardens.grid;
    check(pot_room.template_id==51&&pot_room.platforms.size()==2&&verify_route(pot_room).complete,
      "fragile pots add optional platforms without blocking the original Gardens route");
    const auto &pot=pot_room.platforms.front();
    auto waiting=new_state(pot_room);
    waiting.player.x=pot.base.x+1;
    waiting.player.y=pot.base.y-kPlayerHeight-.2;
    for(int tick=0;tick<20;++tick)world_step(waiting,pot_room,{},1./120);
    check(waiting.player.support==0&&waiting.platform_armed_at.front()>0.&&
          waiting.platform_armed_at.back()==0.,"landing arms only the touched planter");
    auto hopping=waiting;
    Controls jump;jump.jump_pressed=jump.jump_held=true;
    world_step(hopping,pot_room,jump,1./120);
    check(hopping.player.support<0&&hopping.player.vy<0.,"jumping before the crack gives way works normally");
    for(int tick=0;tick<90;++tick)world_step(waiting,pot_room,{},1./120);
    check(waiting.player.support<0&&waiting.deaths==0&&waiting.player.rect().bottom()>pot.base.y&&
          !pot.available(waiting,0,waiting.elapsed),"waiting breaks the pot and drops the courier to the landing below");
    for(int tick=0;tick<360&&!pot.available(waiting,0,waiting.elapsed);++tick)
      world_step(waiting,pot_room,{},1./120);
    waiting.player.x=pot.base.x+1;waiting.player.y=pot.base.y-kPlayerHeight-.2;
    for(int tick=0;tick<20;++tick)world_step(waiting,pot_room,{},1./120);
    check(waiting.player.support==0&&pot.crack_age(waiting,0,waiting.elapsed)<.2,
      "a replaced planter can support another attempt");
    Controls retry_pots;retry_pots.restart=true;
    world_step(waiting,pot_room,retry_pots,1./120);
    check(waiting.platform_armed_at.front()==0.,"retry clears the planter's crack timer");
    garden_run.layout_version=2;gardens.load_room();
    check(gardens.grid.template_id==51&&gardens.grid.platforms.empty(),
      "older Gardens itineraries keep the original footholds");
    garden_run.itinerary[3]=55;gardens.load_room();
    const auto old_token_count=gardens.grid.sweets.size();
    check(gardens.grid.template_id==55&&gardens.grid.platforms.empty()&&gardens.grid.water_basins.empty(),
      "older Warm Reservoir layouts keep the original route");
    garden_run.layout_version=iceberg::ContentVersion;gardens.load_room();
    const auto &spillway=gardens.grid;
    check(spillway.platforms.size()==5&&spillway.water_basins.size()==1&&
          spillway.sweets.size()==old_token_count+1&&verify_route(spillway).complete,
      "current Endless Reservoir adds an optional ice spillway with a token and a verified upper route");
    const auto &floe=spillway.platforms.front();
    auto on_floe=new_state(spillway);
    on_floe.player.x=floe.base.x+2.5;
    on_floe.player.y=floe.base.y-kPlayerHeight-.2;
    for(int tick=0;tick<20;++tick)world_step(on_floe,spillway,{},1./120);
    check(on_floe.player.support==0&&spillway.on_ice(on_floe.player)&&
          on_floe.platform_armed_at.front()>0.&&on_floe.platform_armed_at[1]==0.,
      "each floe is slippery and starts cracking only when the courier lands on it");
    auto skater=on_floe,dry_walker=on_floe;
    skater.player.vx=dry_walker.player.vx=10.;
    auto dry_spillway=spillway;dry_spillway.platforms[0].icy=false;
    for(int tick=0;tick<30;++tick) {
      world_step(skater,spillway,{},1./120);
      world_step(dry_walker,dry_spillway,{},1./120);
    }
    check(skater.player.x-dry_walker.player.x>1.2&&skater.player.support==0,
      "releasing movement glides farther on the floe than on a dry platform");
    for(int tick=0;tick<66;++tick)world_step(on_floe,spillway,{},1./120);
    check(on_floe.deaths==0&&on_floe.player.support<0&&on_floe.player.rect().bottom()>48.&&
          !floe.available(on_floe,0,on_floe.elapsed)&&spillway.platforms[1].available(on_floe,1,on_floe.elapsed),
      "the cracked floe drops the courier and neighboring floes stay solid");
    check(floe.available(on_floe,0,on_floe.platform_armed_at[0]+MovingPlatform::RestoreTime+.01),
      "the broken floe is solid again after its recovery time");
    garden_run.layout_version=2;gardens.room=4;gardens.load_room();
    const auto original_channel=gardens.grid;
    check(original_channel.template_id==52&&original_channel.growth.empty(),
      "older greenhouse channels retain their original route");
    garden_run.layout_version=iceberg::ContentVersion;gardens.load_room();
    const auto &channel=gardens.grid;
    check(channel.growth.size()==1&&channel.sweets.size()==original_channel.sweets.size()+1&&
          !channel.solid_at(101,41)&&verify_route(channel).complete,
      "the dormant catwalk leaves the original lower channel usable");
    gardens.state.player.x=80.5;gardens.state.player.y=41-kPlayerHeight;
    gardens.state.player.grounded=true;
    Controls water_vines;water_vines.interact=true;
    gardens.tick(water_vines,1./120);
    check((channel.switch_flags&1)&&!channel.solid_at(101,41),
      "using the valve starts the bridge growing from the overlook");
    const double bridge_width=channel.growth.front().extended_width(channel.switch_flags);
    const double bridge_time=gardens.state.elapsed;
    Controls read_map;read_map.map=true;
    gardens.tick(read_map,.1);
    check(gardens.state.adventure.map&&gardens.state.elapsed==bridge_time&&
          channel.growth.front().extended_width(channel.switch_flags)==bridge_width,
      "opening the room map freezes the growing vine bridge immediately");
    gardens.tick({},.1);
    gardens.tick(read_map,.1);
    gardens.tick({},.1,true,33,24);
    check(!gardens.state.adventure.map&&gardens.state.elapsed==bridge_time&&
          channel.growth.front().extended_width(channel.switch_flags)==bridge_width,
      "closing the map and a too-small viewport consume no bridge growth");
    gardens.talk({{0,{"Chwila.","One moment."}}});
    gardens.tick({},.1);
    check(gardens.state.adventure.dialogue.active()&&gardens.state.elapsed==bridge_time&&
          channel.growth.front().extended_width(channel.switch_flags)==bridge_width,
      "dialogue freezes the bridge with the rest of the room");
    gardens.state.adventure.dialogue={};
    for(int tick=0;tick<150;++tick)gardens.tick({},1./120);
    check(channel.solid_at(86,41)&&channel.solid_at(112,41)&&!channel.solid_at(113,41),
      "the grown trellis stops before the drop onto the next landing");
    Controls cross_channel;cross_channel.move=1;
    int retries=gardens.state.deaths;
    const auto landed_below=[&] {
      return gardens.state.player.x>=118&&gardens.state.player.grounded&&
             std::abs(gardens.state.player.rect().bottom()-49)<.01;
    };
    for(int tick=0;tick<420&&!landed_below();++tick)
      gardens.tick(cross_channel,1./120);
    check(landed_below()&&gardens.state.deaths==retries&&gardens.state.collected.back(),
      "walking over the vines collects the token and drops onto the old route");
    gardens.begin_campaign(false);gardens.begin_run(iceberg::Mode::Endless);
    check(gardens.grid.template_id==52&&gardens.grid.solid_at(112,41)&&gardens.state.collected.back(),
      "resuming the greenhouse channel restores both the bridge and its token");
    AdventureSession crown(options,path);
    crown.begin_campaign(true);
    crown.profile.unlocked=5;crown.run_district=5;
    auto &crown_run=crown.profile.expeditions[0];
    crown_run={};crown_run.active=true;crown_run.district=5;crown_run.seed=crown_run.origin_seed=73;
    crown_run.itinerary={60,63,61,62,64,65,66,67};
    crown.begin_run(iceberg::Mode::Endless);
    for(double direction:{1.,-1.}) {
      const auto &ridge=crown.grid;
      check(ridge.air_fields.size()==1&&ridge.air_fields[0].ax==44.*direction&&verify_route(ridge).complete,
        "the current Crown route retains a verified gust in its intended direction");
      auto gust=new_state(ridge);gust.player.x=80.;gust.player.y=31.;gust.elapsed=1.5;
      auto lull=gust;lull.elapsed=4.5;
      for(int tick=0;tick<42;++tick) {
        world_step(gust,ridge,{},1./120);
        world_step(lull,ridge,{},1./120);
      }
      check((gust.player.x-lull.player.x)*direction>1.1,
        "a Crown gust moves an airborne courier farther in the wind direction than a lull");
      if(direction>0)crown.advance_room();
    }
    crown_run.layout_version=2;crown.load_room();
    check(crown.grid.template_id==63&&crown.grid.air_fields[0].ax==-4.&&crown.grid.platforms.empty(),
      "legacy Crown expeditions keep their original wind strength");
    AdventureSession ribs(options,{});
    const auto ribs_sample=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==63;});
    ribs.start_tour(int(ribs_sample-iceberg::DebugTour.begin()));
    check(ribs.grid.platforms.size()==2&&ribs.grid.platforms[0].fragile&&ribs.grid.platforms[1].fragile,
      "Fractured Ribs adds two buckling beams to the optional climb");
    auto climb=[&](int duration,int right) {
      for(int tick=0;tick<duration;++tick) {
        Controls c;c.jump_pressed=tick==0;c.jump_held=true;c.move=tick<right?1:0;
        ribs.tick(c,1./120);
      }
    };
    climb(66,51);
    check(ribs.state.player.support==0&&ribs.state.platform_armed_at[0]>0.,
      "an ordinary jump boards the lower beam before it buckles");
    climb(62,15);
    check(ribs.state.player.support==1&&ribs.state.platform_armed_at[1]>0.,
      "the second jump reaches the upper beam");
    climb(82,0);
    check(ribs.state.deaths==0&&ribs.state.player.grounded&&ribs.state.collected[3]&&
          std::abs(ribs.state.player.rect().bottom()-34.)<.01&&
          !ribs.grid.platforms[0].available(ribs.state,0,ribs.state.elapsed),
      "the gust carries the third jump to the existing token shelf as the first beam gives way");
    crown_run.itinerary[1]=68;crown.load_room();
    check(crown.grid.template_id==68&&crown.grid.air_fields.empty(),
      "older Ridge Sensors layouts remain calm");
    crown_run.layout_version=4;crown.load_room();
    const auto &ridge=crown.grid;
    check(ridge.template_id==68&&ridge.air_fields.size()==1&&ridge.air_fields[0].reverses&&verify_route(ridge).complete,
      "current Endless Ridge Sensors has a reversing crosswind and a verified lower route");
    auto collect_upper_token=[](const EngineGrid &grid,double elapsed) {
      auto s=new_state(grid);
      s.player.x=91.;s.player.y=32.-kPlayerHeight;s.player.grounded=true;s.elapsed=elapsed;
      FixedStepper motion;
      Controls launch;launch.move=1;launch.dash_pressed=true;
      motion.advance(s,grid,launch,1./120);
      Controls leap;leap.move=1;leap.jump_pressed=leap.jump_held=true;
      for(int i=0;i<100;++i) {
        motion.advance(s,grid,leap,1./120);
        leap.jump_pressed=false;
      }
      return s.collected.back()&&s.deaths==0;
    };
    const auto calm=verified_room(68,ridge.seed,5);
    check(collect_upper_token(ridge,1.8)&&!collect_upper_token(ridge,5.4)&&!collect_upper_token(calm,1.8),
      "a dash jump from the same ledge collects the upper token with the east gust only");
    int ready_phases=0;
    for(int tick=0;tick<864;++tick) {
      const double elapsed=tick/120.;
      if(!iceworld::ridge_launch_ready(ridge.air_fields.front(),elapsed))continue;
      ++ready_phases;
      if(!collect_upper_token(ridge,elapsed))
        throw std::runtime_error("Ridge beacon lights too early or too late at "+std::to_string(elapsed));
    }
    check(iceworld::has_ridge_mast(ridge)&&!iceworld::has_ridge_mast(calm)&&ready_phases>100,
      "the observation mast marks a sustained gust window and the indicated dash jump works throughout it");
    for(unsigned seed=0;seed<32;++seed) {
      auto sample=verified_room(68,iceberg::mix(seed),5);
      add_ridge_crosswind(sample);
      const auto path=verify_route(sample);
      if(!iceworld::has_ridge_mast(sample)||!path.complete||path.deaths)
        throw std::runtime_error("Ridge mast blocks the lower route for seed "+std::to_string(seed));
    }
    check(true,"the mast preserves the lower Ridge Sensors route over 32 seeds");
    auto exposed=verified_room(70,iceberg::mix(73),5);
    strengthen_expedition_wind(exposed);
    auto sheltered=exposed;
    add_overhang_windbreak(sheltered);
    check(sheltered.windbreak_roof&&sheltered.solid_at(74,34)&&!exposed.solid_at(74,34)&&
      sheltered.air_fields.size()==1&&sheltered.air_fields.front().area.bottom()==34.,
      "the windbreak is solid and the Crown gusts stop at its upper edge");
    const auto drift=[](const EngineGrid &grid,double y) {
      auto player=new_state(grid);
      player.elapsed=1.3;player.player.x=77.;player.player.y=y;
      FixedStepper motion;
      for(int n=0;n<24;++n)motion.advance(player,grid,{},1./120);
      return player.player.x;
    };
    check(drift(sheltered,37.)>drift(exposed,37.)+.3&&
      std::abs(drift(sheltered,17.)-drift(exposed,17.))<.01&&drift(sheltered,17.)<76.7,
      "wind still pushes on the upper route while the lower route is sheltered");

    crown_run.itinerary[1]=69;crown.load_room();
    const auto &counterweights=crown.grid;
    const auto original_counterweights=verified_room(69,counterweights.seed,5);
    const auto lower_route=verify_route(counterweights);
    check(counterweights.platforms.size()==original_counterweights.platforms.size()+1&&
          lower_route.complete&&lower_route.deaths==0,
      "current Shutter Counterweights adds a service lift and retains its lower route");
    const auto &service_lift=counterweights.platforms.back();
    auto lift_rider=new_state(counterweights);
    lift_rider.player.x=service_lift.base.x+1.;
    lift_rider.player.y=service_lift.at(0).y-kPlayerHeight;
    lift_rider.player.grounded=true;
    lift_rider.player.support=int(counterweights.platforms.size()-1);
    const auto low_landing=lift_rider.player.rect().bottom();
    for(int tick=0;tick<350;++tick)world_step(lift_rider,counterweights,{},1./120);
    check(lift_rider.player.support==int(counterweights.platforms.size()-1)&&
          lift_rider.deaths==0&&low_landing-lift_rider.player.rect().bottom()>15.,
      "the service lift carries a neutral rider from the lower route toward the upper shelves");
    crown_run.layout_version=2;crown.load_room();
    check(crown.grid.template_id==69&&crown.grid.platforms.size()==original_counterweights.platforms.size(),
      "legacy Shutter Counterweights retains its original platforms");

    AdventureSession prior_leg(options,path);
    prior_leg.begin_campaign(true);
    prior_leg.profile.unlocked=2;prior_leg.run_district=2;
    auto &estate_run=prior_leg.profile.expeditions[0];estate_run={};
    estate_run.active=true;estate_run.district=2;estate_run.seed=estate_run.origin_seed=73;
    const auto saved_order=iceworld::make_itinerary(73,2,iceberg::RoomsPerLeg);
    estate_run.itinerary=saved_order;estate_run.depth=estate_run.room=7;
    prior_leg.begin_run(iceberg::Mode::Endless);
    check(estate_run.itinerary==saved_order&&prior_leg.room==7,"an existing Endless leg resumes with its saved room order");
    prior_leg.advance_room();
    const auto early_end=estate_run.itinerary.begin()+4;
    check(prior_leg.depth==8&&prior_leg.room==0&&
      std::any_of(estate_run.itinerary.begin(),early_end,[](int id){return iceworld::has_expedition_mechanic(2,id);}),
      "an existing expedition gets the district mechanic when generating its next leg");
    std::filesystem::remove(path);std::filesystem::remove(backup);
    return 0;
  } catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_validation.hpp"
#include <stdexcept>
void check(bool value,const char *message) {
  if(!value)throw std::runtime_error(message);
  std::cout<<"PASS "<<message<<'\n';
}
int main(int argc,char **argv) {
  try {
    int seeds=argc>1?std::max(1,std::atoi(argv[1])):100;
    for(int seed=0;seed<seeds;++seed) {
      int biome=seed%int(iceworld::patterns().size()/12);
      auto route=iceworld::make_itinerary(unsigned(seed),biome);
      if(route!=iceworld::make_itinerary(unsigned(seed),biome))throw std::runtime_error("nondeterministic itinerary");
      for(size_t i=1;i<route.size();++i)if(route[i]==route[i-1]||!iceworld::connects(iceworld::pattern(route[i-1]),iceworld::pattern(route[i])))throw std::runtime_error("repeated adjacent room");
    }
    check(true,"structural seed determinism and adjacency checks");
    int failures=0;
    for(const auto &pattern:iceworld::patterns()) {
      auto g=iceworld::build_room(pattern.id,73,pattern.id/12);
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
      auto g=verified_room(p.id,iceberg::mix(unsigned(n)),p.id/12,expert);
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
    check(save.save(profile),"campaign save written and verified");
    iceberg::Profile loaded;check(save.load(loaded),"campaign save loaded");
    check(loaded.itinerary==profile.itinerary&&loaded.story==profile.story&&loaded.quest_counts[0]==3&&loaded.checkpoint==2,
      "route, story, task and checkpoint survive reload");
    check(loaded.expeditions[0].seed==821&&loaded.expeditions[0].depth==13&&loaded.expeditions[0].itinerary==exp.itinerary,
      "expedition identity and checkpoint stored independently");
    check(loaded.revisit.active&&loaded.revisit.scene==1&&loaded.revisit.seconds==42.5&&loaded.revisit.switches==1&&loaded.interlude,"revisited story route persists its separate checkpoint and machinery");
    check(loaded.challenges.size()==2&&loaded.challenges[0].record.best_seconds==99&&loaded.challenges[1].seed==74,"seeded challenge records stay isolated by route identity");
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
    {std::ofstream out(path);out<<"damaged";}
    check(save.load(loaded)&&save.recovered&&loaded.room==4,"damaged primary recovers prior good save");
    std::filesystem::remove(path);auto backup=path;backup+=".bak";std::filesystem::remove(backup);
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
      check(!session.state.adventure.trials[0].complete,"new custom seed never displays another route's default record");
      session.begin_campaign(true);
      check(session.stage==0&&session.room==0&&session.profile.story==0,"new story resets progression without copying old room state");
    }
    std::filesystem::remove(path);std::filesystem::remove(backup);
    return 0;
  } catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

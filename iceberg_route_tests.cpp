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
    Controls use;use.interact=true;
    Controls retry;retry.restart=true;
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
    require(persisted.route_choices[0]==1&&persisted.layout_version==4,"save retains chosen branch and generator");
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
    options.no_save=true;
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
    for(int q:{500,503,506})require(std::find(original.begin(),original.begin()+16,q)!=original.begin()+16,"parcel landmark remains before the last turn-in");
    std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

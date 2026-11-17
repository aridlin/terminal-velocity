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
    const auto choice=*at;s.choose_route(choice);
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
    AdventureSession direct(options,path);direct.profile.hidden_room=-1;direct.profile.route_choices={};direct.profile.route_decisions={};direct.room=1;direct.stage=2;direct.load_room();direct.advance_room();
    require(direct.grid.template_id==original[2]&&direct.profile.route_decisions[0]==1&&direct.profile.route_choices[0]==0,
      "normal doorway keeps the main branch");
    for(unsigned version:{1u,2u,3u}) {
      auto legacy=iceworld::make_itinerary(73,0,24,version);
      require(std::none_of(legacy.begin(),legacy.end(),[](int v){return v>=500;}),"published generator versions exclude new rooms");
    }
    for(int q:{500,503,506})require(std::find(original.begin(),original.begin()+16,q)!=original.begin()+16,"parcel landmark remains before the last turn-in");
    std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

#pragma once
// Authoritative two-courier expedition. Never loads or writes user profiles.
namespace { namespace party {
constexpr int Rooms=8;
enum class Mode {Race,Coop};
struct Match {
  std::array<std::unique_ptr<AdventureSession>,2> players;
  std::array<bool,2> connected{},ready{};
  std::array<int,2> progress{};
  std::array<double,2> finish{-1,-1};
  Mode mode;
  uint32_t seed;
  int district;
  double clock=0,countdown=3,room_clock=0,exit_wait=0;
  bool started=false,ended=false;
  explicit Match(Mode m,uint32_t s,int d=0):mode(m),seed(s),district(std::clamp(d,0,5)) {
    iceberg::SessionOptions o;o.no_save=true;o.mute=true;o.seed=s;o.seed_override=true;
    for(auto &p:players){p=std::make_unique<AdventureSession>(o,std::filesystem::path{});p->profile.language=iceberg::Language::English;}
    load(0);load(1);
  }
  void load(int i) {
    auto &p=*players[i];p.mode=iceberg::Mode::Endless;p.district=(district+progress[i])%6;p.stage=2;
    p.room=progress[i];p.run_seed=seed;p.run_seconds=clock;
    // Stable selection includes all biome templates, with no adjacent repeat.
    const int id=p.district*12+int((iceberg::mix(seed)%12+unsigned(progress[i])*5)%12);
    p.grid=verified_room(id,iceberg::mix(seed+unsigned(progress[i])),p.district);
    p.state=new_state(p.grid);p.state.menu=false;p.state.multiplayer=true;
    p.profile.pause_on_focus_loss=false;p.stepper={};p.sync_view();
  }
  bool same_room()const{return progress[0]==progress[1];}
  void step(std::array<Controls,2> controls,double dt=1./120) {
    dt=clamp(dt,0.,1./30);
    for(int i=0;i<2;++i)if(connected[i]&&(controls[i].confirm||controls[i].interact))ready[i]=true;
    if(ended)return;
    if(!started){if(connected[0]&&connected[1]&&ready[0]&&ready[1]){countdown=std::max(0.,countdown-dt);started=countdown<=0;}return;}
    // Disconnection suspends the whole match; reconnect keeps the same state.
    if(!connected[0]||!connected[1])return;
    clock+=dt;room_clock+=dt;
    if(mode==Mode::Coop&&same_room()) {
      const uint64_t switches=players[0]->grid.switch_flags|players[1]->grid.switch_flags;
      for(size_t n=0;n<std::min(players[0]->state.platform_armed_at.size(),players[1]->state.platform_armed_at.size());++n) {
        const double a=players[0]->state.platform_armed_at[n],b=players[1]->state.platform_armed_at[n];
        const double shared=a>0&&b>0?std::min(a,b):std::max(a,b);
        for(auto &p:players)p->state.platform_armed_at[n]=shared;
      }
      const int checkpoint=std::max(players[0]->state.checkpoint,players[1]->state.checkpoint);
      for(auto &p:players){
        const auto newly_on=switches&~p->grid.switch_flags;
        for(auto &bridge:p->grid.growth)if(newly_on&bridge.flag)bridge.amount=0;
        p->grid.switch_flags=switches;p->state.checkpoint=checkpoint;
      }
      for(size_t n=0;n<std::min(players[0]->state.collected.size(),players[1]->state.collected.size());++n)
        if(players[0]->state.collected[n]||players[1]->state.collected[n])for(auto &p:players)p->state.collected[n]=true;
    }
    for(int i=0;i<2;++i) {
      if(finish[i]>=0)continue;
      auto &p=*players[i];auto c=controls[i];
      // A match cannot enter campaign menus, save slots, secret rooms or restart its route.
      c.menu_request=c.new_run=c.next_seed=c.quit=false;c.select_level=0;c.language=false;
      if(p.state.won)continue;
      p.tick(c,dt,true,p.last_columns,p.last_rows,2);
      // Both copies use the same machinery phase even when one courier waits.
      p.state.elapsed=room_clock;
    }
    if(mode==Mode::Race) {
      for(int i=0;i<2;++i)if(finish[i]<0&&players[i]->state.won) {
        if(++progress[i]>=Rooms)finish[i]=clock;else load(i);
      }
      ended=finish[0]>=0&&finish[1]>=0;
    } else if(players[0]->state.won&&players[1]->state.won) {
      exit_wait+=dt;
      if(exit_wait>=.6){exit_wait=0;room_clock=0;for(int i=0;i<2;++i)++progress[i];
        if(progress[0]>=Rooms){finish={clock,clock};ended=true;}else{load(0);load(1);}}
    } else exit_wait=0;
    for(int i=0;i<2;++i) {
      auto &p=*players[i];p.state.ghosts.clear();
      if(same_room()&&finish[1-i]<0) {
        ghosts::Ghost g;g.state.id=unsigned(2-i);g.x=players[1-i]->state.player.x;g.y=players[1-i]->state.player.y;g.state.flags=2;
        p.state.ghosts.push_back(g);
      }
    }
  }
  std::string status(int i)const {
    const auto t=[&](const char *pl,const char *en){return std::string(players[i]->profile.language==iceberg::Language::English?en:pl);};
    if(!connected[1-i])return t("Oczekiwanie na kuriera ","Waiting for courier ")+std::to_string(2-i);
    if(!started)return ready[0]&&ready[1]?"Start: "+std::to_string(int(std::ceil(countdown))):t("ENTER/E: gotowy","ENTER/E: ready");
    if(ended)return mode==Mode::Coop?t("WSPÓLNA DOSTAWA","DELIVERED TOGETHER"):finish[0]==finish[1]?t("REMIS","DRAW"):(finish[i]<finish[1-i]?t("WYGRANA","YOU WIN"):t("KONIEC WYŚCIGU","RACE FINISHED"));
    if(finish[i]>=0)return t("Twój czas: ","Your time: ")+std::to_string(finish[i])+"s";
    if(mode==Mode::Coop&&players[i]->state.won)return t("Poczekaj przy wyjściu na partnera","Wait at the exit for your partner");
    return (mode==Mode::Race?t("WYŚCIG","RACE"):t("WSPÓŁPRACA","CO-OP"))+"  "+std::to_string(std::min(Rooms,progress[i]+1))+"/8  "+std::to_string(clock)+"s";
  }
};
}

} // anonymous namespace

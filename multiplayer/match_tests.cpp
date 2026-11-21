#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "../main.cpp"
#include "match.hpp"
#include <source_location>
int main(){
 party::Match race(party::Mode::Race,123),coop(party::Mode::Coop,123);
 auto check=[](bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Match invariant failed at line "+std::to_string(at.line()));};
 for(auto *m:{&race,&coop}){
  check(m->players[0]->grid.seed==m->players[1]->grid.seed && m->players[0]->grid.template_id==m->players[1]->grid.template_id);
  m->connected={true,true};std::array<Controls,2> c;c[0].confirm=true;c[1].confirm=true;
  for(int n=0;n<362;++n)m->step(c);
  check(m->started);check(m->clock<.1);
  m->connected[1]=false;const auto before=m->clock;m->step({});check(m->clock==before);m->connected[1]=true;
 }
 coop.players[0]->grid.switch_flags=1;coop.step({});check(coop.players[1]->grid.switch_flags&1);
 coop.players[0]->state.won=true;coop.step({});check(coop.progress[0]==0&&coop.progress[1]==0);
 race.players[0]->state.won=true;race.step({});check(race.progress[0]==1&&race.progress[1]==0);
 for(int room=0;room<8;++room){for(auto &p:coop.players)p->state.won=true;for(int t=0;t<73;++t)coop.step({});}
 check(coop.ended&&coop.finish[0]==coop.finish[1]);
 // Actual controls must move both couriers through the eight-room expedition.
 for(auto mode:{party::Mode::Race,party::Mode::Coop}) {
  party::Match played(mode,73);played.connected={true,true};played.ready={true,true};played.started=true;
  std::array<RoutePilot,2> pilots;std::array<int,2> last_room{};
  for(int tick=0;tick<120000&&!played.ended;++tick) {
   std::array<Controls,2> input;
   for(int i=0;i<2;++i){if(last_room[i]!=played.progress[i]){pilots[i]={};last_room[i]=played.progress[i];}
    input[i]=pilots[i].controls(played.players[i]->grid,played.players[i]->state);}
   played.step(input);
  }
  if(!played.ended)throw std::runtime_error("Ordinary-input multiplayer route stalled at "+std::to_string(played.progress[0])+","+std::to_string(played.progress[1]));
 }
 std::cout<<"PASS ordinary-input race and co-op finish all eight rooms\n";
 std::cout<<"PASS common route, ready countdown, disconnect freeze, race independence, shared machinery, cooperative exits and finish\n";
}

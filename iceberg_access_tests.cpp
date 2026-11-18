#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
int main(int argc,char **argv) {
  bool task=argc>2&&std::string(argv[1])=="--task";
  int id=task?std::atoi(argv[2]):argc>1?std::atoi(argv[1]):0;
  auto grid=task?verified_room(id,73,id/12):id==0?iceworld::build_settlement(0):iceworld::build_authored(0,id==2);
  Rect goal{};bool found=false;
  for(auto node:grid.interactions)
    if((!task&&node.kind==InteractionKind::Secret&&node.id==id)||(task&&node.kind==InteractionKind::Task)){goal=node.area;found=true;}
  if(!found)return 3;
  goal.x-=1.5;goal.y-=.9;goal.w+=3;goal.h+=1.8;
  auto initial=new_state(grid);auto path=find_motion_path(grid,initial,goal);
  std::cout<<"ACCESS "<<id<<" reached="<<path.reached<<" expanded="<<path.expanded<<" states="<<path.states<<" nearest="<<path.nearest<<'\n';
  if(!path.reached)return 1;
  auto replay=initial;FixedStepper step;
  for(auto c:path.actions) {
    step.advance(replay,grid,c,.1);
    std::cout<<"INPUT 12 "<<c.move<<' '<<c.jump_pressed<<' '<<c.dash_pressed<<' '<<c.aim_y<<'\n';
  }
  bool reached=replay.player.rect().intersects(goal)&&replay.deaths==0;
  std::cout<<"REPLAY reached="<<reached<<" seconds="<<replay.elapsed<<" xy="<<replay.player.x<<','<<replay.player.y<<'\n';
  if(task&&reached) {
    auto returning=find_motion_path(grid,replay,grid.goal);
    for(auto c:returning.actions)step.advance(replay,grid,c,.1);
    std::cout<<"RETURN reached="<<returning.reached<<" won="<<replay.won<<" deaths="<<replay.deaths<<'\n';
    reached=returning.reached&&replay.won&&replay.deaths==0;
  }
  return reached?0:2;
}

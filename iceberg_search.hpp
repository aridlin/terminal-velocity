#pragma once
// Test-only best-first search. Every edge is twelve ordinary 120 Hz input
// ticks; reconstructed paths are replayed through the production session.
#include <queue>
#include <unordered_map>
namespace {
struct SearchNode {GameState state;int parent=-1;Controls action;int depth=0;};
struct SearchEntry {double priority;int node;bool operator<(const SearchEntry &other)const{return priority>other.priority;}};
uint64_t motion_key(const GameState &s) {
  uint64_t h=1469598103934665603ull;
  auto add=[&](int v){h^=uint32_t(v);h*=1099511628211ull;};
  const auto &p=s.player;
  add(int(p.x*2));add(int(p.y*2));add(int(std::round(p.vx/4)));add(int(std::round(p.vy/4)));
  add(p.grounded);add(p.dash_available);add(int(p.dash_time_left*20));add(int(p.momentum_left*10));
  add(int(std::fmod(s.elapsed,16.)*2));add(int(p.wall_lock_left*20));
  for(bool v:s.collected)add(v);
  return h;
}
struct MotionPath {bool reached=false;int expanded=0,states=0;double nearest=0;std::vector<Controls> actions;};
inline MotionPath find_motion_path(const EngineGrid &grid,const GameState &initial,Rect goal,
    const std::function<bool(const GameState &)> &accept={}) {
  auto distance=[&](const GameState &s){auto r=s.player.rect();double dx=std::max({goal.x-r.right(),r.x-goal.right(),0.});
    double dy=std::max({goal.y-r.bottom(),r.y-goal.bottom(),0.});return std::hypot(dx,dy*1.3);};
  std::vector<SearchNode> nodes;nodes.reserve(100000);nodes.push_back({initial,-1,{},0});
  std::priority_queue<SearchEntry> open;open.push({distance(nodes[0].state),0});
  std::unordered_map<uint64_t,int> best;best[motion_key(nodes[0].state)]=0;
  std::vector<Controls> actions;
  for(int move:{-1,0,1}) {
    Controls c;c.move=move;c.jump_held=true;actions.push_back(c);
    c.jump_pressed=true;actions.push_back(c);
    c.jump_pressed=false;c.dash_pressed=true;actions.push_back(c);
    c.aim_y=-1;actions.push_back(c);
  }
  int winner=-1,expanded=0;double nearest=1e9;
  while(!open.empty()&&nodes.size()<150000&&expanded<60000) {
    int index=open.top().node;open.pop();
    // Copy before emplacing into the vector, which can invalidate references.
    auto current=nodes[size_t(index)];++expanded;
    if(current.state.player.rect().intersects(goal)&&(!accept||accept(current.state))){winner=index;break;}
    if(current.depth>400)continue;
    for(auto action:actions) {
      const auto &p=current.state.player;
      if(action.dash_pressed&&!p.dash_available)continue;
      if(action.jump_pressed&&!p.grounded&&p.coyote_left<=0&&p.wall_coyote_left<=0&&p.dash_time_left<=0)continue;
      auto next=current.state;FixedStepper step;
      step.advance(next,grid,action,.1);
      if(next.deaths!=initial.deaths||(!grid.adventure_level&&next.won&&!next.player.rect().intersects(goal)))continue;
      auto key=motion_key(next);int depth=current.depth+1;
      auto found=best.find(key);if(found!=best.end()&&found->second<=depth)continue;
      best[key]=depth;double d=distance(next);nearest=std::min(nearest,d);
      nodes.push_back({std::move(next),index,action,depth});
      open.push({d+depth*.08,int(nodes.size()-1)});
    }
  }
  MotionPath result;result.expanded=expanded;result.states=int(nodes.size());result.nearest=nearest;
  if(winner<0)return result;
  result.reached=true;
  for(int at=winner;nodes[size_t(at)].parent>=0;at=nodes[size_t(at)].parent)result.actions.push_back(nodes[size_t(at)].action);
  std::reverse(result.actions.begin(),result.actions.end());return result;
}

} // namespace

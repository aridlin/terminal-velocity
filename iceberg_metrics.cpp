#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <set>
int main(int argc,char **argv) {
  int seeds=argc>1?std::max(1,std::atoi(argv[1])):10000;
  std::set<std::vector<int>> routes;
  std::array<int,6> rhythms{};std::array<int,4> difficulties{};
  int repeated=0,max_run=0,room_total=0,branch_rooms=0;
  double climb=0,drop=0;
  std::cout<<"{\n  \"structural_seeds\": "<<seeds<<",\n  \"templates\": [\n";
  bool first=true;
  for(const auto &p:iceworld::patterns()) {
    int turns=0,moving=0,previous=0;double ascent=0,descent=0;
    for(size_t i=0;i<p.path.size();++i) {
      moving+=p.path[i].kind=='m';
      if(!i)continue;
      auto a=p.path[i-1],b=p.path[i];
      ascent+=std::max(0,a.y-b.y);descent+=std::max(0,b.y-a.y);
      int direction=b.x>a.x?1:-1;
      if(previous&&direction!=previous)++turns;
      previous=direction;
    }
    if(!first)std::cout<<",\n";
    first=false;
    auto meta=iceworld::describe(p);
    std::cout<<"    {\"id\":"<<p.id<<",\"district\":"<<p.id/12<<",\"main_landings\":"<<p.path.size()
      <<",\"optional_landings\":"<<p.optional.size()<<",\"moving_landings\":"<<moving
      <<",\"ascent\":"<<ascent<<",\"descent\":"<<descent<<",\"direction_changes\":"<<turns
      <<",\"difficulty\":"<<meta.difficulty<<",\"safe_rest_points\":"<<meta.resting_points.size()<<"}";
  }
  for(int n=0;n<seeds;++n) {
    auto route=iceworld::make_itinerary(unsigned(n),n%iceberg::AuthoredDistrictCount);routes.insert(route);
    int previous=-1,run=0;
    for(auto id:route) {
      const auto &p=iceworld::pattern(id);auto meta=iceworld::describe(p);++room_total;
      ++rhythms[size_t(p.rhythm)];++difficulties[size_t(meta.difficulty)];branch_rooms+=!p.optional.empty();
      run=int(p.rhythm)==previous?run+1:1;repeated+=run>1;max_run=std::max(max_run,run);previous=int(p.rhythm);
      for(size_t i=1;i<p.path.size();++i){climb+=std::max(0,p.path[i-1].y-p.path[i].y);drop+=std::max(0,p.path[i].y-p.path[i-1].y);}
    }
  }
  std::cout<<"\n  ],\n  \"distinct_room_orders\": "<<routes.size()<<",\n  \"assembled_rooms\": "<<room_total
    <<",\n  \"rooms_with_optional_branches\": "<<branch_rooms<<",\n  \"adjacent_repeated_rhythm\": "<<repeated
    <<",\n  \"maximum_same_rhythm_run\": "<<max_run<<",\n  \"mean_ascent_per_room\": "<<climb/room_total
    <<",\n  \"mean_descent_per_room\": "<<drop/room_total<<",\n  \"rhythm_counts\": [";
  for(size_t i=0;i<rhythms.size();++i)std::cout<<(i?",":"")<<rhythms[i];
  std::cout<<"],\n  \"difficulty_counts\": [";
  for(size_t i=1;i<difficulties.size();++i)std::cout<<(i>1?",":"")<<difficulties[i];
  std::cout<<"]\n}\n";
}

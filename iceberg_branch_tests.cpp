#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
int main(int argc,char **argv) {
  int variants=argc>1?std::max(1,std::atoi(argv[1])):1;
  try {
    int routes=0,ledges=0;
    for(int variant=0;variant<variants;++variant)for(bool expert:{false,true})
      for(const auto &pattern:iceworld::patterns()) {
        uint32_t seed=variant==0?73:iceberg::mix(unsigned(variant*149));
        auto grid=verified_room(pattern.id,seed,iceworld::district_of(pattern),expert);
        if(grid.template_id!=pattern.id)throw std::runtime_error("template unexpectedly replaced: requested="+std::to_string(pattern.id)+" actual="+std::to_string(grid.template_id)+" expert="+std::to_string(expert)+" seed="+std::to_string(seed));
        auto state=new_state(grid);FixedStepper stepper;
        auto reach=[&](Rect target,bool landing) {
          auto path=find_motion_path(grid,state,target,[&](const GameState &s){return !landing||s.player.grounded;});
          if(!path.reached)throw std::runtime_error("branch search failed id="+std::to_string(pattern.id)+
            " expert="+std::to_string(expert)+" seed="+std::to_string(seed)+" target="+std::to_string(target.x)+","+std::to_string(target.y));
          for(auto c:path.actions)stepper.advance(state,grid,c,.1);
          if(state.deaths||!state.player.rect().intersects(target))throw std::runtime_error("branch replay mismatch");
        };
        for(auto pad:pattern.optional) {
          reach({pad.x+1.,pad.y-kPlayerHeight,pad.w-2.,kPlayerHeight+.1},true);++ledges;
        }
        reach(grid.goal,false);
        if(!state.won)throw std::runtime_error("branch return did not finish");
        ++routes;std::cout<<"PASS id="<<pattern.id<<" expert="<<expert<<" seed="<<seed<<" branches="<<pattern.optional.size()<<'\n';
      }
    std::cout<<"PASS "<<routes<<" optional routes, "<<ledges<<" ledges and all return paths through ordinary inputs\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

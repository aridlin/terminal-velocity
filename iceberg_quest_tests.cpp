#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
void require_quest(bool okay,const char *message){if(!okay)throw std::runtime_error(message);std::cout<<"PASS "<<message<<'\n';}
Rect reach_area(Rect goal){goal.x-=1.5;goal.y-=.9;goal.w+=3;goal.h+=1.8;return goal;}
void install_test_room(AdventureSession &game,EngineGrid grid,int stage=2) {
  game.grid=std::move(grid);game.district=game.grid.district;game.stage=stage;game.room=0;
  game.state=new_state(game.grid);game.stepper={};game.sync_view();
}
void walk_to(AdventureSession &game,Rect goal) {
  auto path=find_motion_path(game.grid,game.state,goal);
  require_quest(path.reached,"ordinary-input path found");
  int deaths=game.state.deaths;
  for(auto c:path.actions)game.tick(c,.1);
  require_quest(game.state.deaths==deaths&&game.state.player.rect().intersects(goal),"path replays through production session without teleporting or damage");
}
void interact(AdventureSession &game){Controls c;c.interact=true;game.tick(c,1./120);}
void finish_dialogue(AdventureSession &game){for(int i=0;i<20&&game.state.adventure.dialogue.active();++i){Controls c;c.confirm=true;game.tick(c,1./120);}}
int main() {
  try {
    iceberg::SessionOptions options;options.no_save=true;options.mute=true;options.seed=73;
    AdventureSession game(options,std::filesystem::temp_directory_path()/"tv-quest-nosave");game.begin_campaign(true);
    {
      AdventureSession inspected(options,std::filesystem::temp_directory_path()/"tv-inspect-nosave");
      inspected.begin_campaign(true);install_test_room(inspected,iceworld::build_room(0,73,0));
      auto object=inspected.grid.interactions.front();
      inspected.state.player.x=object.area.x;inspected.state.player.y=object.area.y;
      auto original_items=inspected.profile.task_items;double elapsed=inspected.state.elapsed;
      Controls examine;examine.inspect=true;inspected.tick(examine,.1);
      require_quest(inspected.state.adventure.inspecting&&inspected.profile.task_items==original_items,
        "inspection opens without collecting or modifying a task");
      Controls move;move.move=1;move.jump_pressed=true;inspected.tick(move,.1);
      require_quest(inspected.state.elapsed==elapsed,"inspection freezes simulation and movement input");
      inspected.tick(examine,.1);require_quest(!inspected.state.adventure.inspecting,"inspection closes without using the object");
      interact(inspected);require_quest(inspected.state.adventure.pickup_left>0&&inspected.profile.task_items!=original_items,
        "parcel collection starts a visible lift animation and persists exactly once");
      auto items=inspected.profile.task_items;interact(inspected);
      require_quest(inspected.profile.task_items==items,"repeated interaction cannot duplicate a parcel");
      Controls pause;pause.pause=true;inspected.tick(pause,.1);double animation=inspected.state.adventure.pickup_left;
      inspected.tick({},.1);require_quest(inspected.state.adventure.pickup_left==animation,"pickup animation respects pause");
      inspected.tick(pause,.1);for(int i=0;i<8;++i)inspected.tick({},.1);
      require_quest(inspected.state.adventure.pickup_left==0,"pickup animation ends without locking controls");
      install_test_room(inspected,iceworld::build_settlement(0));
      auto hidden=std::find_if(inspected.grid.interactions.begin(),inspected.grid.interactions.end(),[](const Interaction &n){return n.kind==InteractionKind::Secret;});
      inspected.state.player.x=hidden->area.x;inspected.state.player.y=hidden->area.y;
      inspected.tick(examine,.1);require_quest(!inspected.state.adventure.inspecting&&inspected.profile.hidden_room<0,
        "inspection does not reveal or enter hidden entrances");
    }
    {
      using iceberg::Achievement;
      using iceberg::SideStory;
      AdventureSession side(options,{});
      side.begin_campaign(true);
      side.profile.story=63;
      side.load_room();
      auto find=[&](InteractionKind kind,int id) {
        const auto node=std::find_if(side.grid.interactions.begin(),side.grid.interactions.end(),
          [&](const Interaction &entry){return entry.kind==kind&&entry.id==id;});
        if(node==side.grid.interactions.end())throw std::runtime_error("missing side story interaction");
        return reach_area(node->area);
      };
      auto use=[&](InteractionKind kind,int id) {walk_to(side,find(kind,id));interact(side);};
      auto scene=[&](SideStory id){use(InteractionKind::SideStory,int(id));};
      auto earned=[&](Achievement id){return (side.profile.discoveries&iceberg::achievement_bit(id))!=0;};
      const auto read_document=[&](SideStory id,iceberg::SideFlag flag) {
        walk_to(side,find(InteractionKind::SideStory,int(id)));
        const auto record=side.side_document(id);
        const double elapsed=side.state.elapsed;
        for(bool copy:{false,true}) {
          side.tick(copy?Controls{.interact=true}:Controls{.inspect=true},.1,true,34,24);
          require_quest(side.state.adventure.inspecting&&side.has_side_flag(flag)==copy&&
            side.state.adventure.inspect_title==record->title.get(side.profile.language)&&
            side.state.adventure.inspect_body.starts_with(record->body.get(side.profile.language)),
            "the complete document is available before and after copying at the smallest viewport");
          side.tick({.move=1},.1,true,34,24);
          require_quest(side.state.elapsed==elapsed,"reading a clue freezes physics even on the opening frame");
          side.tick({.inspect=true},.1,true,34,24);
        }
      };
      read_document(SideStory::HarborStub,iceberg::SideFlag::HarborStub);
      side.district=2;side.stage=0;side.load_room();
      const auto basement=iceberg::UnnumberedFloor;
      const auto entrance=find(InteractionKind::Secret,basement);
      use(InteractionKind::Secret,basement);
      require_quest(side.profile.hidden_room==-1&&!earned(Achievement::LostFloor),
        "one clue leaves the optional stairwell locked");
      read_document(SideStory::EstatePlate,iceberg::SideFlag::EstatePlate);
      use(InteractionKind::Secret,basement);
      require_quest(side.profile.hidden_room==basement&&earned(Achievement::LostFloor),
        "matching clues open the optional stairwell");
      const double district_time=side.profile.district_seconds;
      const double journey_time=side.profile.campaign_seconds;
      const int district_retries=side.profile.district_deaths;
      const int journey_retries=side.profile.campaign_deaths;
      const auto &live_cable=side.grid.pulses.front();
      side.state.elapsed=(1.2-live_cable.phase)*live_cable.period;
      side.state.player.x=live_cable.area.x+2.;
      side.state.player.y=live_cable.area.bottom()-kPlayerHeight;
      side.tick({},1./120);
      require_quest(side.state.deaths==1&&side.profile.campaign_deaths==journey_retries+1&&
        side.profile.campaign_seconds>journey_time&&earned(Achievement::FirstFall),
        "a real basement hit counts in the room and journey and earns the first setback");
      require_quest(side.profile.district_deaths==district_retries&&side.profile.district_seconds==district_time,
        "a hidden room does not change the ongoing district record");
      scene(SideStory::Intercom);finish_dialogue(side);
      scene(SideStory::Tape);
      require_quest(!earned(Achievement::WitnessTape),"starting the tape leaves the award pending");
      finish_dialogue(side);
      require_quest(earned(Achievement::WitnessTape),"finishing the tape earns the award");
      const auto charge_probe=[&]() {
        auto probe=new_state(side.grid);
        const auto &arc=side.grid.pulses.front();
        probe.elapsed=(1.2-arc.phase)*arc.period;
        probe.player.x=arc.area.x+2.;
        probe.player.y=arc.area.bottom()-kPlayerHeight;
        FixedStepper stepper;
        stepper.advance(probe,side.grid,{},1./120);
        return probe.deaths;
      };
      require_quest(charge_probe()==1,"the exposed cable damages the courier while energized");
      scene(SideStory::BasementBreaker);
      require_quest(side.has_side_flag(iceberg::SideFlag::BasementPowerCut)&&charge_probe()==0,
        "the breaker cuts collision damage as well as the basement lights");
      side.load_room(true);
      require_quest(charge_probe()==0,"the breaker stays off when the basement is reloaded");
      scene(SideStory::BasementBreaker);
      require_quest(!side.has_side_flag(iceberg::SideFlag::BasementPowerCut)&&charge_probe()==1,
        "restoring power reactivates the cable in the same room");
      scene(SideStory::BasementBreaker);
      read_document(SideStory::Roster,iceberg::SideFlag::Roster);
      require_quest(!earned(Achievement::BelowTheRoster),"copying the roster leaves its return award pending");
      const auto return_shaft=std::find_if(side.grid.interactions.rbegin(),side.grid.interactions.rend(),
        [](const Interaction &node){return node.kind==InteractionKind::Return;});
      walk_to(side,reach_area(return_shaft->area));interact(side);
      require_quest(side.profile.hidden_room<0&&side.state.player.rect().intersects(entrance)&&earned(Achievement::BelowTheRoster),
        "the service shaft returns to the same Estate entrance and awards the roster");
      require_quest(side.profile.district_deaths==district_retries&&side.profile.district_seconds==district_time,
        "leaving a detour restores the same clean district attempt");
      side.tick({.restart=true},1./120);
      require_quest(side.profile.district_deaths==district_retries+1&&side.profile.district_seconds>district_time,
        "retrying on the regular district route still affects its record");
      const auto before_choice=side.profile;
      for(int option:{0,1}) {
        using iceberg::SideFlag;
        side.profile=before_choice;
        side.load_room(true);
        scene(SideStory::RosterDesk);
        auto &dialogue=side.state.adventure.dialogue;
        for(int i=0;i<20&&dialogue.active()&&!dialogue.choosing();++i)side.tick({.confirm=true},0.);
        require_quest(dialogue.choosing()&&dialogue.reply_scene==iceberg::EstateRosterReplyScene,
          "the returned roster opens a choice with Grażyna");
        if(option)side.tick({.navigate=1},0.);
        side.tick({.confirm=true},0.);
        require_quest(side.has_side_flag(SideFlag::RosterPosted)==(option==0)&&
          side.has_side_flag(SideFlag::RosterSealed)==(option==1)&&side.profile.story==before_choice.story,
          "both roster choices persist without changing main story progression");
        finish_dialogue(side);
        side.load_room(true);
        scene(SideStory::RosterDesk);
        require_quest(side.state.adventure.inspecting&&side.state.adventure.inspect_title==
          (option==0?side.t("Sprostowanie","Correction notice"):side.t("Zapieczętowany odpis","Sealed copy")),
          "revisiting the desk shows the chosen record instead of repeating the exchange");
        side.tick({.inspect=true},0.);
      }
      scene(SideStory::Casting);
      use(InteractionKind::Npc,3);
      require_quest(earned(Achievement::SiliconeReturn)&&side.state.adventure.dialogue.lines.front().speaker==3,
        "the Estate rooftop prop gets a unique response from Grażyna");
      finish_dialogue(side);
      side.district=3;side.stage=0;side.load_room();
      scene(SideStory::EmergencyBell);
      require_quest(!earned(Achievement::RingAgain),"the first bell press gives the warning");
      interact(side);
      require_quest(earned(Achievement::RingAgain),"ringing again after the warning earns the joke award");
    }
    game.profile.story=63;
    for(int q:{0,1,2,3,4,7})game.profile.quests[size_t(q)]=1;
    for(int id:{0,4,8,12,16,20,13,17,21,24,28,32,26,30,34,36,40,44}) {
      install_test_room(game,verified_room(id,73,id/12));
      Interaction target{};bool found=false;
      for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Task){target=node;found=true;}
      require_quest(found,"quest location reserved in assembled room");
      walk_to(game,reach_area(target.area));interact(game);
      unsigned count=game.profile.quest_counts[size_t(target.id)];interact(game);
      require_quest(game.profile.quest_counts[size_t(target.id)]==count,"task interaction cannot duplicate a collected objective");
      walk_to(game,game.grid.goal);
      require_quest(game.state.won,"optional branch has a playable return to the main exit");
    }
    for(int quest:{0,1,2,3,4,7}) {
      require_quest(game.profile.quests[size_t(quest)]==2,"all three unique objectives make the task ready");
      install_test_room(game,iceworld::build_settlement(iceberg::Quests[size_t(quest)].district),3);
      Rect goal{};for(auto node:game.grid.interactions)
        if(node.kind==InteractionKind::Npc&&node.id==iceberg::Quests[size_t(quest)].owner)goal=reach_area(node.area);
      walk_to(game,goal);interact(game);finish_dialogue(game);
      require_quest(game.profile.quests[size_t(quest)]==3&&(game.profile.cosmetics&(1u<<(quest==5?5:quest%7+1))),"character turn-in grants its persistent reward");
    }
    {
      AdventureSession recovered(options,std::filesystem::temp_directory_path()/"tv-recovery-quest-nosave");
      recovered.begin_campaign(true);recovered.profile.story=63;
      for(int q:{0,1,2,3,4,7})recovered.profile.quests[size_t(q)]=1;
      for(int id:{0,4,8,12,16,20,13,17,21,24,28,32,26,30,34,36,40,44}) {
        auto original=iceworld::build_room(id,73,id/12);
        install_test_room(recovered,recovery_room_for(original));
        auto task=std::find_if(recovered.grid.interactions.begin(),recovered.grid.interactions.end(),
          [](const Interaction &node){return node.kind==InteractionKind::Task;});
        require_quest(task!=recovered.grid.interactions.end()&&task->id==original.interactions[0].id&&task->detail==original.interactions[0].detail,
          "generic recovery preserves the reserved task identity");
        walk_to(recovered,reach_area(task->area));interact(recovered);walk_to(recovered,recovered.grid.goal);
        require_quest(recovered.state.won,"recovery task ledge has a playable exit");
      }
      for(int q:{0,1,2,3,4,7})require_quest(recovered.profile.quests[size_t(q)]==2,
        "character task remains completable even if all its rooms use recovery routes");
    }
    EngineGrid practice(200,60);practice.adventure_level=true;practice.event_level=false;
    practice.add_solid_rect(0,50,200,10);practice.spawn_x=4;practice.spawn_y=50-kPlayerHeight;practice.goal={190,45,2,5};
    install_test_room(game,std::move(practice));game.profile.quests[6]=1;game.profile.quest_counts[6]=0;
    game.tick({},1./120);
    for(int attempt=0;attempt<3;++attempt) {
      for(int tick=0;tick<30&&!game.state.player.can_dash();++tick)game.tick({},1./120);
      Controls c;c.move=1;c.dash_pressed=true;game.tick(c,1./120);
      c.dash_pressed=false;c.jump_pressed=c.jump_held=true;game.tick(c,1./120);
      c.jump_pressed=false;for(int tick=0;tick<160&&!game.state.player.grounded;++tick)game.tick(c,1./120);
    }
    require_quest(game.profile.quest_counts[6]==3&&game.profile.quests[6]==2,"Kuba counts three real momentum jumps");
    install_test_room(game,iceworld::build_settlement(0),3);
    Rect kuba{};for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Npc&&node.id==6)kuba=reach_area(node.area);
    walk_to(game,kuba);interact(game);finish_dialogue(game);
    require_quest(game.profile.quests[6]==3,"mastery task can be turned in on the upper settlement street");
    game.profile.story|=(uint64_t(1)<<8)|(uint64_t(1)<<9);
    for(int district=0;district<2;++district)for(int scene=0;scene<2;++scene) {
      game.district=district;game.stage=3;game.room=0;game.profile.revisit={};
      game.load_room();game.persist();
      Rect door{};bool found=false;
      for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Replay&&node.id==scene){door=reach_area(node.area);found=true;}
      require_quest(found,"completed district exposes an authored route revisit");
      walk_to(game,door);interact(game);
      require_quest(game.profile.revisit.active&&game.grid.room_role==(scene?iceberg::RoomRole::Finale:iceberg::RoomRole::Lesson),"revisit door enters the chosen authored scene");
      Rect departure{};
      for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Return)departure=reach_area(node.area);
      walk_to(game,departure);interact(game);
      require_quest(!game.profile.revisit.active&&game.grid.room_role==iceberg::RoomRole::Settlement&&
        game.state.player.rect().intersects(door),"return door places the courier back at the original entrance");
      const int nearby=game.near_interaction();
      require_quest(nearby>=0&&game.grid.interactions[size_t(nearby)].kind==InteractionKind::Replay&&
        game.grid.interactions[size_t(nearby)].id==scene,"returning preserves the same doorway as the nearby interaction");
    }
    for(int id=0;id<3;++id) {
      game.profile.hidden_room=id;install_test_room(game,iceworld::build_hidden(id));
      Rect note{},door{};
      for(auto node:game.grid.interactions){if(node.kind==InteractionKind::Note)note=reach_area(node.area);if(node.kind==InteractionKind::Return)door=reach_area(node.area);}
      walk_to(game,note);interact(game);finish_dialogue(game);
      require_quest(game.profile.discoveries&(1u<<id),"hidden testimony persists independently of campaign completion");
      walk_to(game,door);
      // The unit fixture has no underlying campaign route; return physics is
      // verified here and the campaign checkpoint restoration has its own test.
      game.profile.hidden_room=-1;
    }
    game.state.journal=true;game.state.adventure.journal_tab=1;game.state.adventure.selected=0;game.profile.quests[0]=3;game.sync_view();
    double journal_x=game.state.player.x;interact(game);
    require_quest(game.state.adventure.dialogue.lines.size()==3,"completed mail task unlocks actual letters through the journal controls");
    finish_dialogue(game);
    require_quest(game.state.journal&&game.state.player.x==journal_x,"reading the reward returns to the journal without moving the courier");
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}

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
void walk_to(AdventureSession &game,Rect goal,const std::function<bool(const GameState &)> &accept={}) {
  auto path=find_motion_path(game.grid,game.state,goal,accept);
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
      using iceberg::Achievement;
      AdventureSession rider(options,{});
      rider.begin_campaign(true);
      const std::array rides{
        std::pair{&MovingPlatform::cargo_hoist,Achievement::CargoLift},
        std::pair{&MovingPlatform::pressure_piston,Achievement::PressureLift},
        std::pair{&MovingPlatform::chute_leaf,Achievement::LeafFlume},
        std::pair{&MovingPlatform::archive_rack,Achievement::ArchiveRide}
      };
      for(const auto &[fixture,award]:rides) {
        EngineGrid g(40,28);
        g.adventure_level=true;g.spawn_x=12;g.spawn_y=8;
        g.add_solid_rect(0,25,40,3);
        g.platforms.push_back({.base={8,12,10,.5},.dy=.5,.period=4.});
        g.platforms.front().*fixture=true;
        g.goal={36,23,1,2};
        install_test_room(rider,std::move(g));
        const auto earned=[&] {
          return iceberg::has_award(iceberg::stored_award_progress(rider.profile),award);
        };
        require_quest(!earned(),"being in a room with a ride does not earn its award");
        for(int i=0;i<60&&rider.state.player.support<0;++i)rider.tick({},1./120);
        require_quest(rider.state.player.support==0&&earned(),"actually landing on the deck earns its ride award");
      }
      for(bool moving:{false,true})for(double frame:{1./120,.1}) {
        rider.profile.task_items[1]&=~iceberg::achievement_bit(Achievement::DashingHeadfirst);
        EngineGrid hazard(40,28);
        hazard.adventure_level=true;hazard.spawn_x=12.85;hazard.spawn_y=25-kPlayerHeight;
        hazard.add_solid_rect(0,25,40,3);
        if(moving)hazard.pulses.push_back({{14,22,3,3}});
        else hazard.add_hazard_rect(14,22,3,3);
        hazard.goal={36,23,1,2};
        install_test_room(rider,std::move(hazard));
        rider.tick({.move=1,.dash_pressed=true},frame);
        require_quest(rider.state.deaths==1&&iceberg::has_award(
          iceberg::stored_award_progress(rider.profile),Achievement::DashingHeadfirst),
          "first-step dash contact earns Headfirst at either frame rate");
      }
      rider.profile.task_items[1]&=~iceberg::achievement_bit(Achievement::DashingHeadfirst);
      EngineGrid drop(40,28);
      drop.adventure_level=true;drop.spawn_x=12;drop.spawn_y=32.85;
      drop.goal={36,23,1,2};
      install_test_room(rider,std::move(drop));
      rider.tick({.dash_pressed=true,.aim_y=1},.05);
      require_quest(rider.state.deaths==1&&!iceberg::has_award(
        iceberg::stored_award_progress(rider.profile),Achievement::DashingHeadfirst),
        "dashing out of bounds without touching a hazard does not earn Headfirst");
    }
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
      auto earned=[&](Achievement id){return (iceberg::stored_achievements(side.profile)&iceberg::achievement_bit(id))!=0;};
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
      const Rect shaft_area=reach_area(return_shaft->area);
      walk_to(side,shaft_area);interact(side);
      require_quest(side.profile.hidden_room<0&&side.state.player.rect().intersects(entrance)&&earned(Achievement::BelowTheRoster),
        "the service shaft returns to the same Estate entrance and awards the roster");
      require_quest(side.profile.district_deaths==district_retries&&side.profile.district_seconds==district_time,
        "leaving a detour restores the same clean district attempt");
      const auto before_line=side.profile;
      side.district=3;side.stage=0;side.load_room();
      read_document(SideStory::ArchiveCopy,iceberg::SideFlag::ArchiveCopy);
      require_quest(earned(Achievement::PaperTrail),"following all three clues earns the paper trail award");
      scene(SideStory::ArchiveLine);
      require_quest(side.state.adventure.dialogue.active()&&earned(Achievement::NightLine),
        "the basement intercom makes the unplugged receiver answer in the Archive");
      finish_dialogue(side);
      side.district=2;side.stage=0;side.load_room();
      walk_to(side,{67,44,1,4});
      interact(side);
      require_quest(side.profile.hidden_room==basement,"the same basement entrance remains reachable after the Archive call");
      scene(SideStory::NightWindow);
      require_quest(earned(Achievement::LastPassenger)&&side.state.adventure.night_beat==1&&
        side.state.adventure.night_left>0.,"the dark observation window reveals the passenger after the call");
      const auto return_side=Rect{99,46,4,5};
      walk_to(side,return_side);
      require_quest(side.state.adventure.night_beat>=2,
        "moving away from the observation window brings the passenger closer");
      auto &night=side.state.adventure;
      side.tick({.pause=true},.05);
      const double paused_scale=night.night_plate_time;
      for(int i=0;i<30;++i)side.tick({},.1);
      require_quest(night.night_plate_time==paused_scale&&!earned(Achievement::NightWeight),
        "paused time does not weigh the courier");
      side.tick({.pause=true},.05);
      walk_to(side,{99,47,4.,4.},[](const GameState &state) {
        const auto &p=state.player;
        return p.grounded&&std::abs(p.vx)<.5&&p.x>=99.6&&p.x<102.;
      });
      for(int i=0;i<30;++i)side.tick({},.1);
      require_quest(earned(Achievement::NightWeight)&&side.has_curio(iceberg::Curio::NightPlateRecorded),
        "standing on the scale with power cut prints a second reading and earns the hidden award");
      interact(side);
      require_quest(night.inspecting&&night.inspect_body.find(side.t("OCZEKUJE","WAITING"))!=std::string::npos,
        "the completed scale prints a permanent receipt");
      side.tick({.inspect=true},0.);
      auto path=std::filesystem::temp_directory_path()/"tv-extended-awards.iceberg";
      iceberg::SaveStore save(path);
      iceberg::Profile loaded;
      require_quest(save.save(side.profile)&&save.load(loaded)&&loaded.task_items==side.profile.task_items&&
        (iceberg::stored_achievements(loaded)&iceberg::achievement_bit(Achievement::LastPassenger)),
        "old and extended awards and encounter state round-trip through the existing TVICE6 format");
      walk_to(side,shaft_area);interact(side);
      require_quest(side.profile.hidden_room<0,"the measured route can still return through the shaft");
      side.district=3;side.stage=0;side.load_room();
      scene(SideStory::CarbonReader);
      require_quest(!earned(Achievement::SecondImpression)&&
                    !side.has_side_flag(iceberg::SideFlag::ArchiveCarbonPrinted),
        "the reader will not print from the initial settings");
      for(int dial=0;dial<3;++dial) {
        const auto control=std::find_if(side.grid.interactions.begin(),side.grid.interactions.end(),[dial](const Interaction &node) {
          return node.kind==InteractionKind::SideStory&&node.id==int(SideStory::CarbonDial)&&node.detail==dial;
        });
        require_quest(control!=side.grid.interactions.end(),"each carbon reader drum has its own interaction");
        walk_to(side,control->area);
        for(unsigned press=0;press<iceberg::CarbonReaderCode[size_t(dial)];++press)interact(side);
      }
      scene(SideStory::CarbonReader);
      require_quest(earned(Achievement::SecondImpression)&&
                    side.has_side_flag(iceberg::SideFlag::ArchiveCarbonPrinted),
        "the receipt settings unlock the physical copy and a hidden award");
      side.load_room();
      scene(SideStory::CarbonReader);
      require_quest(side.state.adventure.inspecting&&
                    side.state.adventure.inspect_body==iceberg::CarbonReaderRecord.get(side.profile.language),
        "returning to the machine reveals the completed carbon copy");
      side.tick({.inspect=true},0.);
      using iceberg::SideFlag;
      const auto hatch=std::find_if(side.grid.interactions.begin(),side.grid.interactions.end(),
        [](const auto &node){return node.kind==InteractionKind::Secret&&node.id==iceberg::NightPlatform;});
      require_quest(hatch!=side.grid.interactions.end(),"printing the copy reveals a physical exit from the Archive");
      const auto hatch_area=hatch->area;
      walk_to(side,hatch_area);
      interact(side);
      const auto has_new_award=[&](Achievement award) {
        return iceberg::has_award(iceberg::stored_award_progress(side.profile),award);
      };
      require_quest(side.grid.template_id==iceberg::NightPlatformTemplate&&has_new_award(Achievement::PlatformFound),
        "cabinet 11 opens a secret waiting room");
      scene(SideStory::PlatformSignal);
      require_quest(!has_new_award(Achievement::PlatformReleased),
        "the departure signal waits for an answer from the bench");
      const auto bench=std::find_if(side.grid.interactions.begin(),side.grid.interactions.end(),
        [](const auto &node){return node.kind==InteractionKind::SideStory&&node.id==int(SideStory::PlatformBench);});
      walk_to(side,bench->area,[](const auto &state) {
        return state.player.grounded&&state.player.x>=34.&&state.player.x<=37.&&std::abs(state.player.vx)<1.;
      });
      interact(side);
      require_quest(side.state.adventure.platform_listening,"sitting starts the delayed response");
      side.tick({.pause=true},.05);
      const double paused_wait=side.state.adventure.platform_wait;
      for(int i=0;i<45;++i)side.tick({},.1);
      require_quest(side.state.adventure.platform_wait==paused_wait&&!has_new_award(Achievement::PlatformEcho),
        "pause does not advance the waiting room encounter");
      side.tick({.pause=true},.05);
      for(int i=0;i<45;++i)side.tick({},.1);
      require_quest(has_new_award(Achievement::PlatformEcho)&&side.has_side_flag(SideFlag::PlatformWaited),
        "waiting earns the reflection discovery");
      scene(SideStory::PlatformSignal);
      require_quest(has_new_award(Achievement::PlatformReleased)&&side.state.adventure.platform_departure>0.,
        "the signal sends the reflected passenger home");
      require_quest(save.save(side.profile)&&save.load(loaded)&&loaded.hidden_room==iceberg::NightPlatform&&
        iceberg::has_award(iceberg::stored_award_progress(loaded),Achievement::PlatformReleased),
        "the detour and expanded catalogue survive a save round trip");
      scene(SideStory::PlatformShutter);
      require_quest(!side.state.adventure.platform_shutter_open,
        "the lost-property shutter stays locked while the train departs");
      for(int i=0;i<45;++i)side.tick({},.1);
      auto near_shutter=find(InteractionKind::SideStory,int(SideStory::PlatformShutter));
      walk_to(side,near_shutter,[](const auto &state) {
        return state.player.grounded&&std::abs(state.player.x-iceberg::PlatformShutterX)<1.8&&std::abs(state.player.vx)<1.;
      });
      interact(side);
      require_quest(side.state.adventure.platform_shutter_open&&!has_new_award(Achievement::LastCoat),
        "the shutter opens after the last service without immediately revealing its contents");
      for(int i=0;i<12;++i)side.tick({},.1);
      side.tick({.pause=true},.05);
      const auto shutter_time=side.state.adventure.platform_shutter_time;
      for(int i=0;i<40;++i)side.tick({},.1);
      require_quest(side.state.adventure.platform_shutter_time==shutter_time,
        "pausing preserves the lost-property encounter beat");
      side.tick({.pause=true},.05);
      walk_to(side,reach_area({51,44,1,4}));
      require_quest(side.state.adventure.platform_shutter_time==0.,
        "walking away before the figure arrives resets its approach");
      walk_to(side,near_shutter,[](const auto &state) {
        return state.player.grounded&&std::abs(state.player.x-iceberg::PlatformShutterX)<1.8&&std::abs(state.player.vx)<1.;
      });
      for(int i=0;i<40;++i)side.tick({},.1);
      interact(side);
      finish_dialogue(side);
      require_quest(has_new_award(Achievement::LastCoat)&&side.has_side_flag(SideFlag::LastCoatFound),
        "the coat is collected by choice after waiting");
      require_quest(save.save(side.profile)&&save.load(loaded)&&side.profile.task_items==loaded.task_items,
        "the coat and the new award bits survive TVICE6 serialization");
      const auto platform_exit=side.grid.interactions.front().area;
      walk_to(side,platform_exit);interact(side);
      require_quest(side.profile.hidden_room<0&&side.state.player.rect().intersects(hatch_area),
        "the waiting room exit returns to the cabinet in the Archive");
      require_quest(side.state.adventure.platform_reflection.empty()&&
        !side.state.adventure.platform_listening&&side.state.adventure.platform_departure==0.,
        "leaving the waiting room clears its transient presentation");
      scene(SideStory::ArchiveLostProperty);
      finish_dialogue(side);
      require_quest(has_new_award(Achievement::HeldForCollection)&&side.has_side_flag(SideFlag::LastCoatFiled),
        "returning the coat to the Archive completes the secret without changing the route");
      side.district=2;side.stage=0;side.load_room(true);
      scene(SideStory::EstateDoormat);
      require_quest(side.state.adventure.notice.find("Zosia")!=std::string::npos,
        "the returned coat changes what can be found at the Estate doormat");
      side.district=4;side.stage=0;side.load_room(true);
      scene(SideStory::GardenSeedPod);
      require_quest(has_new_award(Achievement::SeedShower)&&side.state.adventure.seed_pod_left>0.,
        "shaking the optional seed pod produces a burst and a playful achievement");
      side.profile=before_line;side.district=2;side.stage=0;side.load_room(true);
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
    {
      using iceberg::SideStory;
      using iceberg::SideFlag;
      AdventureSession heat(options,{});
      heat.begin_campaign(true);
      heat.profile.story=63;heat.district=1;heat.stage=0;heat.load_room();
      const auto target=[&](SideStory id) {
        const auto node=std::find_if(heat.grid.interactions.begin(),heat.grid.interactions.end(),
          [id](const Interaction &entry){return entry.kind==InteractionKind::SideStory&&entry.id==int(id);});
        if(node==heat.grid.interactions.end())throw std::runtime_error("missing heatworks prop");
        return *node;
      };
      require_quest(!heat.side_story_available(target(SideStory::HeatworksMugBench)),
        "Witek's bench does not displace the opening lamp or briefing");
      walk_to(heat,reach_area(target(SideStory::HeatworksMug).area));
      heat.tick({.inspect=true},0.);
      require_quest(heat.state.adventure.inspecting&&!heat.has_side_flag(SideFlag::HeatworksMugTaken),
        "inspecting the mug reads its note without picking it up");
      heat.tick({.inspect=true},0.);
      interact(heat);
      require_quest(heat.has_side_flag(SideFlag::HeatworksMugTaken)&&
        !heat.side_story_available(target(SideStory::HeatworksMug)),
        "the lifted mug is carried and cannot be picked up twice");
      const auto carried=heat.profile;
      for(int option:{0,1}) {
        heat.profile=carried;heat.load_room(true);
        walk_to(heat,reach_area(target(SideStory::HeatworksMugBench).area));interact(heat);
        auto &dialogue=heat.state.adventure.dialogue;
        for(int i=0;i<20&&dialogue.active()&&!dialogue.choosing();++i)heat.tick({.confirm=true},0.);
        require_quest(dialogue.choosing()&&dialogue.reply_scene==iceberg::HeatworksMugReplyScene&&
          !heat.has_side_flag(SideFlag::HeatworksMugReturned),"the mug remains carried until the courier replies");
        if(option)heat.tick({.navigate=1},0.);
        heat.tick({.confirm=true},0.);
        const auto &reply=iceberg::civic_reply(iceberg::HeatworksMugReplyScene,option);
        require_quest(heat.has_side_flag(SideFlag::HeatworksMugReturned)&&heat.profile.story==carried.story&&
          dialogue.active()&&dialogue.lines.back().text.en==reply.answer.text.en,
          "both replies return the mug and get their own answer without changing the main story");
        finish_dialogue(heat);
        heat.load_room(true);
        walk_to(heat,reach_area(target(SideStory::HeatworksMugBench).area));interact(heat);
        require_quest(heat.state.adventure.inspecting&&heat.state.adventure.inspect_body.find("Ania")!=std::string::npos,
          "returning later opens the signed shift rota beside the mug");
      }
    }
    {
      using iceberg::SideStory;
      using iceberg::SideFlag;
      AdventureSession garden(options,{});
      garden.begin_campaign(true);
      garden.profile.story=63;garden.district=4;garden.stage=0;garden.load_room();
      const auto prop=[&](SideStory id) {
        const auto object=std::find_if(garden.grid.interactions.begin(),garden.grid.interactions.end(),
          [id](const Interaction &entry){return entry.kind==InteractionKind::SideStory&&entry.id==int(id);});
        if(object==garden.grid.interactions.end())throw std::runtime_error("missing garden prop");
        return *object;
      };
      const auto use=[&](SideStory id){walk_to(garden,reach_area(prop(id).area));interact(garden);};
      const auto original_story=garden.profile.story;
      use(SideStory::GardenSnailTap);
      require_quest(garden.state.adventure.inspecting&&!garden.has_side_flag(SideFlag::GardenSnailWatered),
        "the tap shows where its gutter leads before the snail is found");
      garden.tick({.inspect=true},0.);
      use(SideStory::GardensPot);
      require_quest(garden.state.adventure.inspecting&&garden.has_side_flag(SideFlag::GardenSnailFound)&&
        !garden.side_story_available(prop(SideStory::GardenSnailShelter)),
        "lifting the pot discovers the tenant before the shelter can be read");
      garden.tick({.inspect=true},0.);
      use(SideStory::GardenSnailTap);
      require_quest(garden.has_side_flag(SideFlag::GardenSnailWatered)&&garden.state.adventure.garden_snail_left>0.&&
        garden.profile.story==original_story,"watering starts the snail moving without altering Journey progression");
      garden.tick({.pause=true},.1);
      const double travel=garden.state.adventure.garden_snail_left;
      garden.tick({},.1);
      require_quest(garden.state.adventure.garden_snail_left==travel,"the snail stops moving while the game is paused");
      garden.tick({.pause=true},.1);
      for(int i=0;i<45;++i)garden.tick({},.1);
      use(SideStory::GardenSnailShelter);
      require_quest(garden.state.adventure.inspecting&&garden.state.adventure.inspect_body.find("Celina")!=std::string::npos,
        "the snail's trail leads to Celina's readable shelter tag");
      garden.load_room(true);
      require_quest(garden.state.adventure.garden_snail_left==0.&&garden.side_story_available(prop(SideStory::GardenSnailShelter)),
        "returning to the Gardens keeps the snail under the shelter");
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

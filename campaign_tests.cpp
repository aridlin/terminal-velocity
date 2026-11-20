#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
  std::cout << "PASS " << message << '\n';
}
int main() {
  try {
    check(run_stars(4, 0, 12) == 1 && run_stars(0, 0, 12) == 2 &&
              run_stars(3, 12, 12) == 2 && run_stars(0, 12, 12) == 3,
          "separate completion, clean-run and collection goals");
    for (int chapter = 1; chapter <= 3; ++chapter) {
      auto world = build_chapter(chapter);
      auto run = new_state(world);
      FixedStepper stepper;
      for (int tick = 0; tick < 120 * 100 && !run.won; ++tick) {
        auto &p = run.player;
        Controls c;
        c.move = 1;
        c.jump_held = true;
        int foot = int(std::floor(p.y + kPlayerHeight + .05));
        bool trouble = false;
        for (double dx = 1; dx <= 2.5; dx += .5) {
          int x = int(p.x + kPlayerWidth + dx);
          trouble |= world.solid_at(x, foot - 1) ||
                     world.hazard_at(x, foot - 1) || world.hazard_at(x, 37) ||
                     !world.solid_at(x, foot);
        }
        c.jump_pressed = p.grounded && trouble;
        c.dash_pressed = !p.grounded && p.vy >= 0 &&
                         !world.solid_at(int(p.x + kPlayerWidth + 1), 38) &&
                         p.dash_available;
        stepper.advance(run, world, c, 1. / 120);
      }
      std::cout << "chapter=" << chapter << " x=" << run.player.x
                << " deaths=" << run.deaths << " time=" << run.elapsed << '\n';
      check(run.won && run.deaths == 0,
            "authored chapter completed with normal jump/dash inputs");
      check(course_id(world) == chapter + 2,
            "campaign ghost course identity isolated");
    }
    {
      auto level = build_chapter(1);
      auto state = new_state(level);
      Controls retry;
      retry.restart = true;
      FixedStepper stepper;
      stepper.advance(state, level, retry, 1. / 120);
      check(state.deaths == 1 && run_stars(state.deaths, 0, 12) == 1,
            "checkpoint retries count toward the clean-run challenge");
    }
    auto folder =
        std::filesystem::temp_directory_path() /
        ("tv-progress-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    auto file = folder / "progress.dat";
    ProgressStore a(file), b(file);
    bool best = false;
    check(a.unlocked == 1, "new profile starts at chapter one");
    check(a.record(1, 20, 2, 1, 12, best) && best && a.unlocked == 2,
          "completion saves and unlocks next chapter");
    check(a.record(1, 21, 0, 2, 12, best) && a.records[0].stars == 2,
          "clean-run star earned independently of collecting treasures");
    check(a.record(1, 25, 3, 12, 12, best) && !best,
          "slower replay preserves faster time");
    check(a.records[0].best_time == 20 && a.records[0].best_deaths == 0 &&
              a.records[0].stars == 3,
          "best time, clean run and collection stars preserved independently");
    check(b.record(2, 30, 0, 0, 12, best), "second local profile writer saves");
    ProgressStore loaded(file);
    check(loaded.unlocked == 3 && loaded.records[0].stars == 3 &&
              loaded.records[1].complete,
          "progress persists and merges across local game clients");
    auto g = build_chapter(3);
    auto s = new_state(g);
    s.won = true;
    s.elapsed = 44;
    finish_chapter(s, g, loaded);
    check(s.result_recorded && s.earned_stars == 2 && !s.progress_error,
          "result recorded once and exposes earned stars");
    auto original = loaded.records[2].best_time;
    s.elapsed = 12;
    finish_chapter(s, g, loaded);
    check(loaded.records[2].best_time == original,
          "win screen does not repeatedly rewrite records");
    {
      std::ofstream out(file);
      out << "TVP1 3\ninvalid save";
    }
    ProgressStore corrupt(file);
    check(corrupt.unlocked == 1 && !corrupt.records[0].complete,
          "malformed profile safely ignored");
    auto blocked = folder / "directory.dat";
    std::filesystem::create_directory(blocked);
    ProgressStore read_only(blocked);
    check(!read_only.record(1, 20, 0, 0, 12, best) &&
              read_only.records[0].complete,
          "save failure retains session result without stopping play");
    std::filesystem::remove_all(folder);

    GameAudio sound;
    sound.init("assets/audio", false, true);
    check(sound.ready() && sound.loaded_clips() == 13 + 2 * size_t(AudioCue::Count) && sound.error().empty(),
          "all sourced music and effect voices decode offline");
    sound.update(AudioScene::Play, 0, .02);
    for (int voice = 0; voice < 2; ++voice)
      for (int i = 0; i < int(AudioCue::Count); ++i)
        sound.play(AudioCue(i));
    std::vector<float> frames(48000 * 2);
    check(sound.render(frames.data(), 48000),
          "music and overlapping effects mix from real recordings");
    double peak = 0, energy = 0;
    for (float sample : frames) {
      if (!std::isfinite(sample))
        throw std::runtime_error("non-finite audio sample");
      peak = std::max(peak, std::abs(double(sample)));
      energy += sample * sample;
    }
    std::cout << "audio peak=" << peak
              << " rms=" << std::sqrt(energy / frames.size()) << '\n';
    check(peak < .98 && energy > .001,
          "conservative overlapping audio mix leaves headroom");
    // Split a frame across a jump apex and a real pad contact. Velocity
    // sampling alone cannot identify the spring in this frame.
    {
      auto roof=verified_room(35,73,2);
      add_estate_awnings(roof);
      check(roof.springs.size()==2,"spring audio test uses the actual Estate awning");
      auto launch=new_state(roof);
      const auto &awning=roof.springs.front();
      launch.player.x=awning.x+.7;
      launch.player.y=awning.y-kPlayerHeight+.035;
      launch.player.vy=-1.25;
      launch.elapsed=1.;
      const auto before=launch.player;
      const double before_contact=launch.elapsed;
      FixedStepper step;
      step.advance(launch,roof,{},.1);
      check(launch.last_spring==0&&launch.last_spring_time>before_contact&&launch.deaths==0,
            "the awning contact occurs after the apex inside one rendered frame");

      GameAudio actual,expected;
      actual.init("assets/audio",false,true);
      expected.init("assets/audio",false,true);
      feedback_step(launch,roof,before,0,0,0,false,before_contact,.1,actual);
      expected.play(AudioCue::Spring,.8f);
      expected.play(AudioCue::Cloth,.85f);
      std::vector<float> played(48000*2),reference(played.size());
      check(actual.render(played.data(),played.size()/2)&&expected.render(reference.data(),reference.size()/2),
            "the rebound and fabric recordings mix through the real audio engine");
      double signal=0.,difference=0.;
      for(size_t i=0;i<played.size();++i) {
        signal+=std::abs(reference[i]);
        difference+=std::abs(played[i]-reference[i]);
      }
      check(signal>1.&&difference<.001,"an awning contact plays the rebound and fabric exactly once");
      feedback_step(launch,roof,launch.player,0,0,0,false,launch.elapsed,.1,actual);
      check(actual.render(played.data(),played.size()/2)&&
            std::all_of(played.begin(),played.end(),[](float value){return std::abs(value)<.000001f;}),
            "a later feedback frame does not replay the same launch");
    }
    sound.set_climax(true,1);
    for(int block=0;block<150;++block) {
      sound.update(AudioScene::Play,1,.02);
      if(block%25==0)for(int i=0;i<int(AudioCue::Count);++i)sound.play(AudioCue(i));
      if(!sound.render(frames.data(),960))throw std::runtime_error("climax block did not render");
      for(int i=0;i<1920;++i)if(!std::isfinite(frames[size_t(i)])||std::abs(frames[size_t(i)])>=.98f)
        throw std::runtime_error("layered climax clipped or produced nonfinite audio");
    }
    check(true,"layered climax with recorded effects retains mixing headroom");
    GameAudio quiet_layers,loud_layers;
    quiet_layers.init("assets/audio",false,true);loud_layers.init("assets/audio",false,true);
    quiet_layers.set_climax(true,0);loud_layers.set_climax(true,1);
    double difference=0;
    std::vector<float> quiet(1920),loud(1920);
    for(int block=0;block<150;++block) {
      quiet_layers.update(AudioScene::Play,1,.02);loud_layers.update(AudioScene::Play,1,.02);
      quiet_layers.render(quiet.data(),960);loud_layers.render(loud.data(),960);
      for(size_t i=0;i<quiet.size();++i)difference+=std::abs(quiet[i]-loud[i]);
    }
    check(difference>1,"shutter intensity audibly changes the synchronized atmospheric layers");
    loud_layers.set_mix(0,0,0);loud_layers.update(AudioScene::Play,1,.02);loud_layers.render(loud.data(),960);
    check(std::all_of(loud.begin(),loud.end(),[](float x){return std::abs(x)<1e-7f;}),"music control silences both climax layers");
    loud_layers.set_mix(1,1,1);loud_layers.update(AudioScene::Play,1,.02,false);loud_layers.render(loud.data(),960);
    check(std::all_of(loud.begin(),loud.end(),[](float x){return std::abs(x)<1e-7f;}),"focus loss silences the entire layered score immediately");
    {
      GameAudio dry_score,echo_score,interrupted_score;
      dry_score.init("assets/audio",false,true);
      echo_score.init("assets/audio",false,true);
      interrupted_score.init("assets/audio",false,true);
      dry_score.set_mix(1,0,0);echo_score.set_mix(1,0,0);interrupted_score.set_mix(1,0,0);
      echo_score.set_climax(true,1);interrupted_score.set_climax(true,1);
      constexpr size_t rate=48000,frames_per_score=rate*6,block_frames=960;
      std::vector<float> dry(frames_per_score*2),echo(frames_per_score*2),interrupted(frames_per_score*2);
      for(size_t frame=0;frame<frames_per_score;frame+=block_frames) {
        dry_score.update(AudioScene::Play,3,.02);echo_score.update(AudioScene::Play,3,.02);
        interrupted_score.muted=frame>=rate&&frame<rate*3/2;
        const bool focused=frame<rate*3||frame>=rate*21/5;
        interrupted_score.update(AudioScene::Play,3,.02,focused);
        if(!dry_score.render(dry.data()+frame*2,block_frames)||
           !echo_score.render(echo.data()+frame*2,block_frames)||
           !interrupted_score.render(interrupted.data()+frame*2,block_frames))
          throw std::runtime_error("the Crown audio block did not render");
      }
      double energy=0,error=0;
      // Ignore fades. The first voice begins after the clock's 50 ms lead-in.
      for(size_t i=rate*4;i<echo.size();++i) {
        const double delayed=dry[i-rate*8/5]*.75;
        const double measured=echo[i]-dry[i-rate/10];
        energy+=delayed*delayed;
        error+=(measured-delayed)*(measured-delayed);
      }
      check(energy>1&&error/energy<.001,"the Crown adds a second copy of its score 750 ms behind the first");
      check(std::all_of(echo.begin(),echo.end(),[](float x){return std::isfinite(x)&&std::abs(x)<.98f;}),
        "the delayed Crown mix retains headroom");
      const auto silent=[&](size_t first,size_t last) {
        return std::all_of(interrupted.begin()+first*2,interrupted.begin()+last*2,
          [](float x){return std::abs(x)<1e-7f;});
      };
      const auto matches=[&](size_t first,size_t last) {
        return std::equal(echo.begin()+first*2,echo.begin()+last*2,interrupted.begin()+first*2,
          [](float a,float b){return std::abs(a-b)<1e-6f;});
      };
      check(silent(rate,rate*3/2)&&silent(rate*3,rate*21/5),
        "mute and focus loss silence both voices");
      check(matches(rate*5/2,rate*3)&&matches(rate*5,frames_per_score),
        "the Crown theme stays in phase through mute and focus loss");
    }
    sound.set_climax(false);
    sound.set_mix(0,0,1);sound.set_environment(0);
    sound.update(AudioScene::Play,0,.02);sound.render(frames.data(),48000);
    check(std::any_of(frames.begin(),frames.end(),[](float x){return std::abs(x)>.0001f;}),
      "ambience is audible with music and effects channels at zero");
    sound.set_mix(0,0,0);sound.update(AudioScene::Play,0,.02);sound.render(frames.data(),48000);
    check(std::all_of(frames.begin(),frames.end(),[](float x){return std::abs(x)<1e-7f;}),
      "independent channel volumes silence all playing recordings");
    sound.set_mix(1,1,1);sound.set_environment(-1);
    sound.muted = true;
    sound.update(AudioScene::Play, 0, .02);
    check(sound.render(frames.data(), 48000), "muted mix renders");
    check(std::all_of(frames.begin(), frames.end(),
                      [](float x) { return std::abs(x) < 1e-7f; }),
          "mute silences music and already-playing effects");
    sound.muted = false;
    sound.update(AudioScene::Play, 1, .02);
    check(sound.render(frames.data(), 48000), "second music track crossfades");
    sound.update(AudioScene::Paused, 1, .02, false);
    sound.render(frames.data(), 48000);
    check(std::all_of(frames.begin(), frames.end(),
                      [](float x) { return std::abs(x) < 1e-7f; }),
          "focus loss silences audio immediately");
    iceberg::SessionOptions vent_options;vent_options.no_save=vent_options.mute=true;
    AdventureSession vent_run(vent_options,{});
    vent_run.audio.init("assets/audio",false,true);
    vent_run.profile.music_volume=vent_run.profile.ambience_volume=0.f;
    vent_run.grid=EngineGrid(80,32);
    vent_run.grid.adventure_level=true;
    vent_run.grid.district=1;
    vent_run.grid.add_solid_rect(0,24,80,8);
    vent_run.grid.spawn_x=8.;vent_run.grid.spawn_y=24.-kPlayerHeight;
    vent_run.grid.goal={75,20,2,4};
    vent_run.grid.pulses.push_back({{14,21.5,1.5,2.5},4.,0.});
    vent_run.state=new_state(vent_run.grid);
    vent_run.state.player.grounded=true;
    const double just_before=4.*PulseHazard::warning_start-.004;
    const auto vent_mix=[&] {
      if(!vent_run.audio.render(frames.data(),48000))throw std::runtime_error("could not mix vent cue");
      return std::any_of(frames.begin(),frames.end(),[](float x){return std::abs(x)>.0001f;});
    };
    vent_run.state.elapsed=just_before;
    vent_run.state.paused=true;
    vent_run.tick({},.1);
    check(vent_run.state.elapsed==just_before&&!vent_mix(),"paused steam warning leaves the sound and clock quiet");
    vent_run.state.paused=false;
    vent_run.tick({},1./120.);
    check(vent_run.grid.pulses.front().warning(vent_run.state.elapsed)&&vent_mix(),
      "a nearby steam vent gives one audible warning before the burst");
    vent_run.tick({},1./120.);
    check(!vent_mix(),"steam warning does not replay while the vent stays charged");
    vent_run.state.player.x=58.;
    vent_run.state.elapsed=just_before+4.;
    vent_run.tick({},1./120.);
    check(!vent_mix(),"a distant steam warning is silent");
    vent_run.state.player.x=8.;
    vent_run.state.elapsed=8.-.004;
    vent_run.tick({},1./120.);
    check(vent_run.grid.pulses.front().active(vent_run.state.elapsed)&&vent_mix(),
      "the steam burst still has its own audible cue");
    AdventureSession planter_run(vent_options,{});
    const auto hanging_beds=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==54;});
    check(hanging_beds!=iceberg::DebugTour.end(),"Debug Tour includes Hanging Beds");
    planter_run.start_tour(int(hanging_beds-iceberg::DebugTour.begin()));
    planter_run.profile.music_volume=planter_run.profile.ambience_volume=0.f;
    planter_run.profile.effects_volume=1.f;
    planter_run.audio.init("assets/audio",false,true);
    auto &ride=planter_run.state;
    const auto &planter=planter_run.grid.platforms.front();
    const double before_upper=planter.period*.5-.004;
    ride.elapsed=before_upper;
    const auto deck=planter.at(ride.elapsed);
    ride.player.x=deck.x+1.;
    ride.player.y=deck.y-kPlayerHeight;
    ride.player.grounded=true;
    ride.player.support=0;
    const auto planter_silent=[&] {
      if(!planter_run.audio.render(frames.data(),48000))throw std::runtime_error("could not mix planter cue");
      return std::none_of(frames.begin(),frames.end(),[](float x){return std::abs(x)>.0001f;});
    };
    ride.paused=true;
    planter_run.tick({},.1);
    check(ride.elapsed==before_upper&&planter_silent(),"pausing before the upper stop stays quiet");
    ride.paused=false;
    planter_run.tick({},1./120.);
    GameAudio wood_stop;
    wood_stop.init("assets/audio",false,true);
    wood_stop.set_mix(0.f,1.f,0.f);
    wood_stop.play(AudioCue::Wood2,.7f);
    std::vector<float> expected(frames.size());
    check(wood_stop.render(expected.data(),48000)&&!planter_silent()&&
      std::equal(expected.begin(),expected.end(),frames.begin(),[](float a,float b){return std::abs(a-b)<.00001f;}),
      "reaching the upper stop plays the recorded wood cue exactly once");
    planter_run.tick({},1./120.);
    check(planter_silent(),"the planter does not replay its arrival during the dwell");
    ride.elapsed=before_upper;
    ride.player.x=10.;
    ride.player.y=54.-kPlayerHeight;
    ride.player.support=-1;
    planter_run.tick({},1./120.);
    check(planter_silent(),"an empty planter makes no arrival sound");
    AdventureSession piston_run(vent_options,{});
    const auto piston_stop=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==421;});
    check(piston_stop!=iceberg::DebugTour.end(),"the Twin Pistons sound test has a playable room");
    piston_run.start_tour(int(piston_stop-iceberg::DebugTour.begin()));
    piston_run.profile.music_volume=piston_run.profile.ambience_volume=0.f;
    piston_run.profile.effects_volume=1.f;
    piston_run.audio.init("assets/audio",false,true);
    auto &piston_state=piston_run.state;
    const auto &piston=piston_run.grid.platforms[3];
    const auto piston_silent=[&] {
      check(piston_run.audio.render(frames.data(),48000),"mix the piston audio");
      return std::none_of(frames.begin(),frames.end(),[](float x){return std::abs(x)>.0001f;});
    };
    piston_run.tick({},1./120.);
    check(piston_silent(),"entering at an already docked piston stays quiet");
    piston_state.elapsed=piston.period*.5-.004;
    const auto rising=piston.at(piston_state.elapsed);
    piston_state.player.x=rising.x+(rising.w-kPlayerWidth)*.5;
    piston_state.player.y=rising.y-kPlayerHeight;
    piston_state.player.support=3;
    piston_state.player.grounded=true;
    piston_state.paused=true;
    piston_run.tick({},.1);
    check(piston_silent(),"a paused pressure piston does not announce its arrival");
    piston_state.paused=false;
    piston_run.tick({},1./120.);
    GameAudio piston_sound;
    piston_sound.init("assets/audio",false,true);
    piston_sound.set_mix(0.f,1.f,0.f);
    piston_sound.play(AudioCue::Metal2,.7f);
    check(piston_sound.render(expected.data(),48000)&&!piston_silent()&&
      std::equal(expected.begin(),expected.end(),frames.begin(),[](float a,float b){return std::abs(a-b)<.00001f;}),
      "the nearest pressure piston plays one recorded metal contact on arrival");
    piston_run.tick({},1./120.);
    check(piston_silent(),"dwelling at the landing does not replay the piston cue");
    piston_state.elapsed=piston.period-.004;
    piston_state.player.x=4.;
    piston_state.player.y=55.-kPlayerHeight;
    piston_state.player.support=-1;
    piston_run.tick({},1./120.);
    check(piston_silent(),"distant pressure pistons do not play a docking cue");
    AdventureSession tray_run(vent_options,{});
    const auto seed_bank=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==iceberg::GardenSeedBank;});
    check(seed_bank!=iceberg::DebugTour.end(),"Debug Tour includes the Seed Bank");
    tray_run.start_tour(int(seed_bank-iceberg::DebugTour.begin()));
    check(tray_run.grid.platforms.size()==1&&tray_run.grid.platforms.front().seed_tray,
      "the Seed Bank sound test has a working tray");
    tray_run.profile.music_volume=tray_run.profile.ambience_volume=0.f;
    tray_run.profile.effects_volume=1.f;
    tray_run.audio.init("assets/audio",false,true);
    auto &tray_state=tray_run.state;
    const auto &tray=tray_run.grid.platforms.front();
    const auto tray_silent=[&] {
      if(!tray_run.audio.render(frames.data(),48000))throw std::runtime_error("could not mix tray cue");
      return std::none_of(frames.begin(),frames.end(),[](float x){return std::abs(x)>.0001f;});
    };
    tray_run.tick({},1./120.);
    check(tray_silent(),"entering while the seed tray is docked stays quiet");
    const auto ride_tray=[&](double time) {
      tray_state.elapsed=time;
      const auto platform=tray.at(time);
      auto &player=tray_state.player;
      player.x=platform.x+(platform.w-kPlayerWidth)*.5;
      player.y=platform.y-kPlayerHeight;
      player.support=0;
      player.grounded=true;
    };
    ride_tray(tray.period*.5-.1);
    tray_run.tick({},1./120.);
    check(tray_silent(),"the moving seed tray stays quiet between landings");
    ride_tray(tray.period*.5-.004);
    tray_state.paused=true;
    tray_run.tick({},.1);
    check(tray_silent(),"pausing does not trigger a tray cue");
    tray_state.paused=false;
    GameAudio tray_sound;
    tray_sound.init("assets/audio",false,true);
    tray_sound.set_mix(0.f,1.f,0.f);
    for(double landing:{tray.period*.5,tray.period}) {
      ride_tray(landing-.004);
      tray_run.tick({},1./120.);
      tray_sound.play(AudioCue::Wood1,.7f);
      check(tray_sound.render(expected.data(),48000)&&!tray_silent()&&
        std::equal(expected.begin(),expected.end(),frames.begin(),[](float a,float b){return std::abs(a-b)<.00001f;}),
        "the seed tray plays the recorded wood cue on arrival");
      tray_run.tick({},1./120.);
      check(tray_silent(),"the tray stays quiet after the docking cue");
    }
    tray_state.elapsed=tray.period-.004;
    tray_state.player.x=4.;
    tray_state.player.y=56.-kPlayerHeight;
    tray_state.player.support=-1;
    tray_run.tick({},1./120.);
    check(tray_silent(),"a distant seed tray is inaudible");
    GameAudio silent;
    silent.init("missing-audio", true);
    check(!silent.ready() && silent.error().empty(),
          "--mute does not open an audio device or load assets");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}

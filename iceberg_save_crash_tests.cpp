#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#ifndef _WIN32
#include <sys/wait.h>
#include <signal.h>
#endif
int main() {
#ifndef _WIN32
  const auto folder=std::filesystem::temp_directory_path()/("tv-save-crash-"+std::to_string(getpid()));
  std::filesystem::create_directories(folder);
  auto path=folder/"campaign.dat";iceberg::SaveStore store(path);iceberg::Profile previous;
  previous.started=true;previous.seed=73;previous.story=219;
  previous.expeditions[0].active=true;
  for(int n=0;n<128;++n)previous.challenges.push_back({unsigned(n),n%2,iceberg::ContentVersion,{50.+n,n%4,n,true}});
  int old_count=0,new_count=0;
  for(int attempt=0;attempt<200;++attempt) {
    if(!store.save(previous))return 1;
    auto next=previous;next.seed=unsigned(1000+attempt);next.story=uint64_t(next.seed)*3;
    auto &expedition=next.expeditions[0];
    expedition.depth+=iceberg::RoomsPerLeg;
    expedition.completed_legs.push_back({0,1,2,3,4,5,6,attempt%72});
    int ready[2];if(pipe(ready))return 2;
    pid_t child=fork();
    if(child<0)return 2;
    if(child==0) {
      close(ready[0]);char byte='!';int phase=attempt%5;
      // Explicit before/after barriers guarantee coverage independently of
      // filesystem speed; the other 120 cases interrupt at varying offsets.
      if(phase!=1&&write(ready[1],&byte,1)!=1)_exit(2);
      if(phase==0)for(;;)pause();
      if(!store.save(next))_exit(1);
      if(phase==1&&write(ready[1],&byte,1)!=1)_exit(2);
      close(ready[1]);for(;;)pause();
    }
    close(ready[1]);char byte=0;if(read(ready[0],&byte,1)!=1)return 2;close(ready[0]);
    if(attempt%5>=2)usleep(unsigned(attempt%20)*75);
    kill(child,SIGKILL);int status=0;waitpid(child,&status,0);
    if(!WIFSIGNALED(status)||WTERMSIG(status)!=SIGKILL)return 2;
    iceberg::Profile recovered;iceberg::SaveStore after_crash(path);
    if(!after_crash.load(recovered)||recovered.story!=uint64_t(recovered.seed)*3||
       (recovered.seed!=previous.seed&&recovered.seed!=next.seed)||recovered.challenges.size()!=128) {
      std::cerr<<"FAIL interrupted save attempt="<<attempt<<'\n';return 1;
    }
    const auto &expected=recovered.seed==previous.seed?previous:next;
    if(recovered.expeditions[0].completed_legs!=expected.expeditions[0].completed_legs) {
      std::cerr<<"FAIL route history disagrees with the recovered save at attempt "<<attempt<<'\n';return 1;
    }
    old_count+=recovered.seed==previous.seed;new_count+=recovered.seed==next.seed;
    previous=std::move(recovered);
  }
  auto fallback_path=folder/"route-recovery";
  iceberg::SaveStore fallback_store(fallback_path);
  auto first=previous,second=previous;
  first.story+=1;second.story+=2;
  second.expeditions[0].depth+=iceberg::RoomsPerLeg;
  second.expeditions[0].completed_legs.push_back({7,6,5,4,3,2,1,0});
  if(!fallback_store.save(first)||!fallback_store.save(second))return 1;
  iceberg::Profile detached;
  if(!iceberg::SaveStore::validate_file(fallback_path,detached)||
     !detached.expeditions[0].completed_legs.empty()||detached.story!=second.story) {
    std::cerr<<"FAIL primary progress cannot be imported independently of local history\n";return 1;
  }
  iceberg::SaveStore other_slot(iceberg::SaveStore::slot_file(fallback_path,2));
  iceberg::Profile other;
  if(!other_slot.save(detached)||!other_slot.load(other)||!other.expeditions[0].completed_legs.empty()) {
    std::cerr<<"FAIL local route history leaked into another save slot\n";return 1;
  }
  auto recent_history=fallback_path;recent_history+=".routes.bak";
  std::ofstream(recent_history,std::ios::trunc)<<"incomplete history";
  iceberg::Profile progress;
  if(!fallback_store.load(progress)||fallback_store.recovered||progress.story!=second.story||
     !progress.expeditions[0].completed_legs.empty()) {
    std::cerr<<"FAIL damaged optional history interfered with saved progress\n";return 1;
  }
  std::ofstream(fallback_path,std::ios::trunc)<<"incomplete primary";
  iceberg::Profile backup;
  if(!fallback_store.load(backup)||!fallback_store.recovered||backup.story!=first.story||
     backup.expeditions[0].completed_legs!=first.expeditions[0].completed_legs) {
    std::cerr<<"FAIL backup save did not restore its own history\n";return 1;
  }
  std::filesystem::remove_all(folder);
  std::cout<<"PASS 200 actual process-kill save trials; previous="<<old_count<<" replacement="<<new_count<<"; every recovered profile and route history match\n";
  std::cout<<"PASS standalone save import and backup route history recovery\n";
  return old_count&&new_count?0:1;
#else
  std::cout<<"SKIP process-kill save stress: POSIX harness; Windows atomic replacement covered by native save tests\n";
#endif
}

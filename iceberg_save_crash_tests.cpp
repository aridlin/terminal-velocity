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
  for(int n=0;n<128;++n)previous.challenges.push_back({unsigned(n),n%2,iceberg::ContentVersion,{50.+n,n%4,n,true}});
  int old_count=0,new_count=0;
  for(int attempt=0;attempt<200;++attempt) {
    if(!store.save(previous))return 1;
    auto next=previous;next.seed=unsigned(1000+attempt);next.story=uint64_t(next.seed)*3;
    int ready[2];if(pipe(ready))return 2;
    pid_t child=fork();
    if(child<0)return 2;
    if(child==0) {
      close(ready[0]);char byte='!';if(write(ready[1],&byte,1)!=1)_exit(2);close(ready[1]);
      bool okay=store.save(next);_exit(okay?0:1);
    }
    close(ready[1]);char byte=0;if(read(ready[0],&byte,1)!=1)return 2;close(ready[0]);
    usleep(unsigned(attempt%20)*75);kill(child,SIGKILL);int status=0;waitpid(child,&status,0);
    iceberg::Profile recovered;iceberg::SaveStore after_crash(path);
    if(!after_crash.load(recovered)||recovered.story!=uint64_t(recovered.seed)*3||
       (recovered.seed!=previous.seed&&recovered.seed!=next.seed)||recovered.challenges.size()!=128) {
      std::cerr<<"FAIL interrupted save attempt="<<attempt<<'\n';return 1;
    }
    old_count+=recovered.seed==previous.seed;new_count+=recovered.seed==next.seed;
    previous=std::move(recovered);
  }
  std::filesystem::remove_all(folder);
  std::cout<<"PASS 200 actual process-kill save trials; previous="<<old_count<<" replacement="<<new_count<<"; every recovered profile is complete\n";
  return old_count&&new_count?0:1;
#else
  std::cout<<"SKIP process-kill save stress: POSIX harness; Windows atomic replacement covered by native save tests\n";
#endif
}

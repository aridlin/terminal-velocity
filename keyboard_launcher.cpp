// Open only keyboard event descriptors, then permanently drop root before
// starting the game. Invoked through sudo only when the session cannot already
// read evdev. Never install setuid.
#include <array>
#include <climits>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <grp.h>
#include <iostream>
#include <linux/input.h>
#include <pwd.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
int main(int argc, char **argv) {
  if (geteuid() != 0) {
    std::cerr << "Uruchom przez play.sh.\n";
    return 1;
  }
  const char *user = getenv("SUDO_UID");
  if (!user) {
    std::cerr << "Use sudo as your ordinary user.\n";
    return 1;
  }
  char *end = nullptr;
  long uid = strtol(user, &end, 10);
  if (*end || uid <= 0) {
    return 1;
  }
  passwd *pw = getpwuid(uid);
  if (!pw)
    return 1;
  std::vector<int> fds;
  std::string inherited;
  std::error_code error;
  for (const auto &e :
       std::filesystem::directory_iterator("/dev/input", error)) {
    if (!e.path().filename().string().starts_with("event"))
      continue;
    int fd = open(e.path().c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
      continue;
    std::array<unsigned long, (KEY_CNT + sizeof(long) * CHAR_BIT - 1) /
                                  (sizeof(long) * CHAR_BIT)>
        bits{};
    auto bit = [&](int k) {
      return (bits[k / (sizeof(long) * CHAR_BIT)] >>
              (k % (sizeof(long) * CHAR_BIT))) &
             1UL;
    };
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), bits.data()) >= 0 &&
        bit(KEY_A) && bit(KEY_D) && bit(KEY_SPACE)) {
      if (!inherited.empty())
        inherited += ",";
      inherited += std::to_string(fd);
      fds.push_back(fd);
    } else
      close(fd);
  }
  if (fds.empty()) {
    std::cerr << "Nie znaleziono klawiatury evdev.\n";
    return 1;
  }
  // Resolve the executable before dropping privileges; it is executed only as
  // the ordinary user.
  auto executable =
      std::filesystem::read_symlink("/proc/self/exe", error).parent_path() /
      "terminal_velocity";
  if (error)
    return 1;
  if (initgroups(pw->pw_name, pw->pw_gid) ||
      setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) ||
      setresuid(uid, uid, uid) || geteuid() == 0) {
    std::cerr << "Privilege drop failed\n";
    return 1;
  }
  setenv("TV_INPUT_FDS", inherited.c_str(), 1);
  setenv("HOME", pw->pw_dir, 1);
  unsetenv("SUDO_COMMAND");
  std::vector<char *> args;
  std::string path = executable.string();
  args.push_back(path.data());
  for (int i = 1; i < argc; ++i)
    args.push_back(argv[i]);
  args.push_back(nullptr);
  execv(path.c_str(), args.data());
  perror("execv");
  return 1;
}

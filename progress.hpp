#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#ifdef _WIN32
#include "net_platform.hpp"
#endif

struct ChapterRecord {
  double best_time = 0;
  int best_deaths = -1, treasures = 0, stars = 0;
  bool complete = false;
};
inline int run_stars(int deaths, int collected, int total) {
  return 1 + (deaths == 0 ? 1 : 0) + (total > 0 && collected == total ? 1 : 0);
}
class ProgressStore {
  std::filesystem::path file_;
  static void merge(ChapterRecord &a, const ChapterRecord &b) {
    if (!b.complete)
      return;
    if (!a.complete || b.best_time < a.best_time)
      a.best_time = b.best_time;
    if (a.best_deaths < 0 || b.best_deaths < a.best_deaths)
      a.best_deaths = b.best_deaths;
    a.treasures = std::max(a.treasures, b.treasures);
    a.stars = std::max(a.stars, b.stars);
    a.complete = true;
  }

public:
  std::array<ChapterRecord, 3> records{};
  int unlocked = 1;
  static std::filesystem::path default_file() {
    if (const char *override_path = std::getenv("TV_PROGRESS_FILE"))
      return override_path;
#ifdef _WIN32
    wchar_t base[32768];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, 32768);
    if (n && n < 32768)
      return std::filesystem::path(base) / "TerminalVelocity" / "progress.dat";
#else
    if (const char *base = std::getenv("XDG_STATE_HOME"))
      if (std::filesystem::path(base).is_absolute())
        return std::filesystem::path(base) / "terminal-velocity" /
               "progress.dat";
    if (const char *base = std::getenv("HOME"))
      return std::filesystem::path(base) /
             ".local/state/terminal-velocity/progress.dat";
#endif
    return {};
  }
  explicit ProgressStore(std::filesystem::path file = default_file())
      : file_(std::move(file)) {
    load();
  }
  void load() {
    std::ifstream in(file_);
    std::string version;
    int count = 1;
    std::array<ChapterRecord, 3> candidate{};
    if (!(in >> version >> count) || version != "TVP1" || count < 1 ||
        count > 3)
      return;
    for (auto &r : candidate)
      if (!(in >> r.best_time >> r.best_deaths >> r.treasures >> r.stars >>
            r.complete) ||
          !std::isfinite(r.best_time) || r.best_time < 0 || r.best_time > 1e8 ||
          r.best_deaths < -1 || r.best_deaths > 1000000 || r.treasures < 0 ||
          r.treasures > 10000 || r.stars < 0 || r.stars > 3 ||
          (r.complete && (r.best_deaths < 0 || r.stars < 1)))
        return;
    records = candidate;
    unlocked = count;
  }
  bool record(int chapter, double time, int deaths, int treasures, int total,
              bool &new_best) {
    if (chapter < 1 || chapter > 3 || !std::isfinite(time) || time < 0 ||
        deaths < 0 || treasures < 0 || treasures > total)
      return false;
    auto &r = records[size_t(chapter - 1)];
    new_best = !r.complete || time < r.best_time;
    ChapterRecord result{time, deaths, treasures,
                         run_stars(deaths, treasures, total), true};
    merge(r, result);
    unlocked = std::max(unlocked, std::min(3, chapter + 1));
    // Merge the on-disk profile so a second local ghost client does not erase
    // progress already saved by the first one.
    ProgressStore disk(file_);
    unlocked = std::max(unlocked, disk.unlocked);
    for (size_t i = 0; i < records.size(); ++i)
      merge(records[i], disk.records[i]);
    records[size_t(chapter - 1)].stars =
        1 + (records[size_t(chapter - 1)].best_deaths == 0 ? 1 : 0) +
        (total > 0 && records[size_t(chapter - 1)].treasures == total ? 1 : 0);
    if (file_.empty())
      return false;
    std::error_code ec;
    if (!file_.parent_path().empty())
      std::filesystem::create_directories(file_.parent_path(), ec);
    if (ec)
      return false;
    auto temp = file_;
    temp += "." +
            std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()) +
            ".tmp";
    {
      std::ofstream out(temp);
      if (!out)
        return false;
      out.precision(17);
      out << "TVP1 " << unlocked << '\n';
      for (const auto &score : records)
        out << score.best_time << ' ' << score.best_deaths << ' '
            << score.treasures << ' ' << score.stars << ' ' << score.complete
            << '\n';
      out.close();
      if (!out) {
        std::filesystem::remove(temp, ec);
        return false;
      }
    }
#ifdef _WIN32
    bool saved = MoveFileExW(temp.c_str(), file_.c_str(),
                             MOVEFILE_REPLACE_EXISTING |
                                 MOVEFILE_WRITE_THROUGH) != FALSE;
#else
    std::filesystem::rename(temp, file_, ec);
    bool saved = !ec;
#endif
    if (!saved)
      std::filesystem::remove(temp, ec);
    return saved;
  }
};

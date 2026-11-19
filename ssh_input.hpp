// PTY-only input. Negotiates kitty key-up events; legacy terminals use explicit
// latched direction (A/D start, S stop), never terminal-repeat hold guesses.
class SshInput {
public:
  bool focused = true, enhanced = false;
  explicit SshInput(bool basic = false) : basic_(basic) {}
  void clear() {
    keys_.clear();
    direction_ = 0;
    jump_left_ = 0;
    edges_ = {};
  }
  void feed(const std::string &bytes) {
    pending_ += bytes;
    while (!pending_.empty()) {
      if ((unsigned char)pending_[0] != 27) {
        unsigned k = (unsigned char)pending_[0];
        pending_.erase(0, 1);
        key(k, 1, false);
        continue;
      }
      if (pending_.size() < 2)
        break;
      if (pending_[1] != '[' && pending_[1] != 'O') {
        pending_.erase(0, 1);
        key(27, 1, false);
        continue;
      }
      size_t end = 2;
      while (end < pending_.size() &&
             !(pending_[end] >= 0x40 && pending_[end] <= 0x7e))
        ++end;
      if (end == pending_.size()) {
        if (pending_.size() > 128)
          pending_.clear();
        break;
      }
      char final = pending_[end];
      std::string body = pending_.substr(2, end - 2);
      pending_.erase(0, end + 1);
      if (final == 'I' && body.empty()) {
        focused = true;
        continue;
      }
      if (final == 'O' && body.empty()) {
        focused = false;
        clear();
        continue;
      }
      if (final == 'u' && !body.empty() && body[0] == '?') {
        if (!basic_ && (std::atoi(body.c_str() + 1) & 2))
          enhanced = true;
        continue;
      }
      int event = 1;
      auto semi = body.find(';');
      if (semi != std::string::npos) {
        auto colon = body.find(':', semi);
        if (colon != std::string::npos)
          event = std::atoi(body.c_str() + colon + 1);
      }
      bool protocol = !basic_ && (enhanced || final == 'u' ||
                                  (semi != std::string::npos &&
                                   body.find(':', semi) != std::string::npos));
      if (protocol)
        enhanced = true;
      unsigned code = 0;
      if (final == 'u')
        code = std::strtoul(body.c_str(), nullptr, 10);
      else if (final == 'A')
        code = 'w';
      else if (final == 'B')
        code = 's';
      else if (final == 'C')
        code = 'd';
      else if (final == 'D')
        code = 'a';
      else if (final == '~' && std::atoi(body.c_str()) == 5)
        code = 57354;
      else if (final == '~' && std::atoi(body.c_str()) == 6)
        code = 57355;
      else if ((final == '~' && std::atoi(body.c_str()) == 13) || final == 'R')
        code = 100003;
      if (code)
        key(code, event, protocol);
    }
  }
  Controls consume(double dt) {
    if (pending_ == "\x1b") {
      escape_age_ += dt;
      if (escape_age_ > .12) {
        pending_.clear();
        key(27, 1, false);
        escape_age_ = 0;
      }
    } else
      escape_age_ = 0;
    Controls c = edges_;
    edges_ = {};
    if (enhanced) {
      c.move = horizontal();
      c.jump_held = held(' ') || held('w');
      c.fast_fall = held('s');
      c.aim_y = vertical();
      if (held('d') && held('a'))
        c.move_pressed = 0;
      if (held('s') && held('w'))
        c.aim_pressed = 0;
    } else {
      c.move = direction_;
      c.jump_held = jump_left_ > 0;
      jump_left_ = std::max(0.0, jump_left_ - dt);
    }
    return c;
  }
  Controls poll(double dt) {
#ifndef _WIN32
    char bytes[1024];
    ssize_t n;
    while ((n = read(STDIN_FILENO, bytes, sizeof(bytes))) > 0)
      feed(std::string(bytes, n));
#endif
    return consume(dt);
  }

private:
  bool basic_ = false;
  std::string pending_;
  std::map<unsigned, bool> keys_;
  Controls edges_{};
  int direction_ = 0;
  double jump_left_ = 0, escape_age_ = 0;
  bool held(unsigned k) const {
    auto it = keys_.find(k);
    return it != keys_.end() && it->second;
  }
  int horizontal() const { return int(held('d')) - int(held('a')); }
  int vertical() const { return int(held('s')) - int(held('w')); }
  void key(unsigned k, int event, bool protocol) {
    if (k >= 'A' && k <= 'Z')
      k += 'a' - 'A';
    if (!focused)
      return;
    if (protocol) {
      bool prior = held(k);
      keys_[k] = event != 3;
      if (event == 3 || event == 2 || prior)
        return;
      // A complete tap can arrive in one read. Keep the press alongside held state.
      if (k == 'a' || k == 'd')
        edges_.move_pressed = horizontal();
      if (k == 'w' || k == 's')
        edges_.aim_pressed = vertical();
    } else {
      if (k == 'a')
        direction_ = -1;
      if (k == 'd')
        direction_ = 1;
      if (k == 's')
        direction_ = 0;
    }
    if (k == ' ' || k == 'w') {
      edges_.jump_pressed = true;
      jump_left_ = .18;
    }
    if (k == 'x' || k == 'k' || k == 57441 || k == 57447)
      edges_.dash_pressed = true;
    // Explicit vertical dash edges keep the full traversal vocabulary usable
    // on terminals without key-release events. A/D still supplies diagonals;
    // S then I/O produces a vertical dash without changing the latched run.
    if (!protocol && (k == 'i' || k == 'o')) {
      edges_.dash_pressed = true;
      edges_.aim_y = k == 'i' ? -1 : 1;
    }
    if (k == 'q' || k == 3)
      edges_.quit = true;
    if (k == 'r')
      edges_.restart = true;
    if (k == 'n')
      edges_.new_run = true;
    if (k == 'g')
      edges_.next_seed = true;
    if (k == 'm')
      edges_.menu_request = true;
    if (k == '1')
      edges_.select_level = 1;
    if (k == '2')
      edges_.select_level = 2;
    if (k >= '3' && k <= '7')
      edges_.select_level = int(k - '0');
    if (k == 'u')
      edges_.mute_audio = true;
    if (k == 'e' || k == 'f') edges_.interact = true;
    if (k == 'b') edges_.inspect = true;
    if (k == 'j') edges_.map = true;
    if (k == 'l') edges_.language = true;
    if (k == 's') {
      edges_.navigate = 1;
      edges_.fast_fall_pressed |= protocol;
    }
    if (k == 'w') edges_.navigate = -1;
    if (k == 57354 || k == 57355) edges_.tour_page = k == 57354 ? -1 : 1;
    if (k == 'a' || k == 'd') edges_.browse_leg = k == 'a' ? -1 : 1;
    if (k == 13 || k == 10)
      edges_.confirm = true;
    if (k == 27 || k == 'p')
      edges_.pause = true;
    if (k == 9)
      edges_.journal = true;
    if (k == 100003 || k == 'v')
      edges_.debug = true;
  }
};

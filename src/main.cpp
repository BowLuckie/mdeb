#include "linenoise.h"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

using std::cerr;
using std::cout;
using std::string;
using std::vector;

class debugger {
public:
  debugger(string prog_name, pid_t pid)
      : m_prog_name{std::move(prog_name)}, m_pid{pid} {}

  void run();
  void handle_command(const std::string& line);
  void continue_execution();

private:
  string m_prog_name;
  pid_t m_pid;
};

bool is_prefix(const std::string& s, const std::string& of) {
  if (s.size() > of.size()) return false;
  return std::equal(s.begin(), s.end(), of.begin());
}

vector<string> split(const string& s, char delimiter) {
  vector<string> out{};
  std::stringstream ss{s};
  std::string item;

  while (std::getline(ss, item, delimiter)) {
    out.push_back(item);
  }

  return out;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Please enter a program to debug!";
    return EXIT_FAILURE;
  }

  char* prog = argv[1];

  int pid = fork();
  if (pid == 0) {
    if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
      cout << "an error occured whilst activating ptrace" << errno;
      return EXIT_FAILURE;
    }

    execl(prog, prog, nullptr);
  }

  if (pid >= 1) {
    cout << "Started debugging process " << pid << '\n';
    debugger dbg(prog, pid);
    dbg.run();
  }

  return EXIT_SUCCESS;
}

void debugger::run() {
  int wait_status;
  int options = 0;
  waitpid(m_pid, &wait_status, options);

  char* line = nullptr;
  while ((line = linenoise("mdeb> ")) != nullptr) {
    handle_command(line);
    linenoiseHistoryAdd(line);
    linenoiseFree(line);
  }
}

void debugger::handle_command(const std::string& line) {
  auto args = split(line, ' ');
  const auto& command = args[0];

  if (is_prefix(command, "continue")) {
    continue_execution();
  } else {
    std::cerr << "unkown command \"" << line << "\"\n";
  }
} 

void debugger::continue_execution() {
  ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);

  int wait_status;
  int options = 0;
  waitpid(m_pid, &wait_status, options);
}

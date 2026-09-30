#include "linenoise.h"
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

using std::cerr;
using std::cout;
using std::intptr_t;
using std::string;
using std::vector;

class breakpoint {
public:
  breakpoint(pid_t pid, intptr_t addr)
      : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_data{} {}

  void enable();
  void disable();

  intptr_t is_enabled() { return m_enabled; }
  intptr_t get_address() { return m_addr; }

private:
  pid_t m_pid;
  intptr_t m_addr;
  bool m_enabled;
  uint8_t m_saved_data;
};

class debugger {
public:
  debugger(string prog_name, pid_t pid)
      : m_prog_name{std::move(prog_name)}, m_pid{pid} {}
  ~debugger();

  void run();
  void handle_command(const string& line);
  void continue_execution();
  void set_break_addr(intptr_t addr);

private:
  string m_prog_name;
  pid_t m_pid;
  std::unordered_map<intptr_t, breakpoint> m_breakpoints;
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

int execute_debugee(char* prog) {
  if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
    cout << "an error occured whilst activating ptrace" << errno;
    return EXIT_FAILURE;
  }

  execl(prog, prog, nullptr);
  return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Please enter a program to debug!";
    return EXIT_FAILURE;
  }

  char* prog = argv[1];

  int pid = fork();
  if (pid == 0) {
    personality(ADDR_NO_RANDOMIZE);
    execute_debugee(prog);
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
  const string& command = args[0];

  if (is_prefix(command, "continue")) {
    continue_execution();
  } else if (is_prefix(command, "break")) {
    string addr{args[1], 2};
    set_break_addr(std::stol(addr, 0, 16));
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

void breakpoint::enable() {
  auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
  m_saved_data = (uint8_t)(data & 0xff);
  auto int3 = 0xcc;
  auto injected = ((data & ~0xff) | int3);
  ptrace(PTRACE_POKEDATA, m_pid, m_addr, injected);

  m_enabled = true;
}

void breakpoint::disable() {
  auto injected = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
  auto original = ((injected & ~0xff) | m_saved_data);
  ptrace(PTRACE_POKEDATA, original);

  m_enabled = false;
}

void debugger::set_break_addr(intptr_t addr) {
  cout << "set breakpoint at address 0x" << addr << '\n';
  breakpoint bp{m_pid, addr};
  bp.enable();
  m_breakpoints.emplace(addr, bp);
}

debugger::~debugger() { ptrace(PTRACE_KILL, m_pid, nullptr, nullptr); }

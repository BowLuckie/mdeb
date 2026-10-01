#include "linenoise.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

using std::cerr;
using std::intptr_t;
using std::string;
using std::vector;

namespace color {
constexpr const char* reset = "\033[0m";
constexpr const char* red = "\033[31m";
constexpr const char* yellow = "\033[33m";
constexpr const char* aqua = "\033[36m";
} // namespace color

enum class reg : uint8_t {
  rax,
  rbx,
  rcx,
  rdx,
  rdi,
  rsi,
  rbp,
  rsp,
  r8,
  r9,
  r10,
  r11,
  r12,
  r13,
  r14,
  r15,
  rip,
  rflags,
  cs,
  orig_rax,
  fs_base,
  gs_base,
  fs,
  gs,
  ss,
  ds,
  es
};

constexpr std::size_t n_registers = 27;

struct reg_descriptor {
  reg r;
  int dwarf_r;
  std::string name;
};

const std::array<reg_descriptor, n_registers> g_register_descriptors{{
    {reg::r15, 15, "r15"},
    {reg::r14, 14, "r14"},
    {reg::r13, 13, "r13"},
    {reg::r12, 12, "r12"},
    {reg::rbp, 6, "rbp"},
    {reg::rbx, 3, "rbx"},
    {reg::r11, 11, "r11"},
    {reg::r10, 10, "r10"},
    {reg::r9, 9, "r9"},
    {reg::r8, 8, "r8"},
    {reg::rax, 0, "rax"},
    {reg::rcx, 2, "rcx"},
    {reg::rdx, 1, "rdx"},
    {reg::rsi, 4, "rsi"},
    {reg::rdi, 5, "rdi"},
    {reg::orig_rax, -1, "orig_rax"},
    {reg::rip, -1, "rip"},
    {reg::cs, 51, "cs"},
    {reg::rflags, 49, "eflags"},
    {reg::rsp, 7, "rsp"},
    {reg::ss, 52, "ss"},
    {reg::fs_base, 58, "fs_base"},
    {reg::gs_base, 59, "gs_base"},
    {reg::ds, 53, "ds"},
    {reg::es, 50, "es"},
    {reg::fs, 54, "fs"},
    {reg::gs, 55, "gs"},
}};

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
  void continue_execution();
  void handle_command(const string& line);

  void wait_for_signal();

  void dump_registers();
  uint64_t read_memory(intptr_t addr);
  void write_memory(intptr_t addr, uint64_t val);

  void set_break_addr(intptr_t addr);
  void step_over_break();

  uint64_t get_pc();
  void set_pc(uint64_t addr);

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
    cerr << "an error occured whilst activating ptrace" << errno;
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
    return execute_debugee(prog);
  }

  if (pid >= 1) {
    cerr << "Started debugging process " << pid << '\n';
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

uint64_t get_register_value(pid_t pid, reg r) {
  user_regs_struct regs;
  ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

  const reg_descriptor* it =
      std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                   [r](auto&& rd) { return rd.r == r; });

  return *(reinterpret_cast<uint64_t*>(&regs) +
           (it - begin(g_register_descriptors)));
}

void set_register_value(pid_t pid, reg r, uint64_t val) {
  user_regs_struct regs;
  ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

  const reg_descriptor* it =
      std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                   [r](auto&& rd) { return rd.r == r; });

  *(reinterpret_cast<uint64_t*>(&regs) + (it - begin(g_register_descriptors))) =
      val;
  ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

uint64_t get_dwarf_reg(pid_t pid, unsigned regnum) {
  const reg_descriptor* it = std::find_if(
      begin(g_register_descriptors), end(g_register_descriptors),
      [regnum](auto&& rd) { return rd.dwarf_r == static_cast<int>(regnum); });

  if (it == end(g_register_descriptors)) {
    throw std::out_of_range{"unknown dwarf register"};
  }

  return get_register_value(pid, it->r);
}

string get_register_name(reg r) {
  const reg_descriptor* it =
      std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                   [r](auto&& rd) { return rd.r == r; });

  return it->name;
}

reg get_reg_from_name(string& name) {
  const reg_descriptor* it =
      std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                   [name](auto&& rd) { return rd.name == name; });

  return it->r;
}

void debugger::dump_registers() {
  for (const reg_descriptor& rd : g_register_descriptors) {
    std::cerr << color::aqua << rd.name << color::reset << " 0x"
              << std::setfill('0') << std::setw(16) << std::hex
              << get_register_value(m_pid, rd.r) << '\n';
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
  } else if (is_prefix(command, "register")) {
    if (is_prefix(args[1], "dump")) {
      dump_registers();
    } else if (is_prefix(args[1], "read")) {
      std::cerr << get_register_value(m_pid, get_reg_from_name(args[2]))
                << '\n';
    } else if (is_prefix(args[1], "write")) {
      string val{args[3], 2}; // 0xADDR
      set_register_value(m_pid, get_reg_from_name(args[2]),
                         std::stol(val, 0, 16));
    }
  } else if (is_prefix(command, "memory")) {
    const string& saddr{args[2], 2}; // 0xADDR
    auto addr = std::stol(saddr, 0, 16);
    const string& mem_command{args[1]};

    if (is_prefix(mem_command, "read")) {
      cerr << std::hex << read_memory(addr) << std::dec << '\n';
    } else if (is_prefix(mem_command, "write")) {
      string sval{args[3], 2};
      auto val = std::stol(sval, 0, 16);
      write_memory(addr, val);
    }

  } else {
    cerr << "unkown command \"" << line << "\"\n";
  }
}

static std::string signal_name(int sig) {
  if (const char* abbrev = sigabbrev_np(sig)) {
    return std::string("SIG") + abbrev; // "SIGTRAP"
  }
  return "signal " +
         std::to_string(sig); // realtime signals etc. have no abbrev
}

void debugger::wait_for_signal() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);

  if (WIFEXITED(wait_status)) {
    std::cerr << color::aqua << "[mdeb] exited with "
              << WEXITSTATUS(wait_status) << color::reset << '\n';
  } else if (WIFSTOPPED(wait_status)) {
    std::cerr << color::yellow << "[mdeb] stopped by "
              << signal_name(WSTOPSIG(wait_status)) << " at rip=0x" << std::hex
              << get_register_value(m_pid, reg::rip) << std::dec << color::reset
              << '\n';
  } else if (WIFSIGNALED(wait_status)) {
    std::cerr << color::red << "[mdeb] killed by "
              << signal_name(WTERMSIG(wait_status)) << color::reset << '\n';
  }
}

void debugger::continue_execution() {
  step_over_break();
  ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
  wait_for_signal();
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
  ptrace(PTRACE_POKEDATA, m_pid, m_addr, original);

  m_enabled = false;
}

void debugger::set_break_addr(intptr_t addr) {
  cerr << "set breakpoint at address 0x" << std::hex << addr << '\n';
  breakpoint bp{m_pid, addr};
  bp.enable();
  m_breakpoints.emplace(addr, bp);
}

void debugger::step_over_break() {
  // rip is at one address past the possible break
  uint64_t maybe_break_addr = get_pc() - 1;

  auto it = m_breakpoints.find(maybe_break_addr);
  if (it == m_breakpoints.end() || !it->second.is_enabled()) return;

  auto& bp = it->second;
  set_pc(maybe_break_addr);

  bp.disable();
  ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
  int status;
  waitpid(m_pid, &status, 0);
  if (WIFSTOPPED(status)) bp.enable();
}

debugger::~debugger() {
  if (m_pid > 0) {
    kill(m_pid, SIGKILL);
    waitpid(m_pid, nullptr, 0);
  }
}

uint64_t debugger::read_memory(intptr_t addr) {
  return ptrace(PTRACE_PEEKDATA, m_pid, addr, nullptr);
}

void debugger::write_memory(intptr_t addr, uint64_t val) {
  ptrace(PTRACE_POKEDATA, m_pid, addr, val);
}

uint64_t debugger::get_pc() { return get_register_value(m_pid, reg::rip); }

void debugger::set_pc(uint64_t val) {
  set_register_value(m_pid, reg::rip, val);
}

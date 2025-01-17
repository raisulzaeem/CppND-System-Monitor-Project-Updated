#include <unistd.h>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>
#include <numeric>

#include "process.h"
#include "linux_parser.h"

using std::string;
using std::to_string;
using std::vector;

Process::Process(int pid) : 
    pid_(pid), 
    user_(LinuxParser::User(pid)),
    cmd_(LinuxParser::Command(pid)),
    ram_(LinuxParser::Ram(pid)),
    upTime_(LinuxParser::UpTime(pid)) 
    {    
        float herzt = sysconf(_SC_CLK_TCK);
        float total_time = LinuxParser::ActiveJiffies(pid_);
        cpuUtil_ = (total_time/herzt)/upTime_;
    }

// TODO: Return this process's ID
int Process::Pid() { return pid_; }

// TODO: Return this process's CPU utilization
float Process::CpuUtilization() { 
    return cpuUtil_;
}

// TODO: Return the command that generated this process
string Process::Command() { return cmd_; }

// TODO: Return this process's memory utilization
string Process::Ram() { return ram_; }

// TODO: Return the user (name) that generated this process
string Process::User() { return user_; }

// TODO: Return the age of this process (in seconds)
long int Process::UpTime() { return upTime_; }

// TODO: Overload the "less than" comparison operator for Process objects
// REMOVE: [[maybe_unused]] once you define the function
bool Process::operator<(Process const& a) const {
    return cpuUtil_< a.cpuUtil_;
}
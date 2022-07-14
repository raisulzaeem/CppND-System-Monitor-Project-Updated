#include <dirent.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>
#include <numeric>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;


string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}


string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}


vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.emplace_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}


float LinuxParser::MemoryUtilization() { 
  string key, value, unit, line; 
  float memFree, memTotal;

  std::ifstream stream(kProcDirectory + kMeminfoFilename);
  if (stream.is_open()) {
    // We only need two lines of information
    for(int i=0; i<3; i++){
      std::getline(stream, line);
      std::istringstream linestream(line);
      linestream >> key >> value >> unit;
      if(key=="MemTotal:"){
        memTotal = std::stof(value);
      }
      if(key=="MemFree:"){
        memFree = std::stof(value);
      }
    }
  }
  return (memTotal-memFree)/memTotal;
}


long LinuxParser::UpTime() {
  string line, upTimeSeconds;
  std::ifstream stream(kProcDirectory+kUptimeFilename);
  if (stream.is_open()){
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream>>upTimeSeconds;
  }
  return stringToLong(upTimeSeconds);
}


long LinuxParser::Jiffies() { return sysconf(_SC_CLK_TCK) * LinuxParser::UpTime(); }


long LinuxParser::ActiveJiffies(int pid) { 
  string line, temp;
  long value;
  vector<long> cpuData{};
  std::ifstream filestream(kProcDirectory + std::to_string(pid)+kStatFilename);
  if (filestream.is_open()) {
    std::getline(filestream, line); //only first line
    std::istringstream linestream(line);
    for (int i=0; i<13; i++){
      linestream>>temp;
    }
    for (int i=0; i<4; i++){
      linestream>>value;
      cpuData.emplace_back(value);
    }
  }
  return std::accumulate(cpuData.begin(), cpuData.end(), 0L);
 }


long LinuxParser::ActiveJiffies() {
  vector<string> jiffies = CpuUtilization();
  return stringToLong(jiffies[CPUStates::kUser_]) + stringToLong(jiffies[CPUStates::kNice_]) + stringToLong(jiffies[CPUStates::kSystem_]) + 
         stringToLong(jiffies[CPUStates::kIRQ_]) + stringToLong(jiffies[CPUStates::kSoftIRQ_]) + stringToLong(jiffies[CPUStates::kSteal_]);
}


long LinuxParser::IdleJiffies() { 
  string line, cpu;
  long value;
  vector<long> cpuData;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    std::getline(filestream, line); //only first line
    std::istringstream linestream(line);
    linestream >> cpu; // take "cpu" away
    while (linestream >> value) {
      cpuData.emplace_back(value);
    }
  }
  return cpuData.at(CPUStates::kIdle_) + cpuData.at(CPUStates::kIOwait_);
 }


vector<string> LinuxParser::CpuUtilization() { 
  string line, cpu, value;
  vector<string> cpuData;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    std::getline(filestream, line); //only first line
    std::istringstream linestream(line);
    linestream >> cpu; // take "cpu" away
    while (linestream >> value) {
      cpuData.emplace_back(value);
    }
  }
  return cpuData;
}


int LinuxParser::TotalProcesses() {   
  string line, key, value;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    while(std::getline(filestream, line)){
      std::istringstream linestream(line);
      linestream >> key>>value;
      if (key == "processes"){
        break;
      }
    }
  }
  return std::stoi(value); 
}


int LinuxParser::RunningProcesses() { 
  string line, key, value;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    while(std::getline(filestream, line)){
      std::istringstream linestream(line);
      linestream >> key>>value;
      if (key == "procs_running"){
        break;
      }
    }
  }
  return std::stoi(value); 
}


string LinuxParser::Command(int pid) { 
  string cmd;
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + kCmdlineFilename);
  if (filestream.is_open()) {
    std::getline(filestream, cmd);
  }
  return cmd;
}


string LinuxParser::Ram(int pid) { 
  string status, key, ram;
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + kStatusFilename);
  if (filestream.is_open()) {
    while(std::getline(filestream, status)){
      std::istringstream linestream(status);
      linestream>>key;
      // "VmRSS:" is used instead of "VmSize:", because VmSize sums all of the virtual memory from /proc/
      if(key == "VmRSS:") {
        linestream>>ram;
        break;
      }
    }
  }
  return std::to_string(stringToLong(ram)/1000);
}



string LinuxParser::Uid(int pid) {
  string status, key, uid;
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + kStatusFilename);
  if (filestream.is_open()) {
    while(std::getline(filestream, status)){
      std::istringstream linestream(status);
      linestream>>key;
      if(key == "Uid:") {
        linestream>>uid;
        break;
      }
    }
  }
  return uid;
}



string LinuxParser::User(int pid) {
  string uid = Uid(pid);
  string line, id, x, temp, user;
  std::ifstream filestream(kPasswordPath);

  if (filestream.is_open()){
    while(std::getline(filestream, line)){
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      linestream>>temp>>x>>id;

      if(id == uid){
        user = temp;
        break;
      }
    }
  }
  return user;
}



long LinuxParser::UpTime(int pid) {
  string line, value;
  vector<string> values;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    while (linestream >> value) {
      values.emplace_back(value);
    };
  }
  return LinuxParser::UpTime() - (stringToLong(values[21]) / sysconf(_SC_CLK_TCK)); // seconds
}

long LinuxParser::stringToLong(std::string str)
{
  long value{0};
  try
  {
    if(string_val != ""){
      value = std::stol(string_val);
}
  }
  catch(...)
  {
    value = 0;
  }
  return value;
}

#pragma once

#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>

using namespace std;

struct Job {
  int job_number;
  pid_t pid;
  string command_line;
};

inline vector<Job> background_jobs;
inline int next_job_number = 1;

inline int add_job(pid_t pid, string command_line) {
  int job_number = next_job_number++;
  background_jobs.push_back(Job{job_number, pid, std::move(command_line)});
  return job_number;
}
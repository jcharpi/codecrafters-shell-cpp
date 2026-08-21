#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <iostream>
#include <numeric>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <utility>
#include <vector>

using namespace std;

enum class JobStatus { Running, Done };

struct Job {
  int job_number;
  pid_t pid;
  string command_line;
  JobStatus status = JobStatus::Running;
  optional<int> exit_status; // filled in when reaped; bash prints `Exit 2` rather than `Done` for a nonzero exit
};

template <> struct std::formatter<JobStatus> : std::formatter<string_view> {
  auto format(JobStatus status, auto& context) const {
    return std::formatter<string_view>::format(status == JobStatus::Running ? "Running" : "Done", context);
  }
};

inline vector<Job> background_jobs;
inline int next_job_number = 1;

inline int add_job(pid_t pid, string command_line) {
  int job_number = next_job_number++;
  background_jobs.push_back(Job{job_number, pid, std::move(command_line)});
  return job_number;
}

inline char job_marker(ptrdiff_t index) {
  if (index == ssize(background_jobs) - 1) return '+';
  if (index == ssize(background_jobs) - 2) return '-';
  return ' ';
}

inline void print_job_line(ptrdiff_t index) {
  const Job& background_job = background_jobs[index];
  println(cout, "[{}]{}  {:<24}{}", background_job.job_number, job_marker(index), background_job.status,
          background_job.command_line);
}

// Positions into background_jobs, ordered by job number which is the order `jobs`
// prints. We sort a list of positions rather than the jobs themselves because each
// position also represents how recently the job started, which is needed for
// job_marker.
inline vector<ptrdiff_t> background_job_display_order() {
  vector<ptrdiff_t> indices(ssize(background_jobs));
  ranges::iota(indices, 0);
  ranges::sort(indices, {}, [](ptrdiff_t index) { return background_jobs[index].job_number; });
  return indices;
}

inline void reap_background_jobs() {
  for (Job& background_job : background_jobs) {
    int wait_status = 0;
    pid_t result = waitpid(background_job.pid, &wait_status, WNOHANG);
    if (result == 0) continue; // still running

    background_job.status = JobStatus::Done;
    if (result == background_job.pid && WIFEXITED(wait_status)) {
      background_job.exit_status = WEXITSTATUS(wait_status);
    }
  }
}

inline void purge_done_jobs() {
  erase_if(background_jobs, [](const Job& background_job) { return background_job.status == JobStatus::Done; });
}
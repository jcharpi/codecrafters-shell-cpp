#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <utility>
#include <vector>

using namespace std;

enum class JobStatus { Running, Done };

struct Job {
  int job_number;
  pid_t pid;
  string command_line;
  JobStatus status = JobStatus::Running;
  optional<int> exit_status;
};

template <>
struct std::formatter<JobStatus> : std::formatter<string_view> {
  auto format(JobStatus status, auto& context) const {
    return std::formatter<string_view>::format(status == JobStatus::Running ? "Running" : "Done", context);
  }
};

inline vector<Job> background_jobs;
inline int next_job_number = 1;

inline char get_indicator(ptrdiff_t index) {
  if (index == ssize(background_jobs) - 1) return '+';
  if (index == ssize(background_jobs) - 2) return '-';
  return ' ';
}

// Positions into background_jobs, ordered by job number which is the order `jobs`
// prints. We sort a list of positions rather than the jobs themselves because each
// position also represents how recently the job started, which is needed for
// get_indicator.
inline vector<ptrdiff_t> background_job_display_order() {
  vector<ptrdiff_t> indices(ssize(background_jobs));
  ranges::iota(indices, 0);
  ranges::sort(indices, {}, [](ptrdiff_t index) { return background_jobs[index].job_number; });
  return indices;
}

inline int add_job(pid_t pid, string command_line) {
  int job_number = next_job_number++;
  background_jobs.push_back(Job{job_number, pid, std::move(command_line)});
  return job_number;
}
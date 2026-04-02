/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Options {
  fs::path verifier_bin_dir = "";
  fs::path kernel_root = "";
  fs::path results_dir = "blas/reference/full_verification";
  std::vector<std::string> levels{"1", "2", "3"};
  std::vector<std::string> modes{"sysemu", "silicon"};
  int kernel_launch_timeout = 30;
  double epsilon = 1.0e-5;
  std::vector<size_t> level1_num_elements{16, 64, 128, 256, 512};
  std::vector<size_t> level2_problem_dims{16, 32, 64, 128, 256};
  std::vector<size_t> level3_problem_dims{16, 32, 64, 128};
  size_t level2_band_width = 2;
  size_t sysemu_level1_num_elements_limit = 0;
  size_t sysemu_level2_problem_dim_limit = 0;
  size_t sysemu_level3_problem_dim_limit = 128;
};

struct RunRecord {
  std::string level;
  std::string mode;
  size_t problem_size = 0;
  fs::path verifier;
  fs::path kernel_root;
  fs::path host_results_path;
  fs::path device_results_path;
  fs::path log_path;
  int exit_code = -1;
  bool skipped = false;
  std::string skip_reason;
};

std::vector<std::string> splitCsv(const std::string& value) {
  std::vector<std::string> items;
  size_t begin = 0;
  while (begin <= value.size()) {
    const size_t end = value.find(',', begin);
    const std::string token = value.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    if (!token.empty()) {
      items.push_back(token);
    }
    if (end == std::string::npos) {
      break;
    }
    begin = end + 1;
  }
  return items;
}

std::vector<size_t> parseSizeCsv(const std::string& value) {
  std::vector<size_t> sizes;
  for (const auto& token : splitCsv(value)) {
    sizes.push_back(static_cast<size_t>(std::stoul(token)));
  }
  return sizes;
}

bool isSupportedLevel(const std::string& level) {
  return level == "1" || level == "2" || level == "3";
}

bool isSupportedMode(const std::string& mode) {
  return mode == "sysemu" || mode == "silicon" || mode == "hardware";
}

std::string normalizeModeArg(const std::string& mode) {
  return mode == "hardware" ? "silicon" : mode;
}

std::string modeLabel(const std::string& mode) {
  return mode == "silicon" ? "hardware" : mode;
}

fs::path defaultVerifierBinDir() {
  std::error_code ec;
  const fs::path self = fs::read_symlink("/proc/self/exe", ec);
  if (!ec) {
    return self.parent_path();
  }
  return "/tmp/blas-reference-host-build";
}

fs::path defaultKernelRoot() {
  if (const char* envRoot = std::getenv("ET_BLAS_KERNEL_ROOT")) {
    return envRoot;
  }

  static constexpr std::string_view candidates[] = {
    "/tmp/blas-device-build/custom-kernels/reference/fp32",
    "/opt/et/kernels/blas/reference/fp32",
  };

  for (const auto candidate : candidates) {
    if (fs::exists(candidate)) {
      return candidate;
    }
  }

  return candidates[0];
}

Options parseArgs(int argc, char** argv) {
  static constexpr const char* helpMsg =
    "Usage: [options]\n\n"
    "Run BLAS reference verification across Levels 1-3 for sysemu and hardware.\n\n"
    "Optional switches:\n"
    "      --verifier_bin_dir       directory containing per-level verifier binaries\n"
    "  -k, --kernel_root            root containing level1/level2/level3 kernel directories\n"
    "  -o, --results_dir            directory where markdown results and summary are written\n"
    "      --levels                 comma-separated levels to run (default: 1,2,3)\n"
    "      --modes                  comma-separated modes to run (default: sysemu,silicon)\n"
    "  -t, --kernel_launch_timeout  timeout (in seconds) to wait for kernel completion\n"
    "  -e, --epsilon                comparison tolerance for float results\n"
    "  -n, --level1_num_elements    comma-separated Level 1 element counts\n"
    "      --level2_problem_dim     comma-separated Level 2 problem dimensions\n"
    "      --level2_band_width      band width for Level 2 banded routines\n"
    "      --level3_problem_dim     comma-separated Level 3 problem dimensions\n"
    "      --sysemu_level1_num_elements_limit  skip sysemu Level 1 above this element count (0 disables)\n"
    "      --sysemu_level2_problem_dim_limit   skip sysemu Level 2 above this problem dimension (0 disables)\n"
    "      --sysemu_level3_problem_dim_limit   skip sysemu Level 3 above this problem dimension (default: 128)\n";

  static constexpr const char* shortOpts = "k:o:t:e:n:h";
  static const std::vector<option> longOpts{
    {"verifier_bin_dir", required_argument, nullptr, 1000},
    {"kernel_root", required_argument, nullptr, 'k'},
    {"results_dir", required_argument, nullptr, 'o'},
    {"levels", required_argument, nullptr, 1001},
    {"modes", required_argument, nullptr, 1002},
    {"kernel_launch_timeout", required_argument, nullptr, 't'},
    {"epsilon", required_argument, nullptr, 'e'},
    {"level1_num_elements", required_argument, nullptr, 'n'},
    {"level2_problem_dim", required_argument, nullptr, 1003},
    {"level2_band_width", required_argument, nullptr, 1004},
    {"level3_problem_dim", required_argument, nullptr, 1005},
    {"sysemu_level1_num_elements_limit", required_argument, nullptr, 1006},
    {"sysemu_level2_problem_dim_limit", required_argument, nullptr, 1007},
    {"sysemu_level3_problem_dim_limit", required_argument, nullptr, 1008},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0},
  };

  Options opts;
  opts.verifier_bin_dir = defaultVerifierBinDir();
  opts.kernel_root = defaultKernelRoot();

  opterr = 0;
  int optionIndex = 0;
  int ret = 0;

  while ((ret = getopt_long(argc, argv, shortOpts, longOpts.data(), &optionIndex)) != -1) {
    switch (ret) {
    case 1000:
      opts.verifier_bin_dir = optarg;
      break;
    case 'k':
      opts.kernel_root = optarg;
      break;
    case 'o':
      opts.results_dir = optarg;
      break;
    case 1001:
      opts.levels = splitCsv(optarg);
      break;
    case 1002:
      opts.modes = splitCsv(optarg);
      break;
    case 't':
      opts.kernel_launch_timeout = std::stoi(optarg);
      break;
    case 'e':
      opts.epsilon = std::stod(optarg);
      break;
    case 'n':
      opts.level1_num_elements = parseSizeCsv(optarg);
      break;
    case 1003:
      opts.level2_problem_dims = parseSizeCsv(optarg);
      break;
    case 1004:
      opts.level2_band_width = static_cast<size_t>(std::stoul(optarg));
      break;
    case 1005:
      opts.level3_problem_dims = parseSizeCsv(optarg);
      break;
    case 1006:
      opts.sysemu_level1_num_elements_limit = static_cast<size_t>(std::stoul(optarg));
      break;
    case 1007:
      opts.sysemu_level2_problem_dim_limit = static_cast<size_t>(std::stoul(optarg));
      break;
    case 1008:
      opts.sysemu_level3_problem_dim_limit = static_cast<size_t>(std::stoul(optarg));
      break;
    case 'h':
      std::cout << helpMsg << std::endl;
      std::exit(0);
    default:
      throw std::runtime_error("Unknown option");
    }
  }

  if (opts.levels.empty()) {
    throw std::runtime_error("At least one level must be requested");
  }
  if (opts.modes.empty()) {
    throw std::runtime_error("At least one mode must be requested");
  }
  for (const auto& level : opts.levels) {
    if (!isSupportedLevel(level)) {
      throw std::runtime_error("Unsupported level: " + level);
    }
  }
  for (const auto& mode : opts.modes) {
    if (!isSupportedMode(mode)) {
      throw std::runtime_error("Unsupported mode: " + mode);
    }
  }

  return opts;
}

const std::vector<size_t>& problemSizesForLevel(const Options& opts, const std::string& level) {
  if (level == "1") {
    return opts.level1_num_elements;
  }
  if (level == "2") {
    return opts.level2_problem_dims;
  }
  return opts.level3_problem_dims;
}

size_t sysemuProblemLimitForLevel(const Options& opts, const std::string& level) {
  if (level == "1") {
    return opts.sysemu_level1_num_elements_limit;
  }
  if (level == "2") {
    return opts.sysemu_level2_problem_dim_limit;
  }
  return opts.sysemu_level3_problem_dim_limit;
}

std::string sysemuSkipReason(const Options& opts, const std::string& level, const std::string& mode, size_t problemSize) {
  if (normalizeModeArg(mode) != "sysemu") {
    return "";
  }

  const size_t limit = sysemuProblemLimitForLevel(opts, level);
  if (limit == 0) {
    return "";
  }

  if (problemSize <= limit) {
    return "";
  }

  std::ostringstream reason;
  if (level == "1") {
    reason << "policy skip: sysemu Level 1 is limited to " << limit
           << " elements, requested " << problemSize;
  } else {
    reason << "policy skip: sysemu Level " << level << " is limited to problem_dim " << limit
           << ", requested " << problemSize;
  }
  return reason.str();
}

fs::path verifierPath(const fs::path& verifierBinDir, const std::string& level) {
  return verifierBinDir / ("blas_reference_level" + level + "_verifier");
}

std::vector<std::string> buildArgs(const Options& opts, const std::string& level, const std::string& mode,
                                   size_t problemSize, const fs::path& hostResults, const fs::path& deviceResults) {
  const std::string normalizedMode = normalizeModeArg(mode);
  const fs::path levelKernelRoot = opts.kernel_root / ("level" + level);

  std::vector<std::string> args{
    verifierPath(opts.verifier_bin_dir, level).string(),
    "--device_type", normalizedMode,
    "--kernel_root", levelKernelRoot.string(),
    "--host_results_path", hostResults.string(),
    "--device_results_path", deviceResults.string(),
    "--kernel_launch_timeout", std::to_string(opts.kernel_launch_timeout),
    "--epsilon", std::to_string(opts.epsilon),
  };

  if (level == "1") {
    args.emplace_back("--num_elements");
    args.emplace_back(std::to_string(problemSize));
  } else if (level == "2") {
    args.emplace_back("--problem_dim");
    args.emplace_back(std::to_string(problemSize));
    args.emplace_back("--band_width");
    args.emplace_back(std::to_string(opts.level2_band_width));
  } else if (level == "3") {
    args.emplace_back("--problem_dim");
    args.emplace_back(std::to_string(problemSize));
  }

  return args;
}

int runChild(const std::vector<std::string>& args, const fs::path& logPath) {
  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (const auto& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  const pid_t child = fork();
  if (child < 0) {
    throw std::runtime_error("fork() failed");
  }
  if (child == 0) {
    const int logFd = ::open(logPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logFd < 0) {
      std::perror("open");
      _exit(127);
    }
    if (dup2(logFd, STDOUT_FILENO) < 0 || dup2(logFd, STDERR_FILENO) < 0) {
      std::perror("dup2");
      _exit(127);
    }
    close(logFd);
    execv(argv.front(), argv.data());
    std::perror("execv");
    _exit(127);
  }

  int status = 0;
  if (waitpid(child, &status, 0) < 0) {
    throw std::runtime_error("waitpid() failed");
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return 1;
}

void writeSummary(const fs::path& summaryPath, const Options& opts, const std::vector<RunRecord>& records) {
  if (summaryPath.has_parent_path()) {
    fs::create_directories(summaryPath.parent_path());
  }

  std::ofstream out(summaryPath);
  if (!out.is_open()) {
    throw std::runtime_error("Unable to open summary file: " + summaryPath.string());
  }

  out << "# BLAS Reference Full Verification\n\n";
  out << "- verifier_bin_dir: `" << opts.verifier_bin_dir.string() << "`\n";
  out << "- kernel_root: `" << opts.kernel_root.string() << "`\n";
  out << "- results_dir: `" << opts.results_dir.string() << "`\n";
  out << "- kernel_launch_timeout: `" << opts.kernel_launch_timeout << "`\n";
  out << "- epsilon: `" << opts.epsilon << "`\n";
  out << "- level1_num_elements: `";
  for (size_t i = 0; i < opts.level1_num_elements.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << opts.level1_num_elements[i];
  }
  out << "`\n\n";
  out << "- level2_problem_dim: `";
  for (size_t i = 0; i < opts.level2_problem_dims.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << opts.level2_problem_dims[i];
  }
  out << "`\n";
  out << "- level2_band_width: `" << opts.level2_band_width << "`\n";
  out << "- level3_problem_dim: `";
  for (size_t i = 0; i < opts.level3_problem_dims.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << opts.level3_problem_dims[i];
  }
  out << "`\n\n";
  out << "- sysemu_level1_num_elements_limit: `" << opts.sysemu_level1_num_elements_limit << "`\n";
  out << "- sysemu_level2_problem_dim_limit: `" << opts.sysemu_level2_problem_dim_limit << "`\n";
  out << "- sysemu_level3_problem_dim_limit: `" << opts.sysemu_level3_problem_dim_limit << "`\n\n";

  out << "| level | mode | size | status | host results | device results | log |\n";
  out << "| --- | --- | ---: | --- | --- | --- | --- |\n";
  for (const auto& record : records) {
    const std::string status = record.skipped
      ? ("skipped (" + record.skip_reason + ")")
      : (record.exit_code == 0 ? "ok" : ("failed (" + std::to_string(record.exit_code) + ")"));
    const std::string hostPath = record.skipped ? "-" : ("`" + record.host_results_path.string() + "`");
    const std::string devicePath = record.skipped ? "-" : ("`" + record.device_results_path.string() + "`");
    const std::string logPath = "`" + record.log_path.string() + "`";
    out << "| level" << record.level
        << " | " << record.mode
        << " | " << record.problem_size
        << " | " << status
        << " | " << hostPath
        << " | " << devicePath
        << " | " << logPath << " |\n";
  }
}

} // namespace

int main(int argc, char** argv) {
  try {
    const Options opts = parseArgs(argc, argv);

    std::vector<RunRecord> records;
    bool allOk = true;

    for (const auto& mode : opts.modes) {
      const std::string normalizedMode = normalizeModeArg(mode);
      const std::string label = modeLabel(normalizedMode);
      for (const auto& level : opts.levels) {
        const auto& problemSizes = problemSizesForLevel(opts, level);
        const fs::path verifier = verifierPath(opts.verifier_bin_dir, level);
        if (!fs::exists(verifier)) {
          throw std::runtime_error("Missing verifier binary: " + verifier.string());
        }

        const fs::path levelKernelRoot = opts.kernel_root / ("level" + level);
        if (!fs::exists(levelKernelRoot)) {
          throw std::runtime_error("Missing kernel root: " + levelKernelRoot.string());
        }

        for (const auto problemSize : problemSizes) {
          const fs::path resultDir = opts.results_dir / label / ("level" + level) / std::to_string(problemSize);
          const fs::path hostResults = resultDir / "host_results.md";
          const fs::path deviceResults = resultDir / "device_results.md";
          const fs::path logPath = resultDir / "verifier.log";
          fs::create_directories(resultDir);

          const std::string skipReason = sysemuSkipReason(opts, level, normalizedMode, problemSize);
          if (!skipReason.empty()) {
            std::ofstream log(logPath);
            if (!log.is_open()) {
              throw std::runtime_error("Unable to open log file: " + logPath.string());
            }
            log << skipReason << '\n';

            std::cout << "[skip] level" << level << " " << label << " size " << problemSize << ": " << skipReason << std::endl;

            RunRecord record;
            record.level = level;
            record.mode = label;
            record.problem_size = problemSize;
            record.verifier = verifier;
            record.kernel_root = levelKernelRoot;
            record.host_results_path = hostResults;
            record.device_results_path = deviceResults;
            record.log_path = logPath;
            record.exit_code = 0;
            record.skipped = true;
            record.skip_reason = skipReason;
            records.push_back(record);
            continue;
          }

          const auto args = buildArgs(opts, level, normalizedMode, problemSize, hostResults, deviceResults);

          std::ostringstream cmd;
          for (size_t i = 0; i < args.size(); ++i) {
            if (i != 0) {
              cmd << ' ';
            }
            cmd << args[i];
          }
          std::cout << "[run] level" << level << " " << label << " size " << problemSize << ": " << cmd.str() << std::endl;

          RunRecord record;
          record.level = level;
          record.mode = label;
          record.problem_size = problemSize;
          record.verifier = verifier;
          record.kernel_root = levelKernelRoot;
          record.host_results_path = hostResults;
          record.device_results_path = deviceResults;
          record.log_path = logPath;
          record.exit_code = runChild(args, logPath);
          records.push_back(record);

          if (record.exit_code != 0) {
            allOk = false;
          }
        }
      }
    }

    const fs::path summaryPath = opts.results_dir / "summary.md";
    writeSummary(summaryPath, opts, records);

    if (!allOk) {
      std::cerr << "BLAS reference full verification failed. See " << summaryPath << std::endl;
      return 1;
    }

    std::cout << "BLAS reference full verification passed. Summary written to " << summaryPath << std::endl;
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}

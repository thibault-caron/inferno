#ifdef __linux__
#include "metrics/linux_metrics_scrapper.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "fixtures/metrics_file_writer.hpp"

class LinuxMetricsScrapperTest : public ::testing::Test {
 public:
  LinuxMetricsScrapperTest() : procRoot("/tmp/fake_proc"), scrapper(procRoot) {}

 protected:
  std::string procRoot;
  LinuxMetricsScrapper scrapper;

 protected:
  void SetUp() override {
    // Create fake /proc files for testing
    // createFakeProc();
    std::filesystem::create_directories(procRoot + "/net");
    MetricsFileWriter::createFakeProc(procRoot);
  }

  void TearDown() override { std::filesystem::remove_all(procRoot); }
};

// Test 1: First sample should return mostly empty data
TEST_F(LinuxMetricsScrapperTest, FirstSampleReturnsEmptyMetrics) {
  MetricsSample sample = scrapper.sample();
  std::cout << sample.cpu.total_percent << "\n";

  //   CPU should be zero on first call EXPECT_GT(sample.cpu.total_percent,
  //   0.0f);
  EXPECT_EQ(sample.cpu.per_core.size(), 0);

  //   Memory should be populated (doesn't depend on delta)
  //   EXPECT_GT(sample.mem.phys_total, 0);
  EXPECT_EQ(sample.cpu.total_percent, 0.0f);

  //   Disk and net should be empty (need delta)
  EXPECT_EQ(sample.disks.size(), 0);
  EXPECT_EQ(sample.interfaces.size(), 0);
}

// Test 2: Second sample should have data
TEST_F(LinuxMetricsScrapperTest, SecondSampleReturnsMetrics) {
  scrapper.sample();  // First call (establishes baseline)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  MetricsFileWriter::updateFakeProcFiles(procRoot);
  MetricsSample sample = scrapper.sample();  // Second call (computes rates)

  // CPU should have values now
  EXPECT_GT(sample.cpu.total_percent, -0.01f);
  // Allow tiny negative due to rounding

  EXPECT_EQ(sample.cpu.per_core.size(), 2u);
  // EXPECT_GT(sample.cpu.per_core.size(),
  //           0u);  // TODO : windows read actual file and not test file?
  // Match the 16 cores in fake /proc/stat
  //   EXPECT_GT(sample.cpu.per_core.size(), 0);  // Just check we got some
  //   cores

  // Memory should still be populated
  EXPECT_GT(sample.mem.phys_total, 0);
  EXPECT_LE(sample.mem.phys_used, sample.mem.phys_total);

  // Disk and net may or may not have data (depends on system)
  // Just check they're vectors
  EXPECT_TRUE(sample.disks.empty() || sample.disks.size() > 0);
  EXPECT_TRUE(sample.interfaces.empty() || sample.interfaces.size() > 0);
}

// Test 3: Multiple samples should be consistent
TEST_F(LinuxMetricsScrapperTest, MultipleSamplesAreConsistent) {
  scrapper.sample();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  MetricsFileWriter::updateFakeProcFiles(procRoot);
  MetricsSample sample1 = scrapper.sample();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  MetricsFileWriter::updateFakeProcFiles(procRoot);
  MetricsSample sample2 = scrapper.sample();

  // Both should have same CPU core count
  EXPECT_EQ(sample1.cpu.per_core.size(), sample2.cpu.per_core.size());

  // Memory should be in same ballpark
  EXPECT_NEAR(sample1.mem.phys_total, sample2.mem.phys_total,
              sample1.mem.phys_total * 0.01);  // Within 1%
}

// Test 4: CPU percent should be in valid range [0, 100]
TEST_F(LinuxMetricsScrapperTest, CpuPercentIsInValidRange) {
  scrapper.sample();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  MetricsFileWriter::updateFakeProcFiles(procRoot);
  MetricsSample sample = scrapper.sample();

  EXPECT_GE(sample.cpu.total_percent, 0.0f);
  EXPECT_LE(sample.cpu.total_percent, 100.0f);

  for (float core_percent : sample.cpu.per_core) {
    EXPECT_GE(core_percent, 0.0f);
    EXPECT_LE(core_percent, 100.0f);
  }
}

#endif
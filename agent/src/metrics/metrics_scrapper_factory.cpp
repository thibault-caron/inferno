// metrics/metrics_scrapper_factory.cpp
#include "metrics/metrics_scrapper_factory.hpp"

#ifdef _WIN32
  #include "metrics/windows_metrics_scrapper.hpp"
#else
  #include "metrics/linux_metrics_scrapper.hpp"
#endif

std::unique_ptr<IMetricsScrapper> MetricsScrapperFactory::create() {
#ifdef _WIN32
    return std::make_unique<WindowsMetricsScrapper>();
#else
    return std::make_unique<LinuxMetricsScrapper>();
#endif
}
// metrics/metrics_scrapper_factory.hpp
#ifndef METRICS_SCRAPPER_FACTORY_HPP
#define METRICS_SCRAPPER_FACTORY_HPP

#include <memory>
#include "metrics/i_metrics_scrapper.hpp"

class MetricsScrapperFactory {
public:
    static std::unique_ptr<IMetricsScrapper> create();
};

#endif
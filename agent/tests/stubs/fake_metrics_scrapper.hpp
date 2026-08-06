#ifndef FAKE_METRICS_SCRAPPER_HPP
#define FAKE_METRICS_SCRAPPER_HPP

#include "metrics/i_metrics_scrapper.hpp"
#include "builders/metrics_builder.hpp"

class FakeMetricsScrapper : public IMetricsScrapper {
public:
    MetricsSample cannedSample = MetricsBuilder::createMetricsSample();
    int callCount = 0;

    MetricsSample sample() override {
        callCount++;
        return cannedSample;
    }
};

#endif
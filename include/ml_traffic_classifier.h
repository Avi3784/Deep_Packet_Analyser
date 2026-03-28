#ifndef ML_TRAFFIC_CLASSIFIER_H
#define ML_TRAFFIC_CLASSIFIER_H

#include "types.h"
#include <array>
#include <string>
#include <vector>

namespace DPI {

class MLTrafficClassifier {
public:
    static constexpr size_t kFeatureCount = 9;

    struct Node {
        bool is_leaf = true;
        int feature_index = -1;
        double threshold = 0.0;
        int left = -1;
        int right = -1;
        AppType label = AppType::UNKNOWN;
    };

    bool loadModel(const std::string& path);
    bool isLoaded() const { return loaded_; }

    AppType predict(const std::array<double, kFeatureCount>& features) const;

    static std::array<double, kFeatureCount> buildFeatures(const PacketJob& job,
                                                           const Connection& conn,
                                                           bool has_sni,
                                                           bool has_http_host);

private:
    std::vector<Node> nodes_;
    int root_index_ = -1;
    bool loaded_ = false;
};

} // namespace DPI

#endif // ML_TRAFFIC_CLASSIFIER_H

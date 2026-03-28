#include "ml_traffic_classifier.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace DPI {

namespace {

bool parseAppTypeToken(const std::string& token, AppType& out) {
    auto normalize = [](const std::string& value) {
        std::string norm;
        norm.reserve(value.size());
        for (unsigned char c : value) {
            if (std::isalnum(c)) {
                norm.push_back(static_cast<char>(std::toupper(c)));
            }
        }
        return norm;
    };

    const std::string token_norm = normalize(token);
    for (int i = 0; i < static_cast<int>(AppType::APP_COUNT); ++i) {
        AppType candidate = static_cast<AppType>(i);
        std::string candidate_norm = normalize(appTypeToString(candidate));
        if (token_norm == candidate_norm) {
            out = candidate;
            return true;
        }
    }

    if (token_norm == "TWITTER") {
        out = AppType::TWITTER;
        return true;
    }

    return false;
}

} // namespace

bool MLTrafficClassifier::loadModel(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        loaded_ = false;
        return false;
    }

    std::string header;
    std::getline(in, header);
    if (header != "DPI_ML_TREE_V1") {
        std::cerr << "[MLClassifier] Invalid model header\n";
        loaded_ = false;
        return false;
    }

    size_t feature_count = 0;
    if (!(in >> feature_count) || feature_count != kFeatureCount) {
        std::cerr << "[MLClassifier] Feature count mismatch\n";
        loaded_ = false;
        return false;
    }

    int root_index = -1;
    size_t node_count = 0;
    if (!(in >> root_index >> node_count) || node_count == 0) {
        std::cerr << "[MLClassifier] Invalid tree metadata\n";
        loaded_ = false;
        return false;
    }

    std::vector<Node> nodes(node_count);
    for (size_t i = 0; i < node_count; ++i) {
        int index = -1;
        int is_leaf_int = 0;
        std::string label_token;

        if (!(in >> index >> is_leaf_int
                 >> nodes[i].feature_index >> nodes[i].threshold
                 >> nodes[i].left >> nodes[i].right >> label_token)) {
            std::cerr << "[MLClassifier] Invalid node record\n";
            loaded_ = false;
            return false;
        }

        if (index < 0 || static_cast<size_t>(index) >= node_count) {
            std::cerr << "[MLClassifier] Invalid node index\n";
            loaded_ = false;
            return false;
        }

        Node n;
        n.is_leaf = (is_leaf_int != 0);
        n.feature_index = nodes[i].feature_index;
        n.threshold = nodes[i].threshold;
        n.left = nodes[i].left;
        n.right = nodes[i].right;

        if (!parseAppTypeToken(label_token, n.label)) {
            std::cerr << "[MLClassifier] Unknown label token: " << label_token << "\n";
            loaded_ = false;
            return false;
        }

        nodes[static_cast<size_t>(index)] = n;
    }

    if (root_index < 0 || static_cast<size_t>(root_index) >= node_count) {
        std::cerr << "[MLClassifier] Invalid root node index\n";
        loaded_ = false;
        return false;
    }

    nodes_ = std::move(nodes);
    root_index_ = root_index;
    loaded_ = true;
    return true;
}

AppType MLTrafficClassifier::predict(const std::array<double, kFeatureCount>& features) const {
    if (!loaded_ || root_index_ < 0 || static_cast<size_t>(root_index_) >= nodes_.size()) {
        return AppType::UNKNOWN;
    }

    int idx = root_index_;
    for (int guard = 0; guard < 128; ++guard) {
        if (idx < 0 || static_cast<size_t>(idx) >= nodes_.size()) {
            return AppType::UNKNOWN;
        }

        const Node& node = nodes_[static_cast<size_t>(idx)];
        if (node.is_leaf) {
            return node.label;
        }

        if (node.feature_index < 0 || static_cast<size_t>(node.feature_index) >= kFeatureCount) {
            return AppType::UNKNOWN;
        }

        if (features[static_cast<size_t>(node.feature_index)] <= node.threshold) {
            idx = node.left;
        } else {
            idx = node.right;
        }
    }

    return AppType::UNKNOWN;
}

std::array<double, MLTrafficClassifier::kFeatureCount> MLTrafficClassifier::buildFeatures(
    const PacketJob& job,
    const Connection& conn,
    bool has_sni,
    bool has_http_host) {

    std::array<double, kFeatureCount> features{};

    features[0] = static_cast<double>(job.tuple.protocol);
    features[1] = static_cast<double>(job.tuple.dst_port);
    features[2] = static_cast<double>(job.payload_length);
    features[3] = has_sni ? 1.0 : 0.0;
    features[4] = has_http_host ? 1.0 : 0.0;
    features[5] = static_cast<double>(conn.sni.size());
    features[6] = (job.tuple.dst_port == 53 || job.tuple.src_port == 53) ? 1.0 : 0.0;
    features[7] = (job.tuple.dst_port == 443) ? 1.0 : 0.0;
    features[8] = (job.tuple.dst_port == 80) ? 1.0 : 0.0;

    return features;
}

} // namespace DPI

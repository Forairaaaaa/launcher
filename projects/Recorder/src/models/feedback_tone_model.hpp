#pragma once

#include <memory>

namespace recorder {

class FeedbackToneModel {
public:
    FeedbackToneModel();
    ~FeedbackToneModel();

    FeedbackToneModel(const FeedbackToneModel&)            = delete;
    FeedbackToneModel& operator=(const FeedbackToneModel&) = delete;

    void playHint();

private:
    struct Impl;

    std::unique_ptr<Impl> _impl;
};

}  // namespace recorder

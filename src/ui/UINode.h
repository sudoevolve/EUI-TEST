#pragma once

#include "UIPrimitive.h"
#include <cstdint>
#include <string>
#include <utility>

namespace EUINEO {

class UINode {
public:
    explicit UINode(std::string key) : key_(std::move(key)) {}
    virtual ~UINode() = default;

    const std::string& key() const {
        return key_;
    }

    void beginCompose(std::uint64_t composeStamp) {
        composeStamp_ = composeStamp;
        resetDefaults();
    }

    bool composedIn(std::uint64_t composeStamp) const {
        return composeStamp_ == composeStamp;
    }

    UIPrimitive& primitive() {
        return primitive_;
    }

    const UIPrimitive& primitive() const {
        return primitive_;
    }

    int zIndex() const {
        return primitive_.zIndex;
    }

    bool visible() const {
        return primitive_.visible;
    }

    virtual const char* typeName() const = 0;
    virtual void update() = 0;
    virtual void draw() = 0;

protected:
    virtual void resetDefaults() = 0;

    UIPrimitive primitive_;

private:
    std::string key_;
    std::uint64_t composeStamp_ = 0;
};

} // namespace EUINEO

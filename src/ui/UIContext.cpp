#include "UIContext.h"
#include <algorithm>

namespace EUINEO {

namespace {

UIClipRect IntersectClipRect(const UIClipRect& lhs, const UIClipRect& rhs) {
    const float x1 = std::max(lhs.x, rhs.x);
    const float y1 = std::max(lhs.y, rhs.y);
    const float x2 = std::min(lhs.x + lhs.width, rhs.x + rhs.width);
    const float y2 = std::min(lhs.y + lhs.height, rhs.y + rhs.height);

    UIClipRect clipped;
    clipped.x = x1;
    clipped.y = y1;
    clipped.width = std::max(0.0f, x2 - x1);
    clipped.height = std::max(0.0f, y2 - y1);
    return clipped;
}

} // namespace

void UIContext::begin(const std::string& pageId) {
    pageId_ = pageId;
    ++composeStamp_;
    order_.clear();
    clipStack_.clear();
    treeChanged_ = false;
}

void UIContext::end() {
    for (auto it = nodes_.begin(); it != nodes_.end();) {
        if (!it->second->composedIn(composeStamp_)) {
            treeChanged_ = true;
            it = nodes_.erase(it);
            continue;
        }
        ++it;
    }

    if (treeChanged_) {
        Renderer::InvalidateAll();
    }
}

void UIContext::pushClip(float x, float y, float width, float height) {
    UIClipRect clip;
    clip.x = x;
    clip.y = y;
    clip.width = width;
    clip.height = height;

    if (!clipStack_.empty()) {
        clip = IntersectClipRect(clipStack_.back(), clip);
    }

    clipStack_.push_back(clip);
}

void UIContext::popClip() {
    if (!clipStack_.empty()) {
        clipStack_.pop_back();
    }
}

void UIContext::update() {
    for (UINode* node : order_) {
        if (node->visible()) {
            node->update();
        }
    }
}

void UIContext::draw() {
    std::vector<UINode*> drawOrder = order_;
    std::stable_sort(drawOrder.begin(), drawOrder.end(), [](const UINode* lhs, const UINode* rhs) {
        return lhs->zIndex() < rhs->zIndex();
    });

    for (UINode* node : drawOrder) {
        if (node->visible()) {
            node->draw();
        }
    }
}

} // namespace EUINEO

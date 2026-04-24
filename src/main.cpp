/* Special thanks to flurrybun for the code refactor! */

#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>

using namespace geode::prelude;

namespace {
    constexpr float kRodBallScale = 0.25f;

    bool isRodBall(GameObject* obj) {
        auto* sprite = typeinfo_cast<CCSprite*>(obj);
        auto* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
        if (!sprite || !cache) return false;

        if (auto* frame = cache->spriteFrameByName("rod_ball_01_001.png"); frame && sprite->isFrameDisplayed(frame)) return true;
        if (auto* frame = cache->spriteFrameByName("rod_ball_02_001.png"); frame && sprite->isFrameDisplayed(frame)) return true;
        if (auto* frame = cache->spriteFrameByName("rod_ball_03_001.png"); frame && sprite->isFrameDisplayed(frame)) return true;

        return false;
    }

    void disablePulse(GameObject* obj) {
        obj->m_usesAudioScale = false;
        obj->m_hasNoAudioScale = true;
        obj->m_customAudioScale = false;
        obj->m_minAudioScale = 1.0f;
        obj->m_maxAudioScale = 1.0f;
    }
}

class $modify(GameObject) {
    struct Fields {
        bool m_isRodBall = false;
    };

    $override
    void customSetup() {
        GameObject::customSetup();

        if (!Mod::get()->getSettingValue<bool>("enabled")) return;

        m_fields->m_isRodBall = isRodBall(this);
        bool gameplay = m_classType == GameObjectClassType::Game;

        if (m_fields->m_isRodBall) {
            if (!Mod::get()->getSettingValue<bool>("disable-decoration-pulses")) return;

            disablePulse(this);

            /* Removing the pulsing makes the rod "balls" appear too big. Scale them down. */
            GameObject::setScaleX(this->getScaleX() * kRodBallScale);
            GameObject::setScaleY(this->getScaleY() * kRodBallScale);
            return;
        }

        if (gameplay) {
            if (!Mod::get()->getSettingValue<bool>("disable-gameplay-pulses")) return;
        } else {
            if (!Mod::get()->getSettingValue<bool>("disable-decoration-pulses")) return;
        }

        disablePulse(this);
    }

    /* Override scale after setup to preserve custom scale. */
    void setScale(float scale) {
        if (
            m_fields->m_isRodBall &&
            Mod::get()->getSettingValue<bool>("enabled") &&
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses")
        ) {
            GameObject::setScale(scale * kRodBallScale);
            disablePulse(this);
            return;
        }

        GameObject::setScale(scale);
    }

    /* Override scale after setup to preserve custom scale. */
    void setScaleX(float scaleX) {
        if (
            m_fields->m_isRodBall &&
            Mod::get()->getSettingValue<bool>("enabled") &&
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses")
        ) {
            GameObject::setScaleX(scaleX * kRodBallScale);
            disablePulse(this);
            return;
        }

        GameObject::setScaleX(scaleX);
    }

    /* Override scale after setup to preserve custom scale. */
    void setScaleY(float scaleY) {
        if (
            m_fields->m_isRodBall &&
            Mod::get()->getSettingValue<bool>("enabled") &&
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses")
        ) {
            GameObject::setScaleY(scaleY * kRodBallScale);
            disablePulse(this);
            return;
        }

        GameObject::setScaleY(scaleY);
    }
};
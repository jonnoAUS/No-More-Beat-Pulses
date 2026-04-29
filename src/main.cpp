/* Special thanks to flurrybun for the code refactor! */

#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/HardStreak.hpp>

using namespace geode::prelude;

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

class $modify(GameObject) {
    struct Fields {
        bool m_isRodBall = false;
        /* Fix: get previous orb state. */
        bool m_cachedPulseState = false;
        bool m_oldUsesAudioScale = false;
        bool m_oldHasNoAudioScale = false;
        bool m_oldCustomAudioScale = false;
        float m_oldMinAudioScale = 1.0f;
        float m_oldMaxAudioScale = 1.0f;
    };

    void cachePulseState() {
        if (m_fields->m_cachedPulseState) return;

        m_fields->m_oldUsesAudioScale = m_usesAudioScale;
        m_fields->m_oldHasNoAudioScale = m_hasNoAudioScale;
        m_fields->m_oldCustomAudioScale = m_customAudioScale;
        m_fields->m_oldMinAudioScale = m_minAudioScale;
        m_fields->m_oldMaxAudioScale = m_maxAudioScale;
        m_fields->m_cachedPulseState = true;
    }
    void restorePulse() {
        if (!m_fields->m_cachedPulseState) return;

        m_usesAudioScale = m_fields->m_oldUsesAudioScale;
        m_hasNoAudioScale = m_fields->m_oldHasNoAudioScale;
        m_customAudioScale = m_fields->m_oldCustomAudioScale;
        m_minAudioScale = m_fields->m_oldMinAudioScale;
        m_maxAudioScale = m_fields->m_oldMaxAudioScale;
    }

    bool shouldDisablePulseNow() {
        if (!Mod::get()->getSettingValue<bool>("enabled")) {
            return false;
        }

        if (m_fields->m_isRodBall) {
            return Mod::get()->getSettingValue<bool>("disable-decoration-pulses");
        }

        bool gameplay = m_classType == GameObjectClassType::Game;
        return gameplay
            ? Mod::get()->getSettingValue<bool>("disable-gameplay-pulses")
            : Mod::get()->getSettingValue<bool>("disable-decoration-pulses");
    }
    void applyPulseState() {
        if (shouldDisablePulseNow()) {
            disablePulse(this);
        } else {
            restorePulse();
        }
    }

    $override
    void customSetup() {
        GameObject::customSetup();

        m_fields->m_isRodBall = isRodBall(this);
        cachePulseState();
        applyPulseState();

        if (
            m_fields->m_isRodBall &&
            Mod::get()->getSettingValue<bool>("enabled") &&
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses")
        ) {
            GameObject::setScaleX(this->getScaleX() * kRodBallScale);
            GameObject::setScaleY(this->getScaleY() * kRodBallScale);
        }
    }

    /* Override scale after setup to preserve custom scale. */
    void setScale(float scale) {
        if (
            m_fields->m_isRodBall &&
            Mod::get()->getSettingValue<bool>("enabled") &&
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses")
        ) {
            GameObject::setScale(scale * kRodBallScale);
            applyPulseState();
            return;
        }

        GameObject::setScale(scale);
        applyPulseState();
    }

    /* Override scale after setup to preserve custom scale. */
    void setScaleX(float scaleX) {
        if (
            m_fields->m_isRodBall &&
            Mod::get()->getSettingValue<bool>("enabled") &&
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses")
        ) {
            GameObject::setScaleX(scaleX * kRodBallScale);
            applyPulseState();
            return;
        }

        GameObject::setScaleX(scaleX);
        applyPulseState();
    }

    /* Override scale after setup to preserve custom scale. */
    void setScaleY(float scaleY) {
        if (
            m_fields->m_isRodBall &&
            Mod::get()->getSettingValue<bool>("enabled") &&
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses")
        ) {
            GameObject::setScaleY(scaleY * kRodBallScale);
            applyPulseState();
            return;
        }

        GameObject::setScaleY(scaleY);
        applyPulseState();
    }
};

/* Remove pulsing of the wave. */
class $modify(HardStreak) {
    struct Fields {
        float m_basePulseSize = 0.0f;
        bool m_hasBasePulseSize = false;
    };

    bool init() {
        if (!HardStreak::init()) return false;

        if (!m_fields->m_hasBasePulseSize) {
            m_fields->m_basePulseSize = m_pulseSize > 0.0f ? m_pulseSize : m_waveSize;
            m_fields->m_hasBasePulseSize = true;
        }

        return true;
    }

    void updateStroke(float dt) {
        bool disableWavePulse =
            Mod::get()->getSettingValue<bool>("enabled") &&
            Mod::get()->getSettingValue<bool>("disable-wave-pulse");

        if (disableWavePulse) {
            if (!m_fields->m_hasBasePulseSize) {
                m_fields->m_basePulseSize = m_pulseSize > 0.0f ? m_pulseSize : m_waveSize;
                m_fields->m_hasBasePulseSize = true;
            }

            /* Pin before updates so frame is build from stable value. */
            m_pulseSize = m_fields->m_basePulseSize;
        }

        HardStreak::updateStroke(dt);

        if (disableWavePulse) {
            /* Pin again after just incase game rewrites every frame. (idk) */
            m_pulseSize = m_fields->m_basePulseSize;
        }
    }
};
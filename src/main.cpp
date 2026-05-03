/* Special thanks to flurrybun for the code refactor! */

#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/HardStreak.hpp>

using namespace geode::prelude;

static constexpr float kRodBallScale = 0.25f;

static bool settingEnabled() {
    return Mod::get()->getSettingValue<bool>("enabled");
}
static bool settingDisableGameplayPulses() {
    return Mod::get()->getSettingValue<bool>("disable-gameplay-pulses");
}
static bool settingDisableDecorationPulses() {
    return Mod::get()->getSettingValue<bool>("disable-decoration-pulses");
}
static bool settingDisableOrbHitPulse() {
    return Mod::get()->getSettingValue<bool>("disable-orb-hit-pulse");
}
static bool settingDisableWavePulse() {
    return Mod::get()->getSettingValue<bool>("disable-wave-pulse");
}

static bool isRodBall(GameObject* obj) {
    auto* sprite = typeinfo_cast<CCSprite*>(obj);
    auto* cache = CCSpriteFrameCache::sharedSpriteFrameCache();

    if (!sprite || !cache) return false;

    if (auto* frame = cache->spriteFrameByName("rod_ball_01_001.png"); frame && sprite->isFrameDisplayed(frame)) return true;
    if (auto* frame = cache->spriteFrameByName("rod_ball_02_001.png"); frame && sprite->isFrameDisplayed(frame)) return true;
    if (auto* frame = cache->spriteFrameByName("rod_ball_03_001.png"); frame && sprite->isFrameDisplayed(frame)) return true;

    return false;
}

static bool isDecorationObject(GameObject* obj) {
    return obj && (obj->m_isDecoration || obj->m_isDecoration2);
}

static void disablePulse(GameObject* obj) {
    obj->m_usesAudioScale = false;
    obj->m_hasNoAudioScale = true;
    obj->m_customAudioScale = false;
    obj->m_minAudioScale = 1.0f;
    obj->m_maxAudioScale = 1.0f;
}

class $modify(NoMoreBeatPulsesGameObject, GameObject) {
    struct Fields {
        bool m_isRodBall = false;
        bool m_finishedSetup = false;

        /* Fix: get previous orb state. */
        bool m_cachedPulseState = false;
        bool m_oldUsesAudioScale = false;
        bool m_oldHasNoAudioScale = false;
        bool m_oldCustomAudioScale = false;
        float m_oldMinAudioScale = 1.0f;
        float m_oldMaxAudioScale = 1.0f;

        bool m_hasBaseScale = false;
        float m_baseScaleX = 1.0f;
        float m_baseScaleY = 1.0f;
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

    void restorePulseState() {
        if (!m_fields->m_cachedPulseState) return;

        m_usesAudioScale = m_fields->m_oldUsesAudioScale;
        m_hasNoAudioScale = m_fields->m_oldHasNoAudioScale;
        m_customAudioScale = m_fields->m_oldCustomAudioScale;
        m_minAudioScale = m_fields->m_oldMinAudioScale;
        m_maxAudioScale = m_fields->m_oldMaxAudioScale;
    }

    void cacheBaseScale() {
        if (m_fields->m_hasBaseScale) return;

        m_fields->m_baseScaleX = getScaleX();
        m_fields->m_baseScaleY = getScaleY();
        m_fields->m_hasBaseScale = true;
    }

    void setBaseScale(float scaleX, float scaleY) {
        m_fields->m_baseScaleX = scaleX;
        m_fields->m_baseScaleY = scaleY;
        m_fields->m_hasBaseScale = true;
    }

    bool shouldTouchIdlePulse() {
        if (!settingEnabled()) return false;

        /* Rod balls are classified as deco. */
        if (m_fields->m_isRodBall) {
            return settingDisableDecorationPulses();
        }

        /* Game objects have priority. */
        if (m_classType == GameObjectClassType::Game) {
            return settingDisableGameplayPulses();
        }

        return settingDisableDecorationPulses();
    }

    bool shouldShrinkRodBall() {
        return
            m_fields->m_isRodBall &&
            settingEnabled() &&
            settingDisableDecorationPulses();
    }

    void applyPulseState() {
        cachePulseState();

        if (shouldTouchIdlePulse()) {
            disablePulse(this);
            return;
        }

        restorePulseState();
    }

    void applyRodBallScale() {
        if (!shouldShrinkRodBall()) return;

        cacheBaseScale();

        GameObject::setScaleX(m_fields->m_baseScaleX * kRodBallScale);
        GameObject::setScaleY(m_fields->m_baseScaleY * kRodBallScale);
    }

    $override
    void customSetup() {
        GameObject::customSetup();

        m_fields->m_isRodBall = isRodBall(this);

        cachePulseState();
        cacheBaseScale();

        applyPulseState();
        applyRodBallScale();

        m_fields->m_finishedSetup = true;
    }

    /* Override scale after setup to preserve custom scale. */
    $override
    void setScale(float scale) {
        if (!m_fields->m_finishedSetup) {
            GameObject::setScale(scale);
            return;
        }

        if (shouldShrinkRodBall()) {
            setBaseScale(scale, scale);

            GameObject::setScale(scale * kRodBallScale);
            applyPulseState();
            return;
        }

        GameObject::setScale(scale);
        applyPulseState();
    }

    /* Override scale after setup to preserve custom scale. */
    $override
    void setScaleX(float scaleX) {
        if (!m_fields->m_finishedSetup) {
            GameObject::setScaleX(scaleX);
            return;
        }

        if (shouldShrinkRodBall()) {
            m_fields->m_baseScaleX = scaleX;
            m_fields->m_hasBaseScale = true;

            GameObject::setScaleX(scaleX * kRodBallScale);
            applyPulseState();
            return;
        }

        GameObject::setScaleX(scaleX);
        applyPulseState();
    }

    /* Override scale after setup to preserve custom scale. */
    $override
    void setScaleY(float scaleY) {
        if (!m_fields->m_finishedSetup) {
            GameObject::setScaleY(scaleY);
            return;
        }

        if (shouldShrinkRodBall()) {
            m_fields->m_baseScaleY = scaleY;
            m_fields->m_hasBaseScale = true;

            GameObject::setScaleY(scaleY * kRodBallScale);
            applyPulseState();
            return;
        }

        GameObject::setScaleY(scaleY);
        applyPulseState();
    }
};

/* Remove pulsing of the wave. */
class $modify(NoMoreBeatPulsesHardStreak, HardStreak) {
    struct Fields {
        float m_basePulseSize = 0.0f;
        bool m_hasBasePulseSize = false;
    };

    void cachePulseSize() {
        if (m_fields->m_hasBasePulseSize) return;

        m_fields->m_basePulseSize = m_pulseSize > 0.0f ? m_pulseSize : m_waveSize;
        m_fields->m_hasBasePulseSize = true;
    }

    void pinPulseSize() {
        if (!settingEnabled() || !settingDisableWavePulse()) return;

        cachePulseSize();

        m_pulseSize = m_fields->m_basePulseSize;
    }

    $override
    bool init() {
        /* Remove wave pulse. */
        if (!HardStreak::init()) return false;

        cachePulseSize();
        pinPulseSize();

        return true;
    }

    $override
    void updateStroke(float dt) {
        /* Pin before updates so frame is build from stable value. */
        pinPulseSize();

        HardStreak::updateStroke(dt);

        /* Pin again after just incase game rewrites every frame. (idk) */
        pinPulseSize();
    }
};
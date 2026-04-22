#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

/* Sprite scale AFTER removing pulsing. */
constexpr float kRodBallScaleMultiplier = 0.80f;

namespace {
    /* Gameplay-side pulse objects. */
    bool isGameplayPulseObject(int id) {
        return
            (id >= 35 && id <= 36) ||       // bump_01, ring_01
            id == 84 ||                     // gravring_01
            (id >= 140 && id <= 141) ||     // bump_03, ring_03
            (id >= 1019 && id <= 1022) ||   // flash rings + grav jump ring
            id == 1330 ||                   // dropRing_01
            (id >= 1332 && id <= 1333) ||   // bump_02, ring_02
            id == 1594 ||                   // ring_custom_01
            id == 1704 ||                   // dashRing_01
            id == 1751 ||                   // dashRing_02
            id == 3004 ||                   // spiderRing_001
            id == 3027;                     // teleportRing_001
    }

    /* "Light square" deco families. */
    bool isLightSquareFamily(int id) {
        return
            id == 70 ||                     // lightsquare_01_02_001
            (id >= 76 && id <= 78) ||       // lightsquare_04_02_001 duplicate/variant entries
            (id >= 81 && id <= 82) ||       // lightsquare_04_02_001 + lightsquare_04_sideLine_001
            id == 91 ||                     // lightsquare_01_02_001
            id == 94 ||                     // lightsquare_01_05_color_001
            id == 161 ||                    // lightsquare_01_02_001
            (id >= 207 && id <= 213) ||     // lightsquare_01_01_001 -> lightsquare_01_07_001
            (id >= 247 && id <= 261) ||     // lightsquare_02_* + lightsquare_03_* families
            (id >= 263 && id <= 275) ||     // lightsquare_04_* + lightsquare_05_* families
            (id >= 277 && id <= 278) ||     // lightsquare_05_brick02_001, lightsquare_05_brick03_001
            (id >= 1820 && id <= 1821) ||   // lightsquare_02_01_color_001, lightsquare_02_02_color_001
            (id >= 1823 && id <= 1828);     // lightsquare_02_03_color_001 -> lightsquare_02_08_color_001
    }

    /**
     * Triangle light deco.
     *
     * Same idea as the square family, just for the pulse triangles.
     */
    bool isLightTriangleFamily(int id) {
        return
            (id >= 687 && id <= 688) ||     // lighttriangle_01_02_color_001, lighttriangle_01_04_color_001
            (id >= 693 && id <= 702) ||     // lighttriangle_01_* -> lighttriangle_05_* color variants
            (id >= 1747 && id <= 1748);     // repeated lighttriangle_01_* color variants
    }

    /* Block 011 light overlays. */
    bool isBlock011LightFamily(int id) {
        return
            (id >= 1367 && id <= 1386);     // block011_light_01_001 -> block011_light_20_001
    }

    /* Block 012 light overlays. */
    bool isBlock012LightFamily(int id) {
        return
            (id >= 1453 && id <= 1460);     // block012_light_01_001 -> block012_light_08_001
    }

    /**
     * Block 013 light overlays.
     *
     * This block family have both the normal light set and the `_c` variants.
     * Both visibly pulse, so both ranges are covered here.
     */
    bool isBlock013LightFamily(int id) {
        return
            (id >= 1653 && id <= 1668) ||   // block013_light_01_001 -> block013_light_16_001
            (id >= 1669 && id <= 1684);     // block013_light_c_01_001 -> block013_light_c_16_001
    }

    /* Extra deco families found whilst testing. */
    bool isMiscDecorationPulseObject(int id) {
        return
            id == 132 ||                    // d_arrow_01_001
            id == 460 ||                    // d_arrow_02_001
            id == 494 ||                    // d_arrow_03_001
            id == 133 ||                    // d_exmark_01_001
            id == 136 ||                    // d_qmark_01_001
            id == 150 ||                    // d_cross_01_001
            (id >= 15 && id <= 17) ||       // rod_01_001 -> rod_03_001
            (id >= 50 && id <= 54) ||       // d_ball_01_001 -> d_ball_05_001
            id == 60 ||                     // d_ball_06_001
            id == 148 || id == 149 ||       // d_ball_07_001, d_ball_08_001
            id == 236 ||                    // d_circle_01_001
            id == 495 || id == 496 ||       // d_largeSquare_01_001, d_largeSquare_02_001
            id == 497;                      // d_circle_02_001
    }

    /* Decoration-side pulse objects. */
    bool isDecorationPulseObject(int id) {
        return
            isLightSquareFamily(id) ||
            isLightTriangleFamily(id) ||
            isBlock011LightFamily(id) ||
            isBlock012LightFamily(id) ||
            isBlock013LightFamily(id) ||
            isMiscDecorationPulseObject(id);
    }

    /* Check whether a node is displaying the given sprite frame. */
    bool nodeIsSpriteFrame(CCNode* node, char const* frameName) {
        auto* sprite = typeinfo_cast<CCSprite*>(node);
        if (!sprite) return false;

        auto* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
        if (!cache) return false;

        auto* frame = cache->spriteFrameByName(frameName);
        if (!frame) return false;

        return sprite->isFrameDisplayed(frame);
    }

    /* Does this node have a pulse sprite? (only the rod sprites use this) */
    bool nodeHasPulseSprite(CCNode* node) {
        if (!node) return false;

        static constexpr char const* kPulseFrames[] = {
            "rod_ball_01_001.png",
            "rod_ball_02_001.png",
            "rod_ball_03_001.png",
        };

        for (auto const* frameName : kPulseFrames) {
            if (nodeIsSpriteFrame(node, frameName)) {
                return true;
            }

            if (getChildBySpriteFrameName(node, frameName) != nullptr) {
                return true;
            }
        }

        return false;
    }

    /* Should this object have its pulse suppressed? */
    bool shouldSuppressPulseForObject(GameObject* obj) {
        if (!obj) return false;

        if (!Mod::get()->getSettingValue<bool>("enabled")) {
            return false;
        }

        if (
            Mod::get()->getSettingValue<bool>("disable-gameplay-pulses") &&
            isGameplayPulseObject(obj->m_objectID)
        ) {
            return true;
        }

        if (
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses") &&
            (
                isDecorationPulseObject(obj->m_objectID) ||
                nodeHasPulseSprite(obj)
            )
        ) {
            return true;
        }

        return false;
    }

    /* Disables normal audio-scale on a specific object. */
    void disableBeatPulse(GameObject* obj) {
        if (!obj) {
            return;
        }

        /* Hard-disable participation in the audio-scale system. */
        obj->m_usesAudioScale = false;
        obj->m_hasNoAudioScale = true;

        /**
         * Reset any custom scale window so the object can't keep trying to
         * change toward a pulse range that was configed earlier.
         */
        obj->m_customAudioScale = false;
        obj->m_minAudioScale = 1.0f;
        obj->m_maxAudioScale = 1.0f;
    }
}

/* Main object hook. */
class $modify(NoMoreBeatPulsesGameObject, GameObject) {
    struct Fields {
        /* Cached once per object so ID classifier doesn't run on every callback. */
        bool m_shouldDisableBeatPulse = false;
        bool m_isRodBallObject = false;

        /* Store original scale. */
        float m_baseScaleX = 1.0f;
        float m_baseScaleY = 1.0f;
        bool m_hasCapturedBaseScale = false;
    };

    /* Refreshes cached suppression decision. */
    void refreshPulseDecision() {
        if (!m_fields->m_shouldDisableBeatPulse) {
            m_fields->m_shouldDisableBeatPulse = shouldSuppressPulseForObject(this);
        }

        if (!m_fields->m_isRodBallObject) {
            m_fields->m_isRodBallObject = nodeHasPulseSprite(this);
        }
    }

    void captureBaseScaleIfNeeded() {
        if (m_fields->m_hasCapturedBaseScale) {
            return;
        }

        m_fields->m_baseScaleX = this->getScaleX();
        m_fields->m_baseScaleY = this->getScaleY();
        m_fields->m_hasCapturedBaseScale = true;
    }

    void applyRodBallScale() {
        if (!m_fields->m_isRodBallObject) {
            return;
        }

        /* Use cached scale as source. */
        GameObject::setScaleX(m_fields->m_baseScaleX * kRodBallScaleMultiplier);
        GameObject::setScaleY(m_fields->m_baseScaleY * kRodBallScaleMultiplier);
    }

    void customSetup() {
        GameObject::customSetup();

        /* Classify after setup. */
        refreshPulseDecision();
        captureBaseScaleIfNeeded();

        /* Apply immediately after the normal object finishes setup. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }

        if (m_fields->m_isRodBallObject) {
            applyRodBallScale();
        }
    }

    void resetObject() {
        GameObject::resetObject();

        /* Some obj get reset/recycled, so refresh the pulse decision. */
        refreshPulseDecision();

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }

        if (m_fields->m_isRodBallObject) {
            applyRodBallScale();
        }
    }

    void setOpacity(u_char opacity) {
        refreshPulseDecision();

        if (m_fields->m_isRodBallObject) {
            GameObject::setOpacity(opacity >= 255 ? 255 : 0);

            if (m_fields->m_shouldDisableBeatPulse) {
                disableBeatPulse(this);
            }
            return;
        }

        GameObject::setOpacity(opacity);

        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void setVisible(bool visible) {
        refreshPulseDecision();

        if (m_fields->m_isRodBallObject) {
            GameObject::setVisible(visible);
            if (!visible) {
                GameObject::setOpacity(0);
            }
            return;
        }

        GameObject::setVisible(visible);
    }

    void deactivateObject(bool deactivate) {
        GameObject::deactivateObject(deactivate);

        refreshPulseDecision();

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }

        if (m_fields->m_isRodBallObject) {
            applyRodBallScale();
        }
    }

    void activateObject() {
        GameObject::activateObject();

        refreshPulseDecision();

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }

        if (m_fields->m_isRodBallObject) {
            applyRodBallScale();
        }
    }

    void setScale(float scale) {
        refreshPulseDecision();

        if (m_fields->m_isRodBallObject) {
            m_fields->m_baseScaleX = scale;
            m_fields->m_baseScaleY = scale;
            m_fields->m_hasCapturedBaseScale = true;

            GameObject::setScale(scale * kRodBallScaleMultiplier);

            if (m_fields->m_shouldDisableBeatPulse) {
                disableBeatPulse(this);
            }
            return;
        }

        GameObject::setScale(scale);

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void setScaleX(float scaleX) {
        refreshPulseDecision();

        if (m_fields->m_isRodBallObject) {
            m_fields->m_baseScaleX = scaleX;
            m_fields->m_hasCapturedBaseScale = true;

            GameObject::setScaleX(scaleX * kRodBallScaleMultiplier);

            if (m_fields->m_shouldDisableBeatPulse) {
                disableBeatPulse(this);
            }
            return;
        }

        GameObject::setScaleX(scaleX);

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void setScaleY(float scaleY) {
        refreshPulseDecision();

        if (m_fields->m_isRodBallObject) {
            m_fields->m_baseScaleY = scaleY;
            m_fields->m_hasCapturedBaseScale = true;

            GameObject::setScaleY(scaleY * kRodBallScaleMultiplier);

            if (m_fields->m_shouldDisableBeatPulse) {
                disableBeatPulse(this);
            }
            return;
        }

        GameObject::setScaleY(scaleY);

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }
};
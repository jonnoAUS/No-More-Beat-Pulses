#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/PlayLayer.hpp>


using namespace geode::prelude;

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
            id == 1751;                     // dashRing_02
    }

    /**
     * "Light square" deco families.
     *
     * These are some of the most obvious beat pulse decorations.
     * Several IDs are duplicated vars of the same visual,
     * so some ranges look a bit weird but are intentional.
     */
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

    /**
     * Block 011 light overlays.
     *
     * These are the glowing overlay pieces on the 011 block list.
     */
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

    /* Decoration-side pulse objects. */
    bool isDecorationPulseObject(int id) {
        return
            isLightSquareFamily(id) ||
            isLightTriangleFamily(id) ||
            isBlock011LightFamily(id) ||
            isBlock012LightFamily(id) ||
            isBlock013LightFamily(id) ||
            /* Extra IDs found during testing. */
            id == 132 ||                    // d_arrow_01_001
            id == 460 ||                    // d_arrow_02_001
            id == 494 ||                    // d_arrow_03_001
            id == 133 ||                    // d_exmark_01_001
            id == 136 ||                    // d_qmark_01_001
            id == 150;                      // d_cross_01_001
    }

    /* Should this object ID be blocked from pulsing? */
    bool shouldSuppressPulseForID(int id) {
        if (!Mod::get()->getSettingValue<bool>("enabled")) {
            return false;
        }

        if (
            Mod::get()->getSettingValue<bool>("disable-gameplay-pulses") &&
            isGameplayPulseObject(id)
        ) {
            return true;
        }

        if (
            Mod::get()->getSettingValue<bool>("disable-decoration-pulses") &&
            isDecorationPulseObject(id)
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
    };

    void customSetup() {
        GameObject::customSetup();

        /* Does this object belong to a family that should have pulses suppressed? */
        m_fields->m_shouldDisableBeatPulse = shouldSuppressPulseForID(this->m_objectID);

        /* Apply immediately after the normal object finishes setup. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void resetObject() {
        GameObject::resetObject();

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void deactivateObject(bool deactivate) {
        GameObject::deactivateObject(deactivate);

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void activateObject() {
        GameObject::activateObject();

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void setScale(float scale) {
        GameObject::setScale(scale);

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void setScaleX(float scaleX) {
        GameObject::setScaleX(scaleX);

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }

    void setScaleY(float scaleY) {
        GameObject::setScaleY(scaleY);

        /* Reapply the audio-scale block if necessary. */
        if (m_fields->m_shouldDisableBeatPulse) {
            disableBeatPulse(this);
        }
    }
};
/**
 * @file jam_ScaledContent.h
 * @brief Component wrapper that always holds a scaled target layer content
 *        is built into.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ScaledContent
 * @brief Base component owning a target layer, scaled to the host DPI
 *        factor, that content is always built into rather than this
 *        component directly.
 */
class ScaledContent : public juce::Component
{
protected:
    /** @brief Scales the target layer to the current host scale and adds it as a child. */
    ScaledContent()
    {
        targetComponent.setTransform (juce::AffineTransform::scale (getHostScale()));
        addAndMakeVisible (targetComponent);
    }

    /** @return The scaled target layer content is built into. */
    juce::Component* getTargetComponent() noexcept { return &targetComponent; }

private:
    /** Target layer, present unconditionally, that content is built into. */
    juce::Component targetComponent;
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */

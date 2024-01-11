#pragma once

//#include "juce_gui_basics/components/juce_Component.h"
#include "../JuceLibraryCode/JuceHeader.h"

class SynthComponent : public juce::Component
{
public:
    virtual ~SynthComponent() {}

    virtual void updateControls() = 0;
};
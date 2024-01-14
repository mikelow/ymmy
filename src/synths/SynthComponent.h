#pragma once

//#include "juce_gui_basics/components/juce_Component.h"
#include "JuceHeader.h"

class SynthComponent : public juce::Component
{
public:
  SynthComponent(juce::AudioProcessorValueTreeState& vts);
  virtual ~SynthComponent() {}

  virtual void updateControls() = 0;

protected:
  juce::AudioProcessorValueTreeState& vts;
};
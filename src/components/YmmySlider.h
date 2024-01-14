#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "VTSParamComponent.h"


class YmmySlider: public Component
//                  private Slider::Listener
{
public:
  typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

//  YmmySlider(AudioProcessorValueTreeState& valueTreeState, const String& paramId)
  YmmySlider(AudioProcessorValueTreeState& valueTreeState, const String& parameterId)
      : /*vts(valueTreeState), paramId(parameterId),*/ component() {
    component.setSliderStyle(juce::Slider::LinearBarVertical);
//    component.setRange(0.0, 127.0, 1.0);
//    component.setRange(0, 127, 1);
    component.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    component.setPopupDisplayEnabled(true, false, this);
    component.setValue(1.0);

    attachment = std::make_unique<SliderAttachment>(valueTreeState, parameterId, component);

    addAndMakeVisible (component);
  }

  ~YmmySlider() {
  }

  void resized() {
    Rectangle<int> r (getLocalBounds());
    component.setBounds(r);
  }

public:
  Slider component;
private:
  std::unique_ptr<SliderAttachment> attachment;
//  AudioProcessorValueTreeState& vts;
//  const String& paramId;
};

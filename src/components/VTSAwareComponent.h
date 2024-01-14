#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

template <class T>
class VTSAwareComponent: public Component,
                         public ValueTree::Listener
{
public:
  VTSAwareComponent(
    AudioProcessorValueTreeState& valueTreeState,
    T& wrapperComponent,
    const ValueTree& p
  ) : vts(valueTreeState), component(wrapperComponent), path(p) {
//    component.addListener(this);
  }

  ~VTSAwareComponent() {
//    component.removeListener(this);
    vts.state.removeListener(this);
  }

  virtual void vtsPropertyChanged(var& property) = 0;

  void resized() {
    Rectangle<int> r (getLocalBounds());
    component.setBounds(r);
  }

  void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) {

    if (treeWhosePropertyHasChanged.getType() != path.getType()) {
      return;
    }
    if (property == path.getPropertyName(0)) {
      auto prop = treeWhosePropertyHasChanged.getProperty(path.getPropertyName(0), "");
      vtsPropertyChanged(prop);
    }
  }

protected:
  void setVtsProperty(const var& newValue) {
    auto typeName = path.getType();
    auto propName = path.getPropertyName(0);
    Value value{vts.state.getChildWithName(typeName).getPropertyAsValue(propName, nullptr)};
    value.setValue(newValue);
  }

  ValueTree path;
  AudioProcessorValueTreeState& vts;
  T& component;
};

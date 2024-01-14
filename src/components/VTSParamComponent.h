//#pragma once
//
//#include "../JuceLibraryCode/JuceHeader.h"
//
//template <class T, class TAttachment>
//class VTSParamComponent: public Component
//{
//public:
//  VTSParamComponent(
//      AudioProcessorValueTreeState& valueTreeState,
//      T& wrapperComponent,
//      const String& pId
//      ) : vts(valueTreeState), component(wrapperComponent), parameterId(pId) {
////    attachment = std::make_unique<TAttachment>(vts, parameterId, component);
//  }
//
//  ~VTSParamComponent() {
//  }
//
//  void resized() {
//    Rectangle<int> r (getLocalBounds());
//    component.setBounds(r);
//  }
//
//
//protected:
//  std::unique_ptr<TAttachment> attachment;
//  const String& parameterId;
//  AudioProcessorValueTreeState& vts;
//public:
//  T& component;
//
//};

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "VTSAwareComponent.h"

class FilePicker: public VTSAwareComponent<FilenameComponent>,
                  private FilenameComponentListener
{
public:
  FilePicker(AudioProcessorValueTreeState& valueTreeState);
  ~FilePicker() override;

  void setDisplayedFilePath(const String&);
  void vtsPropertyChanged(var& property) override;

private:
  FilenameComponent fileChooser;
  void filenameComponentChanged (FilenameComponent*) override;
  bool shouldChangeDisplayedFilePath(const String &path);
};

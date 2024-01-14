#include "FilePicker.h"

FilePicker::FilePicker(AudioProcessorValueTreeState& valueTreeState)
    : VTSAwareComponent<FilenameComponent>(
          valueTreeState, fileChooser, { "soundFont", { { "path", "" } }, {} }
      ),
      fileChooser(
          "File Path",
          File(),
          true,
          false,
          false,
          "*.sf2;*.sf3",
          String(),
          "Select a SoundFont file") {
  setDisplayedFilePath(valueTreeState.state.getChildWithName("soundFont").getProperty("path", ""));

  addAndMakeVisible (fileChooser);
  fileChooser.addListener(this);
}

FilePicker::~FilePicker() {
  fileChooser.removeListener(this);
}

void FilePicker::vtsPropertyChanged(var& property) {
  setDisplayedFilePath(property);
}

void FilePicker::filenameComponentChanged (FilenameComponent*) {
//  Value value{vts.state.getChildWithName("soundFont").getPropertyAsValue("path", nullptr)};
//  value.setValue(fileChooser.getCurrentFile().getFullPathName());
  setVtsProperty(fileChooser.getCurrentFile().getFullPathName());
}

void FilePicker::setDisplayedFilePath(const String& path) {
  if (!shouldChangeDisplayedFilePath(path)) {
    return;
  }
  // currentPath = path;
  fileChooser.setCurrentFile(File(path), true, dontSendNotification);
}

bool FilePicker::shouldChangeDisplayedFilePath(const String &path) {
  if (path.isEmpty()) {
    return false;
  }
  return true;
}
#include "FilePicker.h"

FilePicker::FilePicker(AudioProcessorValueTreeState& valueTreeState,
                       const ValueTree& path,
                       const String& fileBrowserWildcard,
                       const String& textWhenNothingSelected)
    : VTSAwareComponent<FilenameComponent>(
          valueTreeState, fileChooser, path
      ),
      fileChooser(
          "File Path",
          File(),
          true,
          false,
          false,
          fileBrowserWildcard,
          String(),
          textWhenNothingSelected) {
  auto typeName = path.getType();
  auto propName = path.getPropertyName(0);
//  setDisplayedFilePath(valueTreeState.state.getChildWithName("soundFont").getProperty("path", ""));
  setDisplayedFilePath(valueTreeState.state.getChildWithName(typeName).getProperty(propName, ""));

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
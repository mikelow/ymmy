cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME "ymmy")

set(VERSION "0.0.1")
set(JUCE_DIR "./lib/JUCE")
set(JUCE_MODULES_DIR "${JUCE_DIR}/modules")
set(FLUIDSYNTH_DIR "./lib/fluidsynth")
set(FLUIDSYNTH_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/fluidsynth")
set(YMFM_DIR "./lib/ymfm")
set(SRC_DIR "./src")
set(FORMATS "Standalone" "VST3")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(${PROJECT_NAME} VERSION ${VERSION})

add_subdirectory(${JUCE_DIR} "juce")

juce_add_plugin(${PROJECT_NAME}
        PLUGIN_NAME "ymmy"
        VERSION "${VERSION}"
        DESCRIPTION "Delicious nostalgia"
        PLUGIN_MANUFACTURER_CODE "vgmt"
        PLUGIN_CODE "Ymmy"
        COMPANY_NAME "VGMTrans"
        COMPANY_WEBSITE "https://github.com/vgmtrans"
        BUNDLE_ID "com.vgmtrans.${PROJECT_NAME}"
        ICON_BIG "ymmy.png"
        FORMATS ${FORMATS}
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        AU_SANDBOX_SAFE TRUE
)

juce_generate_juce_header(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${FLUIDSYNTH_BUILD_DIR}/include"
    "${FLUIDSYNTH_DIR}/include"
    "${YMFM_DIR}/src"
    ./src/common
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    JucePlugin_Build_Standalone=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_sources(${PROJECT_NAME}
  PRIVATE
    "${SRC_DIR}/YmmyProcessor.cpp"
    "${SRC_DIR}/YmmyEditor.cpp"
    "${SRC_DIR}/components/FilePicker.cpp"
    "${SRC_DIR}/synths/Synth.cpp"
    "${SRC_DIR}/synths/SynthComponent.cpp"
    "${SRC_DIR}/synths/fluidsynth/FluidSynthComponent.cpp"
    "${SRC_DIR}/synths/fluidsynth/FluidSynthSynth.cpp"
    "${SRC_DIR}/synths/ym2151/YM2151Component.cpp"
    "${SRC_DIR}/synths/ym2151/YM2151Synth.cpp"
    "${SRC_DIR}/synths/ym2151/OPMFileLoader.cpp"
    "${SRC_DIR}/common/utility.cpp"
    "./lib/ymfm/src/ymfm_opm.cpp"
)

# Fluidsynth library options
set(enable-floats ON CACHE BOOL "Enable floats")
set(enable-libsndfile ON CACHE BOOL "Enable libsndfile")
set(enable-threads ON CACHE BOOL "Enable threads")
set(enable-pkgconfig ON CACHE BOOL "Enable pkgconfig")
set(enable-aufile OFF CACHE BOOL "Enable aufile")
set(enable-dbus OFF CACHE BOOL "Enable dbus")
set(enable-dsound OFF CACHE BOOL "Enable dsound")
set(enable-ipv6 OFF CACHE BOOL "Enable ipv6")
set(enable-jack OFF CACHE BOOL "Enable jack")
set(enable-ladspa OFF CACHE BOOL "Enable ladspa")
set(enable-libinstpatch OFF CACHE BOOL "Enable libinstpatch")
set(enable-midishare OFF CACHE BOOL "Enable midishare")
set(enable-network OFF CACHE BOOL "Enable network")
set(enable-openmp OFF CACHE BOOL "Enable openmp")
set(enable-oss OFF CACHE BOOL "Enable oss")
set(enable-pulseaudio OFF CACHE BOOL "Enable pulseaudio")
set(enable-readline OFF CACHE BOOL "Enable readline")
set(enable-sdl2 OFF CACHE BOOL "Enable sdl2")
set(enable-wasapi OFF CACHE BOOL "Enable wasapi")
set(enable-waveout OFF CACHE BOOL "Enable waveout")
set(enable-winmidi OFF CACHE BOOL "Enable winmidi")
set(enable-alsa OFF CACHE BOOL "Enable alsa")
set(enable-lash OFF CACHE BOOL "Enable lash")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${FLUIDSYNTH_DIR}" ${FLUIDSYNTH_BUILD_DIR} EXCLUDE_FROM_ALL)
add_dependencies(${PROJECT_NAME} libfluidsynth)

target_link_libraries(${PROJECT_NAME} PRIVATE libfluidsynth)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client_Standalone
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

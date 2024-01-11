#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class InstrServerConnection;


class InstrServer : public InterprocessConnectionServer
{
public:
    InstrServer(AudioProcessorValueTreeState& valueTreeState);
    ~InstrServer() override;

    void openPipe();
    InterprocessConnection* createConnectionObject() override;
private:
    AudioProcessorValueTreeState& valueTreeState;
    OwnedArray<InstrServerConnection> connections;
};


class InstrServerConnection: public InterprocessConnection {
public:
    InstrServerConnection(AudioProcessorValueTreeState& valueTreeState);
    ~InstrServerConnection() override;

    enum ConnectionState
    {
        Connecting = 0,
        Connected,
        Disconnected
    };

    enum MessageState {
        WaitingForMessage,
        ReceivingSF2,
    };

    void connectionMade() override;
    void connectionLost() override;

    void messageReceived(const juce::MemoryBlock &message) override;

private:
    AudioProcessorValueTreeState& valueTreeState;
    ConnectionState connectionState;
    MessageState messageState;
    std::unique_ptr<juce::MemoryBlock> fileContent;
    CriticalSection fLock;
    ConnectionState fConnected;
};

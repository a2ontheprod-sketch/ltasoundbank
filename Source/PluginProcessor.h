#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

struct LTAPreset
{
    juce::String name;
    int category = 0, type = 0;
    float attack = 0.01f, decay = 0.3f, sustain = 0.45f, release = 0.3f;
    float brightness = 0.65f, resonance = 0.15f;
    float detune = 0.005f, fm = 0.0f, noise = 0.0f, glide = 0.0f;
    float vibrato = 0.0f, drive = 0.0f, space = 0.15f, motion = 0.0f;
    float harmonic = 0.35f, level = 0.23f;
    int arpRate = 2, arpStyle = 0;
};

class LTASoundbankAudioProcessor : public juce::AudioProcessor
{
public:
    LTASoundbankAudioProcessor();
    ~LTASoundbankAudioProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "LTA Soundbank"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 400; }
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;

    static constexpr int numPresets = 400;
    static constexpr int categoryCount = 9;
    static const std::array<const char*,9> categoryNames;
    static const std::array<LTAPreset,400>& presets();
    void selectPreset(int index);
    int selectedPreset() const { return presetIndex.load(); }
    void requestPreview() { previewRequest.store(true); }
    void setFavourite(int index, bool on);
    bool isFavourite(int index) const;
    float scopeSample(int i) const;
    int scopePosition() const { return scopeHead.load(std::memory_order_acquire); }
    float outputLevel() const { return levelMeter.load(); }
    juce::AudioProcessorValueTreeState parameters;

private:
    struct Voice
    {
        bool active=false, held=false;
        int note=60, age=0;
        float velocity=0.0f, env=0.0f, phase=0.0f, phase2=0.0f;
        float lastHz=440.0f, filter=0.0f, lowpass2=0.0f;
        int stage=0;
    };
    std::array<Voice, 32> voices {};
    std::array<std::atomic<bool>,400> favourites {};
    std::array<std::atomic<float>,512> scope {};
    std::atomic<int> scopeHead {0};
    std::atomic<float> levelMeter {0};
    std::atomic<int> presetIndex {0};
    std::atomic<bool> previewRequest {false};
    double sr=44100.0, sampleClock=0, arpClock=0, lastBpm=120;
    std::array<bool,128> heldNotes {};
    int arpStep=0, previousArpNote=-1, previewRemaining=0;
    float delayL[32768] {}, delayR[32768] {};
    int delayPtr=0;
    int voiceFor(int note);
    void noteOn(int note,float velocity, const LTAPreset&);
    void noteOff(int note);
    float renderVoice(Voice&, const LTAPreset&);
    void handleMidi(const juce::MidiMessage&, const LTAPreset&);
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LTASoundbankAudioProcessor)
};

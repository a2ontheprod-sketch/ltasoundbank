#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <vector>

class LTASoundbankAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit LTASoundbankAudioProcessorEditor(LTASoundbankAudioProcessor&);
    ~LTASoundbankAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    LTASoundbankAudioProcessor& processor;
    juce::TextEditor search;
    bool browsing=false, favOnly=false;
    int activeCategory=-1, firstVisible=0;
    float visualLevel=0.0f, transition=0.0f;
    int lastPreset=-1, animationFrame=0;
    std::vector<int> matches;
    juce::Rectangle<int> waveRect, browserRect, searchRect;
    juce::Rectangle<int> backRect, nextRect, previewRect, surpriseRect, browseRect, favRect;
    juce::Rectangle<int> categoryRect, gridRect;
    void rebuildList();
    void choosePreset(int);
    void drawPlay(juce::Graphics&);
    void drawBrowser(juce::Graphics&);
    void drawPill(juce::Graphics&, juce::Rectangle<int>, juce::String, bool active=false);
    void timerCallback() override;
    int positionToPreset(juce::Point<int>) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LTASoundbankAudioProcessorEditor)
};

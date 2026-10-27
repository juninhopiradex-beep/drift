#pragma once

#include "PluginProcessor.h"

// GUI moderna e simples: fundo escuro, cartões finos, knob grande de Amount,
// medidor de variação em tempo real e seletor de modo.
class DriftEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit DriftEditor(DriftProcessor&);
    ~DriftEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    DriftProcessor& processor;

    juce::Slider amountKnob;
    juce::Label amountLabel;

    juce::Slider grooveSlider, attackSlider, airSlider, gritSlider;
    juce::Label grooveLabel, attackLabel, airLabel, gritLabel;

    juce::ComboBox modeBox;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> amountAtt, grooveAtt, attackAtt, airAtt, gritAtt;
    std::unique_ptr<ComboAttachment> modeAtt;

    float meterValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftEditor)
};

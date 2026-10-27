#include "PluginEditor.h"

namespace
{
    const auto bgColour     = juce::Colour::fromString("FF0F1114");
    const auto cardColour   = juce::Colour::fromString("FF161A1F");
    const auto borderColour = juce::Colour::fromString("FF242A32");
    const auto accent       = juce::Colour::fromString("FF7FD1B9");
    const auto textColour   = juce::Colour::fromString("FFE8EBF0");
    const auto mutedColour  = juce::Colour::fromString("FF6B7280");

    void styleSlider(juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
        s.setColour(juce::Slider::trackColourId, accent);
        s.setColour(juce::Slider::backgroundColourId, borderColour);
        s.setColour(juce::Slider::thumbColourId, textColour);
        s.setColour(juce::Slider::textBoxTextColourId, textColour);
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }

    void styleLabel(juce::Label& l, const juce::String& text)
    {
        l.setText(text, juce::dontSendNotification);
        l.setColour(juce::Label::textColourId, mutedColour);
        l.setFont(juce::FontOptions(11.0f));
    }
}

DriftEditor::DriftEditor(DriftProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(360, 470);

    styleSlider(amountKnob);
    amountKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    amountKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    amountKnob.setColour(juce::Slider::rotarySliderFillColourId, accent);
    amountKnob.setColour(juce::Slider::rotarySliderOutlineColourId, borderColour);
    amountKnob.setColour(juce::Slider::thumbColourId, textColour);
    styleLabel(amountLabel, "Quantidade");

    styleSlider(grooveSlider); styleLabel(grooveLabel, "Timing · Groove");
    styleSlider(attackSlider); styleLabel(attackLabel, "Ataque · Attack");
    styleSlider(airSlider);    styleLabel(airLabel, "Ar · Air");
    styleSlider(gritSlider);   styleLabel(gritLabel, "Textura · Grit");

    modeBox.addItem("Livre", 1);
    modeBox.addItem("Sync", 2);
    modeBox.addItem("Adaptativo", 3);
    modeBox.setColour(juce::ComboBox::backgroundColourId, cardColour);
    modeBox.setColour(juce::ComboBox::textColourId, textColour);
    modeBox.setColour(juce::ComboBox::outlineColourId, borderColour);

    for (auto* c : { &amountKnob, &grooveSlider, &attackSlider, &airSlider, &gritSlider })
        addAndMakeVisible(c);
    for (auto* l : { &amountLabel, &grooveLabel, &attackLabel, &airLabel, &gritLabel })
        addAndMakeVisible(l);
    addAndMakeVisible(modeBox);

    auto& apvts = processor.apvts;
    amountAtt = std::make_unique<SliderAttachment>(apvts, "amount", amountKnob);
    grooveAtt = std::make_unique<SliderAttachment>(apvts, "groove", grooveSlider);
    attackAtt = std::make_unique<SliderAttachment>(apvts, "attack", attackSlider);
    airAtt    = std::make_unique<SliderAttachment>(apvts, "air", airSlider);
    gritAtt   = std::make_unique<SliderAttachment>(apvts, "grit", gritSlider);
    modeAtt   = std::make_unique<ComboAttachment>(apvts, "mode", modeBox);

    startTimerHz(30);
}

DriftEditor::~DriftEditor()
{
    stopTimer();
}

void DriftEditor::timerCallback()
{
    // Suavização do medidor para leitura confortável.
    const float target = processor.meterTimingMs.load();
    meterValue += (target - meterValue) * 0.2f;
    repaint();
}

void DriftEditor::paint(juce::Graphics& g)
{
    g.fillAll(bgColour);

    auto area = getLocalBounds().reduced(14);

    // Cabeçalho
    auto header = area.removeFromTop(40);
    g.setColour(textColour);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("DRIFT", header.removeFromLeft(120), juce::Justification::centredLeft);
    g.setColour(mutedColour);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Micro-variação natural", header, juce::Justification::centredLeft);

    // Cartão do medidor
    auto meter = area.removeFromTop(64);
    g.setColour(cardColour);
    g.fillRoundedRectangle(meter.toFloat(), 12.0f);
    g.setColour(borderColour);
    g.drawRoundedRectangle(meter.toFloat(), 12.0f, 0.5f);

    g.setColour(mutedColour);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Desvio de timing", meter.reduced(12, 8).removeFromTop(14), juce::Justification::left);
    g.setColour(accent);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(juce::String(meterValue, 1) + " ms",
               meter.reduced(12, 8).removeFromBottom(18), juce::Justification::right);
}

void DriftEditor::resized()
{
    auto area = getLocalBounds().reduced(14);
    area.removeFromTop(40);      // cabeçalho
    area.removeFromTop(64 + 10); // medidor

    // Grelha 2x2 dos módulos.
    auto grid = area.removeFromTop(190);
    const int cellW = grid.getWidth() / 2;
    const int cellH = grid.getHeight() / 2;

    auto place = [&](juce::Label& l, juce::Slider& s, int col, int row)
    {
        auto cell = juce::Rectangle<int>(grid.getX() + col * cellW,
                                         grid.getY() + row * cellH,
                                         cellW - 6, cellH - 6).reduced(4);
        l.setBounds(cell.removeFromTop(14));
        s.setBounds(cell.removeFromTop(24));
    };

    place(grooveLabel, grooveSlider, 0, 0);
    place(attackLabel, attackSlider, 1, 0);
    place(airLabel,    airSlider,    0, 1);
    place(gritLabel,   gritSlider,   1, 1);

    area.removeFromTop(12);

    // Faixa inferior: knob de Quantidade + modos.
    auto footer = area.removeFromTop(140);
    auto knobArea = footer.removeFromLeft(130);
    amountKnob.setBounds(knobArea.reduced(4, 8));
    amountLabel.setBounds(knobArea.removeFromBottom(0));

    footer.removeFromLeft(12);
    modeBox.setBounds(footer.removeFromTop(30).reduced(0, 2));
}
